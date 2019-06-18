#include "pch.h"
#include "MIDI.h"

#pragma comment(lib, "winmm")

#define MThd		0x6468544D		// Start of file
#define MTrk		0x6B72544D		// Start of track

#define BUFFER_TIME_LENGTH		60   // Amount to fill in milliseconds

// These structures are stored in MIDI files; they need to be byte aligned.
//
#pragma pack(1)

// Contents of MThd chunk.
struct MIDIFILEHDR
{
    WORD	wFormat;			// Format (hi-lo)
    WORD	wTrackCount;		// # tracks (hi-lo)
    WORD	wTimeDivision;		// Time division (hi-lo)
};

#pragma pack() // End of need for byte-aligned structures


// Macros for swapping hi/lo-endian data
//
#define WORDSWAP(w)		(((w) >> 8) | \
						(((w) << 8) & 0xFF00))

#define DWORDSWAP(dw)	(((dw) >> 24) | \
						(((dw) >> 8) & 0x0000FF00) | \
						(((dw) << 8) & 0x00FF0000) | \
						(((dw) << 24) & 0xFF000000))


static char gteBadRunStat[] 	= "Reference to missing running status.";
static char gteRunStatMsgTrunc[]= "Running status message truncated";
static char gteChanMsgTrunc[]	= "Channel message truncated";
static char gteSysExLenTrunc[]	= "SysEx event truncated (length)";
static char gteSysExTrunc[]	= "SysEx event truncated";
static char gteMetaNoClass[]	= "Meta event truncated (no class byte)";
static char gteMetaLenTrunc[]	= "Meta event truncated (length)";
static char gteMetaTrunc[]	= "Meta event truncated";
static char gteNoMem[]		= "Out of memory during malloc call";


//////////////////////////////////////////////////////////////////////
// CMIDI -- Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMIDI::CMIDI()
	: m_dwSoundSize(0)
	, m_pSoundData(0)
	, m_dwFormat(0)
	, m_dwTrackCount(0)
	, m_dwTimeDivision(0)
	, m_bPlaying(FALSE)
	, m_hStream(0)
	, m_dwProgressBytes(0)
	, m_bLooped(FALSE)
	, m_tkCurrentTime(0)
	, m_dwBufferTickLength(0)
	, m_dwCurrentTempo(0)
	, m_dwTempoMultiplier(100)
	, m_bInsertTempo(FALSE)
	, m_bBuffersPrepared(FALSE)
	, m_nCurrentBuffer(0)
	, m_uMIDIDeviceID(MIDI_MAPPER)
	, m_nEmptyBuffers(0)
	, m_bPaused(FALSE)
	, m_uCallbackStatus(0)
	, m_hBufferReturnEvent(0)

	, m_ptsTrack(0)
	, m_ptsFound(0)
	, m_dwStatus(0)
	, m_tkNext(0)
	, m_dwMallocBlocks(0)
{
	m_hBufferReturnEvent = ::CreateEvent(0, FALSE, FALSE, TEXT("Wait For Buffer Return"));
	//ASSERT(m_hBufferReturnEvent != 0);
}

CMIDI::~CMIDI()
{
	Stop(FALSE);

	if(m_hBufferReturnEvent)
		::CloseHandle(m_hBufferReturnEvent);
}


BOOL CMIDI::Create(UINT uResID, HWND pWndParent /* = NULL */)
{
	return	Create(MAKEINTRESOURCE(uResID), pWndParent);
}


BOOL CMIDI::Create(LPCTSTR pszResID, HWND pWndParent /* = NULL */)
{
	//////////////////////////////////////////////////////////////////
	// load resource
	HINSTANCE hApp = ::GetModuleHandle(0);
	//ASSERT(hApp);

	HRSRC hResInfo = ::FindResource(hApp, pszResID, TEXT("MIDI"));
	if(hResInfo == 0)
		return FALSE;

	HGLOBAL hRes = ::LoadResource(hApp, hResInfo);
	if(hRes == 0)
		return FALSE;

	LPVOID pTheSound = ::LockResource(hRes);
	if(pTheSound == 0)
		return FALSE;

	DWORD dwTheSound = ::SizeofResource(hApp, hResInfo);

	return Create((LPBYTE)pTheSound, dwTheSound, pWndParent);
}


BOOL CMIDI::Create(LPBYTE pSoundData, DWORD dwSize, HWND pWndParent /* = NULL */)
{
	if( m_pSoundData ) {
		// already created
		//ASSERT(FALSE);
		return FALSE;
	}

	//ASSERT(pSoundData != 0);
	//ASSERT(dwSize > 0);

	register LPBYTE p = LPBYTE(pSoundData);

	// check header of MIDI
	if(*(DWORD*)p != MThd) {
		//ASSERT(FALSE);
		return FALSE;
	}
	p += sizeof(DWORD);

	// check header size
	DWORD dwHeaderSize = DWORDSWAP(*(DWORD*)p);
	if( dwHeaderSize != sizeof(MIDIFILEHDR) ) {
		//ASSERT(FALSE);
		return FALSE;
	}
	p += sizeof(DWORD);

	// get header
	MIDIFILEHDR hdr;
	::CopyMemory(&hdr, p, dwHeaderSize);
	m_dwFormat = DWORD(WORDSWAP(hdr.wFormat));
	m_dwTrackCount = DWORD(WORDSWAP(hdr.wTrackCount));
	m_dwTimeDivision = DWORD(WORDSWAP(hdr.wTimeDivision));
	p += dwHeaderSize;

	// create the array of tracks
	m_Tracks.resize(m_dwTrackCount);
	for(register DWORD i = 0; i < m_dwTrackCount; ++i) {
		// check header of track
		if(*(DWORD*)p != MTrk) {
			//ASSERT(FALSE);
			return FALSE;
		}
		p += sizeof(DWORD);

		m_Tracks[i].dwTrackLength = DWORDSWAP(*(DWORD*)p);
		p += sizeof(DWORD);

		m_Tracks[i].pTrackStart = m_Tracks[i].pTrackCurrent = p;
			p += m_Tracks[i].dwTrackLength;

        // Handle bozo MIDI files which contain empty track chunks
		if( !m_Tracks[i].dwTrackLength ) {
			m_Tracks[i].fdwTrack |= ITS_F_ENDOFTRK;
			continue;
		}

		// We always preread the time from each track so the mixer code can
		// determine which track has the next event with a minimum of work
		if( !GetTrackVDWord( &m_Tracks[i], &m_Tracks[i].tkNextEventDue )) {
			//TRACE0("Error in MIDI data\n");
			//ASSERT(FALSE);
			return FALSE;
		}
	}


	m_pSoundData = pSoundData;
	m_dwSoundSize = dwSize;
	m_pWndParent = &pWndParent;

	// allocate volume channels and initialise them
	m_Volumes.resize(NUM_CHANNELS, VOLUME_INIT);

	if( ! StreamBufferSetup() ) {
		//ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


BOOL CMIDI :: Play(BOOL bInfinite /* = FALSE */) {
	if( IsPaused() ) {
		Continue();
		return TRUE;
	}

	// calling Play() while it is already playing will restart from scratch
	if( IsPlaying() )
		Stop();

	// Clear the status of our callback so it will handle
	// MOM_DONE callbacks once more
	m_uCallbackStatus = 0;

	if( !m_bLooped )
		m_bInsertTempo = TRUE;

	MMRESULT mmResult;
	if( (mmResult = midiStreamRestart(m_hStream)) != MMSYSERR_NOERROR ) {
		MidiError(mmResult);
		return FALSE;
	}

	m_bPlaying = TRUE;
	m_bLooped = bInfinite;

	return m_bPlaying;
}


BOOL CMIDI :: Stop(BOOL bReOpen /*=TRUE*/) {
	MMRESULT mmrRetVal;

	if( IsPlaying()	|| (m_uCallbackStatus != STATUS_CALLBACKDEAD) ) {
		m_bPlaying = m_bPaused = FALSE;
		if( m_uCallbackStatus != STATUS_CALLBACKDEAD && m_uCallbackStatus != STATUS_WAITINGFOREND )
			m_uCallbackStatus = STATUS_KILLCALLBACK;

		if( (mmrRetVal = midiStreamStop(m_hStream) ) != MMSYSERR_NOERROR ) {
			MidiError(mmrRetVal);
			return FALSE;
		}
		if( (mmrRetVal = midiOutReset((HMIDIOUT)m_hStream)) != MMSYSERR_NOERROR ) {
			MidiError(mmrRetVal);
			return FALSE;
		}
		// Wait for the callback thread to release this thread, which it will do by
		// calling SetEvent() once all buffers are returned to it
		if( WaitForSingleObject( m_hBufferReturnEvent, DEBUG_CALLBACK_TIMEOUT ) == WAIT_TIMEOUT ) {
			// Note, this is a risky move because the callback may be genuinely busy, but
			// when we're debugging, it's safer and faster than freezing the application,
			// which leaves the MIDI device locked up and forces a system reset...
			//TRACE0("Timed out waiting for MIDI callback\n");
			m_uCallbackStatus = STATUS_CALLBACKDEAD;
		}
	}

	if( m_uCallbackStatus == STATUS_CALLBACKDEAD ) {
		m_uCallbackStatus = 0;
		FreeBuffers();
		if( m_hStream ) {
			if( (mmrRetVal = midiStreamClose(m_hStream) ) != MMSYSERR_NOERROR ) {
				MidiError(mmrRetVal);
			}
			m_hStream = 0;
		}

		if( bReOpen ) {
			if( !StreamBufferSetup() ) {
				// Error setting up for MIDI file
				// Notification is already taken care of...
				return FALSE;
			}
			if( ! m_bLooped ) {
				Rewind();
				m_dwProgressBytes = 0;
				m_dwStatus = 0;
			}
		}
	}
	return TRUE;
}


BOOL CMIDI :: Pause() {
	if( ! m_bPaused && m_bPlaying && m_pSoundData && m_hStream ) {
		midiStreamPause(m_hStream);
		m_bPaused = TRUE;
	}
	return FALSE;
}


BOOL CMIDI :: Continue() {
	if( m_bPaused && m_bPlaying && m_pSoundData && m_hStream ) {
		midiStreamRestart(m_hStream);
		m_bPaused = FALSE;
	}
	return FALSE;
}


BOOL CMIDI :: Rewind() {
	if( ! m_pSoundData )
		return FALSE;

	for(register DWORD i = 0; i < m_dwTrackCount; ++i) {
		m_Tracks[i].pTrackCurrent = m_Tracks[i].pTrackStart;
		m_Tracks[i].byRunningStatus = 0;
		m_Tracks[i].tkNextEventDue = 0;
		m_Tracks[i].fdwTrack = 0;

        // Handle bozo MIDI files which contain empty track chunks
		if( !m_Tracks[i].dwTrackLength ) {
			m_Tracks[i].fdwTrack |= ITS_F_ENDOFTRK;
			continue;
		}

		// We always preread the time from each track so the mixer code can
		// determine which track has the next event with a minimum of work
		if( !GetTrackVDWord( &m_Tracks[i], &m_Tracks[i].tkNextEventDue )) {
			//TRACE0("Error in MIDI data\n");
			//ASSERT(FALSE);
			return FALSE;
		}
	}

	return TRUE;
}


DWORD CMIDI :: GetChannelCount() const {
	return m_Volumes.size();
}


void CMIDI :: SetVolume(DWORD dwPercent) {
	const DWORD dwSize = m_Volumes.size();
	for( register DWORD i = 0; i < dwSize; ++i )
		SetChannelVolume(i, dwPercent);
}


DWORD CMIDI :: GetVolume() const {
	DWORD dwVolume = 0;
	const DWORD dwSize = m_Volumes.size();
	for( register DWORD i = 0; i < dwSize; ++i )
		dwVolume += GetChannelVolume(i);

	return dwVolume / GetChannelCount();
}


void CMIDI :: SetChannelVolume(DWORD dwChannel, DWORD dwPercent) {
	//ASSERT(dwChannel < m_Volumes.size());

	if( !m_bPlaying )
		return;

	m_Volumes[dwChannel] = (dwPercent > 100) ? 100 : dwPercent;
	DWORD dwEvent = MIDI_CTRLCHANGE | dwChannel | ((DWORD)MIDICTRL_VOLUME << 8) | ((DWORD)(m_Volumes[dwChannel]*VOLUME_MAX/100) << 16);
	MMRESULT mmrRetVal;
	if(( mmrRetVal = midiOutShortMsg((HMIDIOUT)m_hStream, dwEvent)) != MMSYSERR_NOERROR ) {
		MidiError(mmrRetVal);
		return;
	}
}


DWORD CMIDI :: GetChannelVolume(DWORD dwChannel) const {
	//ASSERT(dwChannel < GetChannelCount());
	return m_Volumes[dwChannel];
}


void CMIDI :: SetTempo(DWORD dwPercent) {
	m_dwTempoMultiplier = dwPercent ? dwPercent : 1;
	m_bInsertTempo = TRUE;
}


DWORD CMIDI :: GetTempo() const {
	return m_dwTempoMultiplier;
}


void CMIDI :: SetInfinitePlay(BOOL bSet) {
	m_bLooped = bSet;
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- implementation
//////////////////////////////////////////////////////////////////////

// This function converts MIDI data from the track buffers setup by a
// previous call to ConverterInit().  It will convert data until an error is
// encountered or the output buffer has been filled with as much event data
// as possible, not to exceed dwMaxLength. This function can take a couple
// bit flags, passed through dwFlags. Information about the success/failure
// of this operation and the number of output bytes actually converted will
// be returned in the CONVERTINFO structure pointed at by lpciInfo.
int CMIDI :: ConvertToBuffer(DWORD dwFlags, CONVERTINFO * lpciInfo) {
	int	    nChkErr;

	lpciInfo->dwBytesRecorded = 0;

	if( dwFlags & CONVERTF_RESET ) {
		m_dwProgressBytes = 0;
		m_dwStatus = 0;
		memset( &m_teTemp, 0, sizeof(TEMPEVENT));
		m_ptsTrack = m_ptsFound = 0;
	}

	// If we were already done, then return with a warning...
	if( m_dwStatus & CONVERTF_STATUS_DONE ) {
		if( m_bLooped ) {
			Rewind();
			m_dwProgressBytes = 0;
			m_dwStatus = 0;
		} else
			return CONVERTERR_DONE;
	} else if( m_dwStatus & CONVERTF_STATUS_STUCK ) {
		// The caller is asking us to continue, but we're already hosed because we
		// previously identified something as corrupt, so complain louder this time.
		return( CONVERTERR_STUCK );
	} else if( m_dwStatus & CONVERTF_STATUS_GOTEVENT ) {
		// Turn off this bit flag
		m_dwStatus ^= CONVERTF_STATUS_GOTEVENT;

		// The following code for this case is duplicated from below, and is
		// designed to handle a "straggler" event, should we have one left over
		// from previous processing the last time this function was called.

		// Don't add end of track event 'til we're done
		if( m_teTemp.byShortData[0] == MIDI_META && m_teTemp.byShortData[1] == MIDI_META_EOT ) {
			if( m_dwMallocBlocks ) {
				delete [] m_teTemp.pLongData;
				--m_dwMallocBlocks;
			}
		} else if(( nChkErr = AddEventToStreamBuffer( &m_teTemp, lpciInfo )) != CONVERTERR_NOERROR ) {
			if( nChkErr == CONVERTERR_BUFFERFULL ) {
				// Do some processing and tell caller that this buffer's full
				m_dwStatus |= CONVERTF_STATUS_GOTEVENT;
				return CONVERTERR_NOERROR;
			} else if( nChkErr == CONVERTERR_METASKIP ) {
				// We skip by all meta events that aren't tempo changes...
			} else {
				//TRACE0("Unable to add event to stream buffer.\n");
				if( m_dwMallocBlocks ) {
					delete [] m_teTemp.pLongData;
					m_dwMallocBlocks--;
				}
				return( TRUE );
			}
		}
	}

	for(;;) {
		m_ptsFound = 0;
		m_tkNext = 0xFFFFFFFFL;
		// Find nearest event due
		for( register DWORD idx = 0; idx < m_Tracks.size(); ++idx ) {
			m_ptsTrack = &m_Tracks[idx];
			if( !(m_ptsTrack->fdwTrack & ITS_F_ENDOFTRK) && (m_ptsTrack->tkNextEventDue < m_tkNext) ) {
				m_tkNext = m_ptsTrack->tkNextEventDue;
				m_ptsFound = m_ptsTrack;
			}
		}

		// None found?  We must be done, so return to the caller with a smile.
		if( !m_ptsFound ) {
			m_dwStatus |= CONVERTF_STATUS_DONE;
			// Need to set return buffer members properly
			return CONVERTERR_NOERROR;
		}

		// Ok, get the event header from that track
		if( !GetTrackEvent( m_ptsFound, &m_teTemp )) {
			// Warn future calls that this converter is stuck at a corrupt spot
			// and can't continue
			m_dwStatus |= CONVERTF_STATUS_STUCK;
			return CONVERTERR_CORRUPT;
		}

		// Don't add end of track event 'til we're done
		if( m_teTemp.byShortData[0] == MIDI_META && m_teTemp.byShortData[1] == MIDI_META_EOT ) {
			if( m_dwMallocBlocks ) {
				delete [] m_teTemp.pLongData;
				--m_dwMallocBlocks;
			}
			continue;
		}

		if(( nChkErr = AddEventToStreamBuffer( &m_teTemp, lpciInfo )) != CONVERTERR_NOERROR ) {
			if( nChkErr == CONVERTERR_BUFFERFULL ) {
				// Do some processing and tell somebody this buffer is full...
				m_dwStatus |= CONVERTF_STATUS_GOTEVENT;
				return CONVERTERR_NOERROR;
			} else if( nChkErr == CONVERTERR_METASKIP ) {
				// We skip by all meta events that aren't tempo changes...
			} else {
				//TRACE0("Unable to add event to stream buffer.\n");
				if( m_dwMallocBlocks ) {
					delete [] m_teTemp.pLongData;
					m_dwMallocBlocks--;
				}
				return TRUE;
			}
		}
	}	

	return CONVERTERR_NOERROR;
}

// GetTrackEvent
//
// Fills in the event struct with the next event from the track
//
// pteTemp->tkEvent will contain the absolute tick time of the event
// pteTemp->byShortData[0] will contain
//  MIDI_META if the event is a meta event;
//   in this case pteTemp->byShortData[1] will contain the meta class
//  MIDI_SYSEX or MIDI_SYSEXEND if the event is a SysEx event
//  Otherwise, the event is a channel message and pteTemp->byShortData[1]
//   and pteTemp->byShortData[2] will contain the rest of the event.
//
// pteTemp->dwEventLength will contain
//  The total length of the channel message in pteTemp->byShortData if
//   the event is a channel message
//  The total length of the paramter data pointed to by
//   pteTemp->pLongData otherwise
//
// pteTemp->pLongData will point at any additional paramters if the 
//  event is a SysEx or meta event with non-zero length; else
//  it will contain NULL
//
// Returns TRUE on success or FALSE on any kind of parse error
// Prints its own error message ONLY in the debug version
//
// Maintains the state of the input track (i.e. 
// ptsTrack->pTrackPointers, and ptsTrack->byRunningStatus).
//
BOOL CMIDI :: GetTrackEvent(TRACK * ptsTrack, TEMPEVENT * pteTemp) {
	DWORD   idx;
	UINT    dwEventLength;

	// Clear out the temporary event structure to get rid of old data...
	memset( pteTemp, 0, sizeof(TEMPEVENT));

	// Already at end of track? There's nothing to read.
    if( ptsTrack->fdwTrack & ITS_F_ENDOFTRK )
		return FALSE;

	// Get the first byte, which determines the type of event.
	BYTE byByte;
	if( !GetTrackByte(ptsTrack, &byByte) )
		return FALSE;

	// If the high bit is not set, then this is a channel message
	// which uses the status byte from the last channel message
	// we saw. NOTE: We do not clear running status across SysEx or
	// meta events even though the spec says to because there are
	// actually files out there which contain that sequence of data.
	if( !(byByte & 0x80) ) {
		// No previous status byte? We're hosed.
		if( !ptsTrack->byRunningStatus ) {
			TrackError(ptsTrack, gteBadRunStat);
			return FALSE;
		}

		pteTemp->byShortData[0] = ptsTrack->byRunningStatus;
		pteTemp->byShortData[1] = byByte;

		byByte = pteTemp->byShortData[0] & 0xF0;
		pteTemp->dwEventLength = 2;

		// Only program change and channel pressure events are 2 bytes long;
		// the rest are 3 and need another byte
		if(( byByte != MIDI_PRGMCHANGE ) && ( byByte != MIDI_CHANPRESS )) {
			if( !GetTrackByte( ptsTrack, &pteTemp->byShortData[2] ))
				return FALSE;
			++pteTemp->dwEventLength;
		}
	} else if(( byByte & 0xF0 ) != MIDI_SYSEX ) {
		// Not running status, not in SysEx range - must be
		// normal channel message (0x80-0xEF)
		pteTemp->byShortData[0] = byByte;
		ptsTrack->byRunningStatus = byByte;

		// Strip off channel and just keep message type
		byByte &= 0xF0;

		dwEventLength = ( byByte == MIDI_PRGMCHANGE || byByte == MIDI_CHANPRESS ) ? 1 : 2;
		pteTemp->dwEventLength = dwEventLength + 1;

		if( !GetTrackByte( ptsTrack, &pteTemp->byShortData[1] ))
			return FALSE;
		if( dwEventLength == 2 )
			if( !GetTrackByte( ptsTrack, &pteTemp->byShortData[2] ))
				return FALSE;
	} else if(( byByte == MIDI_SYSEX ) || ( byByte == MIDI_SYSEXEND )) {
		// One of the SysEx types. (They are the same as far as we're concerned;
		// there is only a semantic difference in how the data would actually
		// get sent when the file is played. We must take care to put the proper
		// event type back on the output track, however.)
		//
		// Parse the general format of:
		//  BYTE 	bEvent (MIDI_SYSEX or MIDI_SYSEXEND)
		//  VDWORD 	cbParms
		//  BYTE   	abParms[cbParms]
		pteTemp->byShortData[0] = byByte;
		if( !GetTrackVDWord( ptsTrack, &pteTemp->dwEventLength )) {
			TrackError( ptsTrack, gteSysExLenTrunc );
			return FALSE;
		}

		// Malloc a temporary memory block to hold the parameter data
		pteTemp->pLongData = new BYTE [pteTemp->dwEventLength];
		if( pteTemp->pLongData == 0 ) {
			TrackError( ptsTrack, gteNoMem );
			return FALSE;
		}
		// Increment our counter, which tells the program to look around for
		// a malloc block to free, should it need to exit or reset before the
		// block would normally be freed
		++m_dwMallocBlocks;

		// Copy from the input buffer to the parameter data buffer
		for( idx = 0; idx < pteTemp->dwEventLength; idx++ )
			if( !GetTrackByte( ptsTrack, pteTemp->pLongData + idx )) {
				TrackError( ptsTrack, gteSysExTrunc );
				return FALSE;
			}
	} else if( byByte == MIDI_META ) {
		// It's a meta event. Parse the general form:
		//  BYTE	bEvent	(MIDI_META)
		//  BYTE	bClass
		//  VDWORD	cbParms
		//  BYTE	abParms[cbParms]
		pteTemp->byShortData[0] = byByte;

		if( !GetTrackByte( ptsTrack, &pteTemp->byShortData[1] ))
			return FALSE;

		if( !GetTrackVDWord( ptsTrack, &pteTemp->dwEventLength )) {	
			TrackError( ptsTrack, gteMetaLenTrunc );
			return FALSE;
		}

		// NOTE: It's perfectly valid to have a meta with no data
		// In this case, dwEventLength == 0 and pLongData == NULL
		if( pteTemp->dwEventLength ) {		
			// Malloc a temporary memory block to hold the parameter data
			pteTemp->pLongData = new BYTE [pteTemp->dwEventLength];
			if( pteTemp->pLongData == 0 ) {
				TrackError( ptsTrack, gteNoMem );
				return FALSE;
			}
			// Increment our counter, which tells the program to look around for
			// a malloc block to free, should it need to exit or reset before the
			// block would normally be freed
			++m_dwMallocBlocks;

			// Copy from the input buffer to the parameter data buffer
			for( idx = 0; idx < pteTemp->dwEventLength; idx++ )
				if( !GetTrackByte( ptsTrack, pteTemp->pLongData + idx )) {
					TrackError( ptsTrack, gteMetaTrunc );
					return FALSE;
				}
		}

		if( pteTemp->byShortData[1] == MIDI_META_EOT )
			ptsTrack->fdwTrack |= ITS_F_ENDOFTRK;
	} else {
		// Messages in this range are system messages and aren't supposed to
		// be in a normal MIDI file. If they are, we've either misparsed or the
		// authoring software is stupid.
		return FALSE;
	}

	// Event time was already stored as the current track time
	pteTemp->tkEvent = ptsTrack->tkNextEventDue;

	// Now update to the next event time. The code above MUST properly
	// maintain the end of track flag in case the end of track meta is
	// missing.  NOTE: This code is a continuation of the track event
	// time pre-read which is done at the end of track initialization.
	if( !( ptsTrack->fdwTrack & ITS_F_ENDOFTRK )) {
		DWORD	tkDelta;

		if( !GetTrackVDWord( ptsTrack, &tkDelta ))
			return FALSE;

		ptsTrack->tkNextEventDue += tkDelta;
	}

	return TRUE;
}


// GetTrackVDWord
//
// Attempts to parse a variable length DWORD from the given track. A VDWord
// in a MIDI file
//  (a) is in lo-hi format 
//  (b) has the high bit set on every byte except the last
//
// Returns the DWORD in *lpdw and TRUE on success; else
// FALSE if we hit end of track first.
BOOL CMIDI :: GetTrackVDWord(TRACK * ptsTrack, LPDWORD lpdw) {
	//ASSERT(ptsTrack != 0);
	//ASSERT(lpdw != 0);

	if( ptsTrack->fdwTrack & ITS_F_ENDOFTRK )
		return FALSE;

	BYTE	byByte;
	DWORD	dw = 0;

	do {
		if( !GetTrackByte( ptsTrack, &byByte ))
			return FALSE;

		dw = ( dw << 7 ) | ( byByte & 0x7F );
	} while( byByte & 0x80 );

	*lpdw = dw;

	return TRUE;
}


// AddEventToStreamBuffer
//
// Put the given event into the given stream buffer at the given location
// pteTemp must point to an event filled out in accordance with the
// description given in GetTrackEvent
//
// Handles its own error notification by displaying to the appropriate
// output device (either our debugging window, or the screen).
int CMIDI :: AddEventToStreamBuffer( TEMPEVENT * pteTemp, CONVERTINFO *lpciInfo ) {
	MIDIEVENT * pmeEvent = (MIDIEVENT *)( lpciInfo->mhBuffer.lpData
							+ lpciInfo->dwStartOffset
							+ lpciInfo->dwBytesRecorded );

	// When we see a new, empty buffer, set the start time on it...
	if( !lpciInfo->dwBytesRecorded )
		lpciInfo->tkStart = m_tkCurrentTime;

	// Use the above set start time to figure out how much longer we should fill
	// this buffer before officially declaring it as "full"
	if( m_tkCurrentTime - lpciInfo->tkStart > m_dwBufferTickLength )
		if( lpciInfo->bTimesUp ) {
			lpciInfo->bTimesUp = FALSE;
			return CONVERTERR_BUFFERFULL;
		} else
			lpciInfo->bTimesUp = TRUE;

	DWORD tkNow = m_tkCurrentTime;

	// Delta time is absolute event time minus absolute time
	// already gone by on this track
	DWORD tkDelta = pteTemp->tkEvent - m_tkCurrentTime;

	// Event time is now current time on this track
	m_tkCurrentTime = pteTemp->tkEvent;

	if( m_bInsertTempo ) {
		m_bInsertTempo = FALSE;

		if( lpciInfo->dwMaxLength-lpciInfo->dwBytesRecorded < 3*sizeof(DWORD)) {
			// Cleanup from our write operation
			return CONVERTERR_BUFFERFULL;
		}
		if( m_dwCurrentTempo ) {
			pmeEvent->dwDeltaTime = 0;
			pmeEvent->dwStreamID = 0;
			pmeEvent->dwEvent = ( m_dwCurrentTempo * 100 ) / m_dwTempoMultiplier;
			pmeEvent->dwEvent |= (((DWORD)MEVT_TEMPO ) << 24 ) | MEVT_F_SHORT;

			lpciInfo->dwBytesRecorded += 3 * sizeof(DWORD);
			pmeEvent += 3 * sizeof(DWORD);
		}
	}

	if( pteTemp->byShortData[0] < MIDI_SYSEX ) {
		// Channel message. We know how long it is, just copy it.
		// Need 3 DWORD's: delta-t, stream-ID, event
		if( lpciInfo->dwMaxLength-lpciInfo->dwBytesRecorded < 3*sizeof(DWORD)) {
			// Cleanup from our write operation
			return CONVERTERR_BUFFERFULL;
		}

		pmeEvent->dwDeltaTime = tkDelta;
		pmeEvent->dwStreamID = 0;
		pmeEvent->dwEvent = ( pteTemp->byShortData[0] )
					| (((DWORD)pteTemp->byShortData[1] ) << 8 )
					| (((DWORD)pteTemp->byShortData[2] ) << 16 )
					| MEVT_F_SHORT;

		if((( pteTemp->byShortData[0] & 0xF0) == MIDI_CTRLCHANGE ) && ( pteTemp->byShortData[1] == MIDICTRL_VOLUME )) {
			// If this is a volume change, generate a callback so we can grab
			// the new volume for our cache
			pmeEvent->dwEvent |= MEVT_F_CALLBACK;
		}
		lpciInfo->dwBytesRecorded += 3 *sizeof(DWORD);
	} else if(( pteTemp->byShortData[0] == MIDI_SYSEX ) || ( pteTemp->byShortData[0] == MIDI_SYSEXEND )) {
		//TRACE0("AddEventToStreamBuffer: Ignoring SysEx event.\n");
		if( m_dwMallocBlocks ) {
			delete [] pteTemp->pLongData;
			--m_dwMallocBlocks;
		}
	} else {
		// Better be a meta event.
		//  BYTE	byEvent
		//  BYTE	byEventType
		//  VDWORD	dwEventLength
		//  BYTE	pLongEventData[dwEventLength]
		//ASSERT( pteTemp->byShortData[0] == MIDI_META );

		// The only meta-event we care about is change tempo
		if( pteTemp->byShortData[1] != MIDI_META_TEMPO ) {
			if( m_dwMallocBlocks ) {
				delete [] pteTemp->pLongData;
				--m_dwMallocBlocks;
			}
			return CONVERTERR_METASKIP;
		}

		// We should have three bytes of parameter data...
		//ASSERT(pteTemp->dwEventLength == 3);

		// Need 3 DWORD's: delta-t, stream-ID, event data
		if( lpciInfo->dwMaxLength - lpciInfo->dwBytesRecorded < 3 *sizeof(DWORD)) {
			// Cleanup the temporary event if necessary and return
			if( m_dwMallocBlocks ) {
				delete [] pteTemp->pLongData;
				--m_dwMallocBlocks;
			}
			return CONVERTERR_BUFFERFULL;
		}

		pmeEvent->dwDeltaTime = tkDelta;
		pmeEvent->dwStreamID = 0;
	// Note: this is backwards from above because we're converting a single
	//		 data value from hi-lo to lo-hi format...
		pmeEvent->dwEvent = ( pteTemp->pLongData[2] )
				| (((DWORD)pteTemp->pLongData[1] ) << 8 )
				| (((DWORD)pteTemp->pLongData[0] ) << 16 );

		// This next step has absolutely nothing to do with the conversion of a
		// MIDI file to a stream, it's simply put here to add the functionality
		// of the tempo slider. If you don't need this, be sure to remove the
		// next two lines.
		m_dwCurrentTempo = pmeEvent->dwEvent;
		pmeEvent->dwEvent = (pmeEvent->dwEvent * 100 ) / m_dwTempoMultiplier;

		pmeEvent->dwEvent |= (((DWORD)MEVT_TEMPO ) << 24 ) | MEVT_F_SHORT;

		m_dwBufferTickLength = (m_dwTimeDivision * 1000 * BUFFER_TIME_LENGTH) / m_dwCurrentTempo;
		//TRACE1("m_dwBufferTickLength = %lu\n", m_dwBufferTickLength);

		if( m_dwMallocBlocks ) {
			delete [] pteTemp->pLongData;
			--m_dwMallocBlocks;
		}
		lpciInfo->dwBytesRecorded += 3 *sizeof(DWORD);
	}

	return CONVERTERR_NOERROR;
}


// StreamBufferSetup()
//
// Opens a MIDI stream. Then it goes about converting the data into a midiStream buffer for playback.
BOOL CMIDI :: StreamBufferSetup() {
	int		nChkErr;
	BOOL	bFoundEnd = FALSE;

	MMRESULT		mmrRetVal;

	if( !m_hStream )
		if(( mmrRetVal = midiStreamOpen( &m_hStream,
					&m_uMIDIDeviceID,
					uintptr_t(1), uintptr_t(MidiProc),
					uintptr_t(this),
					CALLBACK_FUNCTION )) != MMSYSERR_NOERROR ) {
		MidiError(mmrRetVal);
		return FALSE;
	}

	// allocate stream buffers and initialise them
	m_StreamBuffers.resize(NUM_STREAM_BUFFERS);

	MIDIPROPTIMEDIV mptd;
	mptd.cbStruct = sizeof(mptd);
	mptd.dwTimeDiv = m_dwTimeDivision;
	if(( mmrRetVal = midiStreamProperty( m_hStream, (LPBYTE)&mptd,
					    MIDIPROP_SET | MIDIPROP_TIMEDIV )) != MMSYSERR_NOERROR ) {
		MidiError( mmrRetVal );
		return FALSE;
	}

	m_nEmptyBuffers = 0;
	DWORD dwConvertFlag = CONVERTF_RESET;

	for( m_nCurrentBuffer = 0; m_nCurrentBuffer < NUM_STREAM_BUFFERS; m_nCurrentBuffer++ ) {
		m_StreamBuffers[m_nCurrentBuffer].mhBuffer.dwBufferLength = OUT_BUFFER_SIZE;
		m_StreamBuffers[m_nCurrentBuffer].mhBuffer.lpData = new char [OUT_BUFFER_SIZE];
		if( m_StreamBuffers[m_nCurrentBuffer].mhBuffer.lpData == 0 )
			return FALSE;

		// Tell the converter to convert up to one entire buffer's length of output
		// data. Also, set a flag so it knows to reset any saved state variables it
		// may keep from call to call.
		m_StreamBuffers[m_nCurrentBuffer].dwStartOffset = 0;
		m_StreamBuffers[m_nCurrentBuffer].dwMaxLength = OUT_BUFFER_SIZE;
		m_StreamBuffers[m_nCurrentBuffer].tkStart = 0;
		m_StreamBuffers[m_nCurrentBuffer].bTimesUp = FALSE;

		if(( nChkErr = ConvertToBuffer( dwConvertFlag, &m_StreamBuffers[m_nCurrentBuffer] )) != CONVERTERR_NOERROR ) {
			if( nChkErr == CONVERTERR_DONE ) {
				bFoundEnd = TRUE;
			} else {
				//TRACE0("Initial conversion pass failed\n");
				return FALSE;
			}
		}
		m_StreamBuffers[m_nCurrentBuffer].mhBuffer.dwBytesRecorded = m_StreamBuffers[m_nCurrentBuffer].dwBytesRecorded;

		if( !m_bBuffersPrepared )
			if(( mmrRetVal = midiOutPrepareHeader( (HMIDIOUT)m_hStream,
						&m_StreamBuffers[m_nCurrentBuffer].mhBuffer,
						sizeof(MIDIHDR))) != MMSYSERR_NOERROR ) {
				MidiError( mmrRetVal );
				return FALSE;
			}

		if(( mmrRetVal = midiStreamOut( m_hStream,
						&m_StreamBuffers[m_nCurrentBuffer].mhBuffer,
						sizeof(MIDIHDR))) != MMSYSERR_NOERROR ) {
			MidiError(mmrRetVal);
			break;
		}
		dwConvertFlag = 0;

		if( bFoundEnd )
			break;
	}

	m_bBuffersPrepared = TRUE;
	m_nCurrentBuffer = 0;
	return TRUE;
}

// This function unprepares and frees all our buffers -- something we must
// do to work around a bug in MMYSYSTEM that prevents a device from playing
// back properly unless it is closed and reopened after each stop.
void CMIDI :: FreeBuffers() {
	DWORD	idx;
	MMRESULT	mmrRetVal;

	if( m_bBuffersPrepared ) {
		for( idx = 0; idx < NUM_STREAM_BUFFERS; idx++ )
			if(( mmrRetVal = midiOutUnprepareHeader( (HMIDIOUT)m_hStream,
								&m_StreamBuffers[idx].mhBuffer,
								sizeof(MIDIHDR))) != MMSYSERR_NOERROR ) {
				MidiError(mmrRetVal);
			}
		m_bBuffersPrepared = FALSE;
	}
	// Free our stream buffers...
	for( idx = 0; idx < NUM_STREAM_BUFFERS; idx++ )
	if( m_StreamBuffers[idx].mhBuffer.lpData ) {
		delete [] m_StreamBuffers[idx].mhBuffer.lpData;
		m_StreamBuffers[idx].mhBuffer.lpData = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- error handling
//////////////////////////////////////////////////////////////////////

void CMIDI :: MidiError(MMRESULT mmResult) {
	#ifdef _DEBUG
		char chText[512];
		midiOutGetErrorText(mmResult, chText, sizeof(chText));
		//TRACE1("Midi error: %hs\n", chText);
	#endif
}


void CMIDI :: TrackError(TRACK * ptsTrack, LPSTR lpszErr ) {
	//TRACE1("Track buffer offset %lu\n", (DWORD)(ptsTrack->pTrackCurrent - ptsTrack->pTrackStart));
    //TRACE1("Track total length %lu\n", ptsTrack->dwTrackLength);
	//TRACE1("%hs\n", lpszErr);
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- overridables
//////////////////////////////////////////////////////////////////////

void CMIDI :: OnMidiOutOpen() {
}


void CMIDI :: OnMidiOutDone(MIDIHDR & rHdr) {
	if( m_uCallbackStatus == STATUS_CALLBACKDEAD )
		return;

	++m_nEmptyBuffers;

	if( m_uCallbackStatus == STATUS_WAITINGFOREND ) {
		if( m_nEmptyBuffers < NUM_STREAM_BUFFERS )
			return;
		else {
			m_uCallbackStatus = STATUS_CALLBACKDEAD;
			Stop();
			SetEvent(m_hBufferReturnEvent);
			return;
		}
	}

	// This flag is set whenever the callback is waiting for all buffers to
	// come back.
	if( m_uCallbackStatus == STATUS_KILLCALLBACK ) {
		// Count NUM_STREAM_BUFFERS-1 being returned for the last time
		if( m_nEmptyBuffers < NUM_STREAM_BUFFERS )
			return;
		else {
			// Change the status to callback dead
			m_uCallbackStatus = STATUS_CALLBACKDEAD;
			SetEvent(m_hBufferReturnEvent);
			return;
		}
	}

	m_dwProgressBytes += m_StreamBuffers[m_nCurrentBuffer].mhBuffer.dwBytesRecorded;

	///////////////////////////////////////////////////////////////////////////////
	// Fill an available buffer with audio data again...

	if( m_bPlaying && m_nEmptyBuffers ) {
		m_StreamBuffers[m_nCurrentBuffer].dwStartOffset = 0;
		m_StreamBuffers[m_nCurrentBuffer].dwMaxLength = OUT_BUFFER_SIZE;
		m_StreamBuffers[m_nCurrentBuffer].tkStart = 0;
		m_StreamBuffers[m_nCurrentBuffer].dwBytesRecorded = 0;
		m_StreamBuffers[m_nCurrentBuffer].bTimesUp = FALSE;

		int nChkErr;

		if(( nChkErr = ConvertToBuffer( 0, &m_StreamBuffers[m_nCurrentBuffer] )) != CONVERTERR_NOERROR ) {
			if( nChkErr == CONVERTERR_DONE ) {
				m_uCallbackStatus = STATUS_WAITINGFOREND;
				return;
			} else {
				//TRACE0("MidiProc() conversion pass failed!\n");
				return;
			}
		}

		m_StreamBuffers[m_nCurrentBuffer].mhBuffer.dwBytesRecorded = m_StreamBuffers[m_nCurrentBuffer].dwBytesRecorded;

		MMRESULT mmrRetVal;
		if( (mmrRetVal = midiStreamOut(m_hStream, &m_StreamBuffers[m_nCurrentBuffer].mhBuffer, sizeof(MIDIHDR))) != MMSYSERR_NOERROR ) {
			MidiError(mmrRetVal);
			return;
		}
		m_nCurrentBuffer = ( m_nCurrentBuffer + 1 ) % NUM_STREAM_BUFFERS;
		m_nEmptyBuffers--;
	}
}


void CMIDI :: OnMidiOutPositionCB(MIDIHDR & rHdr, MIDIEVENT & rEvent) {
	if( MIDIEVENT_TYPE(rEvent.dwEvent) == MIDI_CTRLCHANGE )
	{
		if( MIDIEVENT_DATA1(rEvent.dwEvent) == MIDICTRL_VOLUME ) {
			// Mask off the channel number and cache the volume data byte
			m_Volumes[MIDIEVENT_CHANNEL(rEvent.dwEvent)] = DWORD(MIDIEVENT_VOLUME(rEvent.dwEvent)*100/VOLUME_MAX);
			if( m_pWndParent/* && ::IsWindow(m_pWndParent->GetSafeHwnd())*/)
				// Do not use SendMessage(), because a change of the midi stream has no effect
				// during callback handling, so if the owner wants to adjust the volume, as a
				// result of the windows message, (s)he will not hear that change.
				PostMessage(*m_pWndParent,
					WM_MIDI_VOLUMECHANGED,
					WPARAM(this),
					LPARAM(
						MAKELONG(
							WORD(MIDIEVENT_CHANNEL(rEvent.dwEvent)),
							WORD(MIDIEVENT_VOLUME(rEvent.dwEvent)*100/VOLUME_MAX)
						)
					)
				);
		}
	}
}


void CMIDI :: OnMidiOutClose() {
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- static members
//////////////////////////////////////////////////////////////////////

void CMIDI :: MidiProc(HMIDIOUT hMidi, UINT uMsg, uintptr_t dwInstanceData, uintptr_t dwParam1, uintptr_t dwParam2) {
	CMIDI * pMidi = (CMIDI *) dwInstanceData;
	//ASSERT(pMidi != 0);
	MIDIHDR * pHdr = (MIDIHDR*) dwParam1;

	switch(uMsg) {
		case MOM_OPEN:
			pMidi->OnMidiOutOpen();
			break;

		case MOM_CLOSE:
			pMidi->OnMidiOutClose();
			break;

		case MOM_DONE:
			//ASSERT(pHdr != 0);
			pMidi->OnMidiOutDone(*pHdr);
			break;

		case MOM_POSITIONCB:
			//ASSERT(pHdr != 0);
			pMidi->OnMidiOutPositionCB(*pHdr, *((MIDIEVENT*)(pHdr->lpData + pHdr->dwOffset)));
			break;

		default:
			break;
	}
}