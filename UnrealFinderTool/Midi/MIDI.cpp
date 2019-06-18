#include "pch.h"
#include "MIDI.h"

#pragma comment(lib, "winmm")

#define M_THD		0x6468544D		// Start of file
#define M_TRK		0x6B72544D		// Start of track

#define BUFFER_TIME_LENGTH		60   // Amount to fill in milliseconds

// These structures are stored in MIDI files; they need to be byte aligned.
//
#pragma pack(1)

// Contents of MThd chunk.
struct MidiFileHdr
{
	WORD wFormat; // Format (hi-lo)
	WORD wTrackCount; // # tracks (hi-lo)
	WORD wTimeDivision; // Time division (hi-lo)
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


static char GteBadRunStat[] = "Reference to missing running status.";
//static char GteRunStatMsgTrunc[] = "Running status message truncated";
//static char GteChanMsgTrunc[] = "Channel message truncated";
static char GteSysExLenTrunc[] = "SysEx event truncated (length)";
static char GteSysExTrunc[] = "SysEx event truncated";
//static char GteMetaNoClass[] = "Meta event truncated (no class byte)";
static char GteMetaLenTrunc[] = "Meta event truncated (length)";
static char GteMetaTrunc[] = "Meta event truncated";
static char GteNoMem[] = "Out of memory during malloc call";


//////////////////////////////////////////////////////////////////////
// CMIDI -- Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMIDI::CMIDI()
	: mDwSoundSize(0)
	  , mPSoundData(nullptr)
	  , mDwFormat(0)
	  , mDwTrackCount(0)
	  , mDwTimeDivision(0)
	  , mBPlaying(FALSE)
	  , mHStream(nullptr)
	  , mDwProgressBytes(0)
	  , mBLooped(FALSE)
	  , mTkCurrentTime(0)
	  , mDwBufferTickLength(0)
	  , mDwCurrentTempo(0)
	  , mDwTempoMultiplier(100)
	  , mBInsertTempo(FALSE)
	  , mBBuffersPrepared(FALSE)
	  , mNCurrentBuffer(0)
	  , mUMidiDeviceId(MIDI_MAPPER)
	  , mNEmptyBuffers(0)
	  , mBPaused(FALSE)
	  , mUCallbackStatus(0)
	  , mHBufferReturnEvent(nullptr)

	  , mPWndParent(nullptr)
	  , mPtsTrack(nullptr)
	  , mPtsFound(nullptr)
	  , mDwStatus(0)
	  , mTkNext(0)
	  , mDwMallocBlocks(0), mTeTemp()
{
	mHBufferReturnEvent = CreateEvent(nullptr, FALSE, FALSE, TEXT("Wait For Buffer Return"));
	//ASSERT(m_hBufferReturnEvent != 0);
}

CMIDI::~CMIDI()
{
	Stop(FALSE);

	if (mHBufferReturnEvent)
		::CloseHandle(mHBufferReturnEvent);
}


BOOL CMIDI::Create(const UINT uResId, const HWND pWndParent /* = NULL */)
{
	return Create(MAKEINTRESOURCE(uResId), pWndParent);
}


BOOL CMIDI::Create(const LPCTSTR pszResId, const HWND pWndParent /* = NULL */)
{
	//////////////////////////////////////////////////////////////////
	// load resource
	HINSTANCE hApp = GetModuleHandle(nullptr);
	//ASSERT(hApp);

	HRSRC hResInfo = FindResource(hApp, pszResId, TEXT("MIDI"));
	if (hResInfo == nullptr)
		return FALSE;

	HGLOBAL hRes = LoadResource(hApp, hResInfo);
	if (hRes == nullptr)
		return FALSE;

	LPVOID pTheSound = LockResource(hRes);
	if (pTheSound == nullptr)
		return FALSE;

	DWORD dwTheSound = ::SizeofResource(hApp, hResInfo);

	return Create(static_cast<LPBYTE>(pTheSound), dwTheSound, pWndParent);
}


BOOL CMIDI::Create(const LPBYTE pSoundData, const DWORD dwSize, HWND pParent /* = NULL */)
{
	if (mPSoundData)
	{
		// already created
		//ASSERT(FALSE);
		return FALSE;
	}

	//ASSERT(pSoundData != 0);
	//ASSERT(dwSize > 0);

	auto p = LPBYTE(pSoundData);

	// check header of MIDI
	if (*reinterpret_cast<DWORD*>(p) != M_THD)
	{
		//ASSERT(FALSE);
		return FALSE;
	}
	p += sizeof(DWORD);

	// check header size
	DWORD dwHeaderSize = DWORDSWAP(*(DWORD*)p);
	if (dwHeaderSize != sizeof(MidiFileHdr))
	{
		//ASSERT(FALSE);
		return FALSE;
	}
	p += sizeof(DWORD);

	// get header
	MidiFileHdr hdr;
	CopyMemory(&hdr, p, dwHeaderSize);
	mDwFormat = DWORD(WORDSWAP(hdr.wFormat));
	mDwTrackCount = DWORD(WORDSWAP(hdr.wTrackCount));
	mDwTimeDivision = DWORD(WORDSWAP(hdr.wTimeDivision));
	p += dwHeaderSize;

	// create the array of tracks
	mTracks.resize(mDwTrackCount);
	for (DWORD i = 0; i < mDwTrackCount; ++i)
	{
		// check header of track
		if (*reinterpret_cast<DWORD*>(p) != M_TRK)
		{
			//ASSERT(FALSE);
			return FALSE;
		}
		p += sizeof(DWORD);

		mTracks[i].DwTrackLength = DWORDSWAP(*(DWORD*)p);
		p += sizeof(DWORD);

		mTracks[i].PTrackStart = mTracks[i].PTrackCurrent = p;
		p += mTracks[i].DwTrackLength;

		// Handle bozo MIDI files which contain empty track chunks
		if (!mTracks[i].DwTrackLength)
		{
			mTracks[i].FdwTrack |= ITS_F_ENDOFTRK;
			continue;
		}

		// We always preread the time from each track so the mixer code can
		// determine which track has the next event with a minimum of work
		if (!GetTrackVdWord(&mTracks[i], &mTracks[i].TkNextEventDue))
		{
			//TRACE0("Error in MIDI data\n");
			//ASSERT(FALSE);
			return FALSE;
		}
	}


	mPSoundData = pSoundData;
	mDwSoundSize = dwSize;
	mPWndParent = &pParent;

	// allocate volume channels and initialise them
	mVolumes.resize(NUM_CHANNELS, VOLUME_INIT);

	if (! StreamBufferSetup())
	{
		//ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


BOOL CMIDI::Play(BOOL bInfinite /* = FALSE */)
{
	if (IsPaused())
	{
		Continue();
		return TRUE;
	}

	// calling Play() while it is already playing will restart from scratch
	if (IsPlaying())
		Stop();

	// Clear the status of our callback so it will handle
	// MOM_DONE callbacks once more
	mUCallbackStatus = 0;

	if (!mBLooped)
		mBInsertTempo = TRUE;

	MMRESULT mmResult;
	if ((mmResult = midiStreamRestart(mHStream)) != MMSYSERR_NOERROR)
	{
		MidiError(mmResult);
		return FALSE;
	}

	mBPlaying = TRUE;
	mBLooped = bInfinite;

	return mBPlaying;
}


BOOL CMIDI::Stop(BOOL bReOpen /*=TRUE*/)
{
	MMRESULT mmrRetVal;

	if (IsPlaying() || (mUCallbackStatus != STATUS_CALLBACKDEAD))
	{
		mBPlaying = mBPaused = FALSE;
		if (mUCallbackStatus != STATUS_CALLBACKDEAD && mUCallbackStatus != STATUS_WAITINGFOREND)
			mUCallbackStatus = STATUS_KILLCALLBACK;

		if ((mmrRetVal = midiStreamStop(mHStream)) != MMSYSERR_NOERROR)
		{
			MidiError(mmrRetVal);
			return FALSE;
		}
		if ((mmrRetVal = midiOutReset(reinterpret_cast<HMIDIOUT>(mHStream))) != MMSYSERR_NOERROR)
		{
			MidiError(mmrRetVal);
			return FALSE;
		}
		// Wait for the callback thread to release this thread, which it will do by
		// calling SetEvent() once all buffers are returned to it
		if (WaitForSingleObject(mHBufferReturnEvent, DEBUG_CALLBACK_TIMEOUT) == WAIT_TIMEOUT)
		{
			// Note, this is a risky move because the callback may be genuinely busy, but
			// when we're debugging, it's safer and faster than freezing the application,
			// which leaves the MIDI device locked up and forces a system reset...
			//TRACE0("Timed out waiting for MIDI callback\n");
			mUCallbackStatus = STATUS_CALLBACKDEAD;
		}
	}

	if (mUCallbackStatus == STATUS_CALLBACKDEAD)
	{
		mUCallbackStatus = 0;
		FreeBuffers();
		if (mHStream)
		{
			if ((mmrRetVal = midiStreamClose(mHStream)) != MMSYSERR_NOERROR)
			{
				MidiError(mmrRetVal);
			}
			mHStream = nullptr;
		}

		if (bReOpen)
		{
			if (!StreamBufferSetup())
			{
				// Error setting up for MIDI file
				// Notification is already taken care of...
				return FALSE;
			}
			if (! mBLooped)
			{
				Rewind();
				mDwProgressBytes = 0;
				mDwStatus = 0;
			}
		}
	}
	return TRUE;
}


BOOL CMIDI::Pause()
{
	if (! mBPaused && mBPlaying && mPSoundData && mHStream)
	{
		midiStreamPause(mHStream);
		mBPaused = TRUE;
	}
	return FALSE;
}


BOOL CMIDI::Continue()
{
	if (mBPaused && mBPlaying && mPSoundData && mHStream)
	{
		midiStreamRestart(mHStream);
		mBPaused = FALSE;
	}
	return FALSE;
}


BOOL CMIDI::Rewind()
{
	if (! mPSoundData)
		return FALSE;

	for (DWORD i = 0; i < mDwTrackCount; ++i)
	{
		mTracks[i].PTrackCurrent = mTracks[i].PTrackStart;
		mTracks[i].ByRunningStatus = 0;
		mTracks[i].TkNextEventDue = 0;
		mTracks[i].FdwTrack = 0;

		// Handle bozo MIDI files which contain empty track chunks
		if (!mTracks[i].DwTrackLength)
		{
			mTracks[i].FdwTrack |= ITS_F_ENDOFTRK;
			continue;
		}

		// We always preread the time from each track so the mixer code can
		// determine which track has the next event with a minimum of work
		if (!GetTrackVdWord(&mTracks[i], &mTracks[i].TkNextEventDue))
		{
			//TRACE0("Error in MIDI data\n");
			//ASSERT(FALSE);
			return FALSE;
		}
	}

	return TRUE;
}


size_t CMIDI::GetChannelCount() const
{
	return mVolumes.size();
}


void CMIDI::SetVolume(const size_t percent)
{
	const size_t dwSize = mVolumes.size();
	for (size_t i = 0; i < dwSize; ++i)
		SetChannelVolume(i, percent);
}


size_t CMIDI::GetVolume() const
{
	size_t dwVolume = 0;
	const size_t dwSize = mVolumes.size();
	for (size_t i = 0; i < dwSize; ++i)
		dwVolume += GetChannelVolume(i);

	return dwVolume / GetChannelCount();
}


void CMIDI::SetChannelVolume(const size_t dwChannel, const size_t dwPercent)
{
	//ASSERT(dwChannel < m_Volumes.size());

	if (!mBPlaying)
		return;

	mVolumes[dwChannel] = (dwPercent > 100) ? 100 : dwPercent;
	size_t dwEvent = MIDI_CTRLCHANGE | dwChannel | (static_cast<DWORD>(MIDICTRL_VOLUME) << 8) | (static_cast<DWORD>(mVolumes[dwChannel] * VOLUME_MAX / 100) << 16);
	MMRESULT mmrRetVal;
	if ((mmrRetVal = midiOutShortMsg(reinterpret_cast<HMIDIOUT>(mHStream), static_cast<DWORD>(dwEvent))) != MMSYSERR_NOERROR)
	{
		MidiError(mmrRetVal);
		return;
	}
}


size_t CMIDI::GetChannelVolume(size_t dwChannel) const
{
	//ASSERT(dwChannel < GetChannelCount());
	return mVolumes[dwChannel];
}


void CMIDI::SetTempo(DWORD dwPercent)
{
	mDwTempoMultiplier = dwPercent ? dwPercent : 1;
	mBInsertTempo = TRUE;
}


DWORD CMIDI::GetTempo() const
{
	return mDwTempoMultiplier;
}


void CMIDI::SetInfinitePlay(BOOL bSet)
{
	mBLooped = bSet;
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
int CMIDI::ConvertToBuffer(const DWORD dwFlags, ConvertInfo* lpciInfo)
{
	int nChkErr;

	lpciInfo->DwBytesRecorded = 0;

	if (dwFlags & CONVERTF_RESET)
	{
		mDwProgressBytes = 0;
		mDwStatus = 0;
		memset(&mTeTemp, 0, sizeof(TempEvent));
		mPtsTrack = mPtsFound = nullptr;
	}

	// If we were already done, then return with a warning...
	if (mDwStatus & CONVERTF_STATUS_DONE)
	{
		if (mBLooped)
		{
			Rewind();
			mDwProgressBytes = 0;
			mDwStatus = 0;
		}
		else
			return CONVERTERR_DONE;
	}
	else if (mDwStatus & CONVERTF_STATUS_STUCK)
	{
		// The caller is asking us to continue, but we're already hosed because we
		// previously identified something as corrupt, so complain louder this time.
		return (CONVERTERR_STUCK);
	}
	else if (mDwStatus & CONVERTF_STATUS_GOTEVENT)
	{
		// Turn off this bit flag
		mDwStatus ^= CONVERTF_STATUS_GOTEVENT;

		// The following code for this case is duplicated from below, and is
		// designed to handle a "straggler" event, should we have one left over
		// from previous processing the last time this function was called.

		// Don't add end of track event 'til we're done
		if (mTeTemp.byShortData[0] == MIDI_META && mTeTemp.byShortData[1] == MIDI_META_EOT)
		{
			if (mDwMallocBlocks)
			{
				delete [] mTeTemp.pLongData;
				--mDwMallocBlocks;
			}
		}
		else if ((nChkErr = AddEventToStreamBuffer(&mTeTemp, lpciInfo)) != CONVERTERR_NOERROR)
		{
			if (nChkErr == CONVERTERR_BUFFERFULL)
			{
				// Do some processing and tell caller that this buffer's full
				mDwStatus |= CONVERTF_STATUS_GOTEVENT;
				return CONVERTERR_NOERROR;
			}
			else if (nChkErr == CONVERTERR_METASKIP)
			{
				// We skip by all meta events that aren't tempo changes...
			}
			else
			{
				//TRACE0("Unable to add event to stream buffer.\n");
				if (mDwMallocBlocks)
				{
					delete [] mTeTemp.pLongData;
					mDwMallocBlocks--;
				}
				return (TRUE);
			}
		}
	}

	for (;;)
	{
		mPtsFound = nullptr;
		mTkNext = 0xFFFFFFFFL;
		// Find nearest event due
		for (auto& m_track : mTracks)
		{
			mPtsTrack = &m_track;
			if (!(mPtsTrack->FdwTrack & ITS_F_ENDOFTRK) && (mPtsTrack->TkNextEventDue < mTkNext))
			{
				mTkNext = mPtsTrack->TkNextEventDue;
				mPtsFound = mPtsTrack;
			}
		}

		// None found?  We must be done, so return to the caller with a smile.
		if (!mPtsFound)
		{
			mDwStatus |= CONVERTF_STATUS_DONE;
			// Need to set return buffer members properly
			return CONVERTERR_NOERROR;
		}

		// Ok, get the event header from that track
		if (!GetTrackEvent(mPtsFound, &mTeTemp))
		{
			// Warn future calls that this converter is stuck at a corrupt spot
			// and can't continue
			mDwStatus |= CONVERTF_STATUS_STUCK;
			return CONVERTERR_CORRUPT;
		}

		// Don't add end of track event 'til we're done
		if (mTeTemp.byShortData[0] == MIDI_META && mTeTemp.byShortData[1] == MIDI_META_EOT)
		{
			if (mDwMallocBlocks)
			{
				delete [] mTeTemp.pLongData;
				--mDwMallocBlocks;
			}
			continue;
		}

		if ((nChkErr = AddEventToStreamBuffer(&mTeTemp, lpciInfo)) != CONVERTERR_NOERROR)
		{
			if (nChkErr == CONVERTERR_BUFFERFULL)
			{
				// Do some processing and tell somebody this buffer is full...
				mDwStatus |= CONVERTF_STATUS_GOTEVENT;
				return CONVERTERR_NOERROR;
			}
			else if (nChkErr == CONVERTERR_METASKIP)
			{
				// We skip by all meta events that aren't tempo changes...
			}
			else
			{
				//TRACE0("Unable to add event to stream buffer.\n");
				if (mDwMallocBlocks)
				{
					delete [] mTeTemp.pLongData;
					mDwMallocBlocks--;
				}
				return TRUE;
			}
		}
	}
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
//  The total length of the parameter data pointed to by
//   pteTemp->pLongData otherwise
//
// pteTemp->pLongData will point at any additional parameters if the 
//  event is a SysEx or meta event with non-zero length; else
//  it will contain NULL
//
// Returns TRUE on success or FALSE on any kind of parse error
// Prints its own error message ONLY in the debug version
//
// Maintains the state of the input track (i.e. 
// ptsTrack->pTrackPointers, and ptsTrack->byRunningStatus).
//
BOOL CMIDI::GetTrackEvent(Track* ptsTrack, TempEvent* pteTemp)
{
	DWORD idx;

	// Clear out the temporary event structure to get rid of old data...
	memset(pteTemp, 0, sizeof(TempEvent));

	// Already at end of track? There's nothing to read.
	if (ptsTrack->FdwTrack & ITS_F_ENDOFTRK)
		return FALSE;

	// Get the first byte, which determines the type of event.
	BYTE byByte;
	if (!GetTrackByte(ptsTrack, &byByte))
		return FALSE;

	// If the high bit is not set, then this is a channel message
	// which uses the status byte from the last channel message
	// we saw. NOTE: We do not clear running status across SysEx or
	// meta events even though the spec says to because there are
	// actually files out there which contain that sequence of data.
	if (!(byByte & 0x80))
	{
		// No previous status byte? We're hosed.
		if (!ptsTrack->ByRunningStatus)
		{
			TrackError(ptsTrack, GteBadRunStat);
			return FALSE;
		}

		pteTemp->byShortData[0] = ptsTrack->ByRunningStatus;
		pteTemp->byShortData[1] = byByte;

		byByte = pteTemp->byShortData[0] & 0xF0;
		pteTemp->dwEventLength = 2;

		// Only program change and channel pressure events are 2 bytes long;
		// the rest are 3 and need another byte
		if ((byByte != MIDI_PRGMCHANGE) && (byByte != MIDI_CHANPRESS))
		{
			if (!GetTrackByte(ptsTrack, &pteTemp->byShortData[2]))
				return FALSE;
			++pteTemp->dwEventLength;
		}
	}
	else if ((byByte & 0xF0) != MIDI_SYSEX)
	{
		// Not running status, not in SysEx range - must be
		// normal channel message (0x80-0xEF)
		pteTemp->byShortData[0] = byByte;
		ptsTrack->ByRunningStatus = byByte;

		// Strip off channel and just keep message type
		byByte &= 0xF0;

		UINT dwEventLength = (byByte == MIDI_PRGMCHANGE || byByte == MIDI_CHANPRESS) ? 1 : 2;
		pteTemp->dwEventLength = dwEventLength + 1;

		if (!GetTrackByte(ptsTrack, &pteTemp->byShortData[1]))
			return FALSE;
		if (dwEventLength == 2)
			if (!GetTrackByte(ptsTrack, &pteTemp->byShortData[2]))
				return FALSE;
	}
	else if ((byByte == MIDI_SYSEX) || (byByte == MIDI_SYSEXEND))
	{
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
		if (!GetTrackVdWord(ptsTrack, &pteTemp->dwEventLength))
		{
			TrackError(ptsTrack, GteSysExLenTrunc);
			return FALSE;
		}

		// Malloc a temporary memory block to hold the parameter data
		pteTemp->pLongData = new BYTE [pteTemp->dwEventLength];
		if (pteTemp->pLongData == nullptr)
		{
			TrackError(ptsTrack, GteNoMem);
			return FALSE;
		}
		// Increment our counter, which tells the program to look around for
		// a malloc block to free, should it need to exit or reset before the
		// block would normally be freed
		++mDwMallocBlocks;

		// Copy from the input buffer to the parameter data buffer
		for (idx = 0; idx < pteTemp->dwEventLength; idx++)
			if (!GetTrackByte(ptsTrack, pteTemp->pLongData + idx))
			{
				TrackError(ptsTrack, GteSysExTrunc);
				return FALSE;
			}
	}
	else if (byByte == MIDI_META)
	{
		// It's a meta event. Parse the general form:
		//  BYTE	bEvent	(MIDI_META)
		//  BYTE	bClass
		//  VDWORD	cbParms
		//  BYTE	abParms[cbParms]
		pteTemp->byShortData[0] = byByte;

		if (!GetTrackByte(ptsTrack, &pteTemp->byShortData[1]))
			return FALSE;

		if (!GetTrackVdWord(ptsTrack, &pteTemp->dwEventLength))
		{
			TrackError(ptsTrack, GteMetaLenTrunc);
			return FALSE;
		}

		// NOTE: It's perfectly valid to have a meta with no data
		// In this case, dwEventLength == 0 and pLongData == NULL
		if (pteTemp->dwEventLength)
		{
			// Malloc a temporary memory block to hold the parameter data
			pteTemp->pLongData = new BYTE [pteTemp->dwEventLength];
			if (pteTemp->pLongData == nullptr)
			{
				TrackError(ptsTrack, GteNoMem);
				return FALSE;
			}
			// Increment our counter, which tells the program to look around for
			// a malloc block to free, should it need to exit or reset before the
			// block would normally be freed
			++mDwMallocBlocks;

			// Copy from the input buffer to the parameter data buffer
			for (idx = 0; idx < pteTemp->dwEventLength; idx++)
				if (!GetTrackByte(ptsTrack, pteTemp->pLongData + idx))
				{
					TrackError(ptsTrack, GteMetaTrunc);
					return FALSE;
				}
		}

		if (pteTemp->byShortData[1] == MIDI_META_EOT)
			ptsTrack->FdwTrack |= ITS_F_ENDOFTRK;
	}
	else
	{
		// Messages in this range are system messages and aren't supposed to
		// be in a normal MIDI file. If they are, we've either disparaged or the
		// authoring software is stupid.
		return FALSE;
	}

	// Event time was already stored as the current track time
	pteTemp->tkEvent = ptsTrack->TkNextEventDue;

	// Now update to the next event time. The code above MUST properly
	// maintain the end of track flag in case the end of track meta is
	// missing.  NOTE: This code is a continuation of the track event
	// time pre-read which is done at the end of track initialization.
	if (!(ptsTrack->FdwTrack & ITS_F_ENDOFTRK))
	{
		DWORD tkDelta;

		if (!GetTrackVdWord(ptsTrack, &tkDelta))
			return FALSE;

		ptsTrack->TkNextEventDue += tkDelta;
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
BOOL CMIDI::GetTrackVdWord(Track* ptsTrack, const LPDWORD lpdw)
{
	//ASSERT(ptsTrack != 0);
	//ASSERT(lpdw != 0);

	if (ptsTrack->FdwTrack & ITS_F_ENDOFTRK)
		return FALSE;

	BYTE byByte;
	DWORD dw = 0;

	do
	{
		if (!GetTrackByte(ptsTrack, &byByte))
			return FALSE;

		dw = (dw << 7) | (byByte & 0x7F);
	}
	while (byByte & 0x80);

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
int CMIDI::AddEventToStreamBuffer(TempEvent* pteTemp, ConvertInfo* lpciInfo)
{
	auto* pmeEvent = reinterpret_cast<MIDIEVENT *>(lpciInfo->MhBuffer.lpData
		+ lpciInfo->DwStartOffset
		+ lpciInfo->DwBytesRecorded);

	// When we see a new, empty buffer, set the start time on it...
	if (!lpciInfo->DwBytesRecorded)
		lpciInfo->TkStart = mTkCurrentTime;

	// Use the above set start time to figure out how much longer we should fill
	// this buffer before officially declaring it as "full"
	if (mTkCurrentTime - lpciInfo->TkStart > mDwBufferTickLength)
	{
		if (lpciInfo->BTimesUp)
		{
			lpciInfo->BTimesUp = FALSE;
			return CONVERTERR_BUFFERFULL;
		}
		else
		{
			lpciInfo->BTimesUp = TRUE;
		}
	}

	// Delta time is absolute event time minus absolute time
	// already gone by on this track
	DWORD tkDelta = pteTemp->tkEvent - mTkCurrentTime;

	// Event time is now current time on this track
	mTkCurrentTime = pteTemp->tkEvent;

	if (mBInsertTempo)
	{
		mBInsertTempo = FALSE;

		if (lpciInfo->DwMaxLength - lpciInfo->DwBytesRecorded < 3 * sizeof(DWORD))
		{
			// Cleanup from our write operation
			return CONVERTERR_BUFFERFULL;
		}
		if (mDwCurrentTempo)
		{
			pmeEvent->dwDeltaTime = 0;
			pmeEvent->dwStreamID = 0;
			pmeEvent->dwEvent = (mDwCurrentTempo * 100) / mDwTempoMultiplier;
			pmeEvent->dwEvent |= (static_cast<DWORD>(MEVT_TEMPO) << 24) | MEVT_F_SHORT;

			lpciInfo->DwBytesRecorded += 3 * sizeof(DWORD);
			pmeEvent += 3 * sizeof(DWORD);
		}
	}

	if (pteTemp->byShortData[0] < MIDI_SYSEX)
	{
		// Channel message. We know how long it is, just copy it.
		// Need 3 DWORD's: delta-t, stream-ID, event
		if (lpciInfo->DwMaxLength - lpciInfo->DwBytesRecorded < 3 * sizeof(DWORD))
		{
			// Cleanup from our write operation
			return CONVERTERR_BUFFERFULL;
		}

		pmeEvent->dwDeltaTime = tkDelta;
		pmeEvent->dwStreamID = 0;
		pmeEvent->dwEvent = (pteTemp->byShortData[0])
			| (static_cast<DWORD>(pteTemp->byShortData[1]) << 8)
			| (static_cast<DWORD>(pteTemp->byShortData[2]) << 16)
			| MEVT_F_SHORT;

		if (((pteTemp->byShortData[0] & 0xF0) == MIDI_CTRLCHANGE) && (pteTemp->byShortData[1] == MIDICTRL_VOLUME))
		{
			// If this is a volume change, generate a callback so we can grab
			// the new volume for our cache
			pmeEvent->dwEvent |= MEVT_F_CALLBACK;
		}
		lpciInfo->DwBytesRecorded += 3 * sizeof(DWORD);
	}
	else if ((pteTemp->byShortData[0] == MIDI_SYSEX) || (pteTemp->byShortData[0] == MIDI_SYSEXEND))
	{
		//TRACE0("AddEventToStreamBuffer: Ignoring SysEx event.\n");
		if (mDwMallocBlocks)
		{
			delete [] pteTemp->pLongData;
			--mDwMallocBlocks;
		}
	}
	else
	{
		// Better be a meta event.
		//  BYTE	byEvent
		//  BYTE	byEventType
		//  VDWORD	dwEventLength
		//  BYTE	pLongEventData[dwEventLength]
		//ASSERT( pteTemp->byShortData[0] == MIDI_META );

		// The only meta-event we care about is change tempo
		if (pteTemp->byShortData[1] != MIDI_META_TEMPO)
		{
			if (mDwMallocBlocks)
			{
				delete [] pteTemp->pLongData;
				--mDwMallocBlocks;
			}
			return CONVERTERR_METASKIP;
		}

		// We should have three bytes of parameter data...
		//ASSERT(pteTemp->dwEventLength == 3);

		// Need 3 DWORD's: delta-t, stream-ID, event data
		if (lpciInfo->DwMaxLength - lpciInfo->DwBytesRecorded < 3 * sizeof(DWORD))
		{
			// Cleanup the temporary event if necessary and return
			if (mDwMallocBlocks)
			{
				delete [] pteTemp->pLongData;
				--mDwMallocBlocks;
			}
			return CONVERTERR_BUFFERFULL;
		}

		pmeEvent->dwDeltaTime = tkDelta;
		pmeEvent->dwStreamID = 0;
		// Note: this is backwards from above because we're converting a single
		//		 data value from hi-lo to lo-hi format...
		pmeEvent->dwEvent = (pteTemp->pLongData[2])
			| (static_cast<DWORD>(pteTemp->pLongData[1]) << 8)
			| (static_cast<DWORD>(pteTemp->pLongData[0]) << 16);

		// This next step has absolutely nothing to do with the conversion of a
		// MIDI file to a stream, it's simply put here to add the functionality
		// of the tempo slider. If you don't need this, be sure to remove the
		// next two lines.
		mDwCurrentTempo = pmeEvent->dwEvent;
		pmeEvent->dwEvent = (pmeEvent->dwEvent * 100) / mDwTempoMultiplier;

		pmeEvent->dwEvent |= (static_cast<DWORD>(MEVT_TEMPO) << 24) | MEVT_F_SHORT;

		mDwBufferTickLength = (mDwTimeDivision * 1000 * BUFFER_TIME_LENGTH) / mDwCurrentTempo;
		//TRACE1("m_dwBufferTickLength = %lu\n", m_dwBufferTickLength);

		if (mDwMallocBlocks)
		{
			delete [] pteTemp->pLongData;
			--mDwMallocBlocks;
		}
		lpciInfo->DwBytesRecorded += 3 * sizeof(DWORD);
	}

	return CONVERTERR_NOERROR;
}


// StreamBufferSetup()
//
// Opens a MIDI stream. Then it goes about converting the data into a midiStream buffer for playback.
BOOL CMIDI::StreamBufferSetup()
{
	int nChkErr;
	BOOL bFoundEnd = FALSE;

	MMRESULT mmrRetVal;

	if (!mHStream)
		if ((mmrRetVal = midiStreamOpen(&mHStream,
		                                &mUMidiDeviceId,
		                                uintptr_t(1), uintptr_t(MidiProc),
		                                uintptr_t(this),
		                                CALLBACK_FUNCTION)) != MMSYSERR_NOERROR)
		{
			MidiError(mmrRetVal);
			return FALSE;
		}

	// allocate stream buffers and initialise them
	mStreamBuffers.resize(NUM_STREAM_BUFFERS);

	MIDIPROPTIMEDIV mptd;
	mptd.cbStruct = sizeof(mptd);
	mptd.dwTimeDiv = mDwTimeDivision;
	if ((mmrRetVal = midiStreamProperty(mHStream, reinterpret_cast<LPBYTE>(&mptd),
	                                    MIDIPROP_SET | MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR)
	{
		MidiError(mmrRetVal);
		return FALSE;
	}

	mNEmptyBuffers = 0;
	DWORD dwConvertFlag = CONVERTF_RESET;

	for (mNCurrentBuffer = 0; mNCurrentBuffer < NUM_STREAM_BUFFERS; mNCurrentBuffer++)
	{
		mStreamBuffers[mNCurrentBuffer].MhBuffer.dwBufferLength = OUT_BUFFER_SIZE;
		mStreamBuffers[mNCurrentBuffer].MhBuffer.lpData = new char [OUT_BUFFER_SIZE];
		if (mStreamBuffers[mNCurrentBuffer].MhBuffer.lpData == nullptr)
			return FALSE;

		// Tell the converter to convert up to one entire buffer's length of output
		// data. Also, set a flag so it knows to reset any saved state variables it
		// may keep from call to call.
		mStreamBuffers[mNCurrentBuffer].DwStartOffset = 0;
		mStreamBuffers[mNCurrentBuffer].DwMaxLength = OUT_BUFFER_SIZE;
		mStreamBuffers[mNCurrentBuffer].TkStart = 0;
		mStreamBuffers[mNCurrentBuffer].BTimesUp = FALSE;

		if ((nChkErr = ConvertToBuffer(dwConvertFlag, &mStreamBuffers[mNCurrentBuffer])) != CONVERTERR_NOERROR)
		{
			if (nChkErr == CONVERTERR_DONE)
			{
				bFoundEnd = TRUE;
			}
			else
			{
				//TRACE0("Initial conversion pass failed\n");
				return FALSE;
			}
		}
		mStreamBuffers[mNCurrentBuffer].MhBuffer.dwBytesRecorded = mStreamBuffers[mNCurrentBuffer].DwBytesRecorded;

		if (!mBBuffersPrepared)
			if ((mmrRetVal = midiOutPrepareHeader(reinterpret_cast<HMIDIOUT>(mHStream),
			                                      &mStreamBuffers[mNCurrentBuffer].MhBuffer,
			                                      sizeof(MIDIHDR))) != MMSYSERR_NOERROR)
			{
				MidiError(mmrRetVal);
				return FALSE;
			}

		if ((mmrRetVal = midiStreamOut(mHStream,
		                               &mStreamBuffers[mNCurrentBuffer].MhBuffer,
		                               sizeof(MIDIHDR))) != MMSYSERR_NOERROR)
		{
			MidiError(mmrRetVal);
			break;
		}
		dwConvertFlag = 0;

		if (bFoundEnd)
			break;
	}

	mBBuffersPrepared = TRUE;
	mNCurrentBuffer = 0;
	return TRUE;
}

// This function unprepared and frees all our buffers -- something we must
// do to work around a bug in MMYSYSTEM that prevents a device from playing
// back properly unless it is closed and reopened after each stop.
void CMIDI::FreeBuffers()
{
	DWORD idx;
	MMRESULT mmrRetVal;

	if (mBBuffersPrepared)
	{
		for (idx = 0; idx < NUM_STREAM_BUFFERS; idx++)
			if ((mmrRetVal = midiOutUnprepareHeader(reinterpret_cast<HMIDIOUT>(mHStream),
			                                        &mStreamBuffers[idx].MhBuffer,
			                                        sizeof(MIDIHDR))) != MMSYSERR_NOERROR)
			{
				MidiError(mmrRetVal);
			}
		mBBuffersPrepared = FALSE;
	}
	// Free our stream buffers...
	for (idx = 0; idx < NUM_STREAM_BUFFERS; idx++)
		if (mStreamBuffers[idx].MhBuffer.lpData)
		{
			delete [] mStreamBuffers[idx].MhBuffer.lpData;
			mStreamBuffers[idx].MhBuffer.lpData = nullptr;
		}
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- error handling
//////////////////////////////////////////////////////////////////////

void CMIDI::MidiError(const MMRESULT mmResult)
{
#ifdef _DEBUG
	char chText[512];
	midiOutGetErrorText(mmResult, chText, sizeof(chText));
	//TRACE1("Midi error: %hs\n", chText);
#endif
}


void CMIDI::TrackError(Track* ptsTrack, LPSTR errMsg)
{
	//TRACE1("Track buffer offset %lu\n", (DWORD)(ptsTrack->pTrackCurrent - ptsTrack->pTrackStart));
	//TRACE1("Track total length %lu\n", ptsTrack->dwTrackLength);
	//TRACE1("%hs\n", lpszErr);
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- overridable
//////////////////////////////////////////////////////////////////////

void CMIDI::OnMidiOutOpen()
{
}


void CMIDI::OnMidiOutDone(MIDIHDR& rHdr)
{
	if (mUCallbackStatus == STATUS_CALLBACKDEAD)
		return;

	++mNEmptyBuffers;

	if (mUCallbackStatus == STATUS_WAITINGFOREND)
	{
		if (mNEmptyBuffers < NUM_STREAM_BUFFERS)
			return;
		else
		{
			mUCallbackStatus = STATUS_CALLBACKDEAD;
			Stop();
			SetEvent(mHBufferReturnEvent);
			return;
		}
	}

	// This flag is set whenever the callback is waiting for all buffers to
	// come back.
	if (mUCallbackStatus == STATUS_KILLCALLBACK)
	{
		// Count NUM_STREAM_BUFFERS-1 being returned for the last time
		if (mNEmptyBuffers < NUM_STREAM_BUFFERS)
			return;
		else
		{
			// Change the status to callback dead
			mUCallbackStatus = STATUS_CALLBACKDEAD;
			SetEvent(mHBufferReturnEvent);
			return;
		}
	}

	mDwProgressBytes += mStreamBuffers[mNCurrentBuffer].MhBuffer.dwBytesRecorded;

	///////////////////////////////////////////////////////////////////////////////
	// Fill an available buffer with audio data again...

	if (mBPlaying && mNEmptyBuffers)
	{
		mStreamBuffers[mNCurrentBuffer].DwStartOffset = 0;
		mStreamBuffers[mNCurrentBuffer].DwMaxLength = OUT_BUFFER_SIZE;
		mStreamBuffers[mNCurrentBuffer].TkStart = 0;
		mStreamBuffers[mNCurrentBuffer].DwBytesRecorded = 0;
		mStreamBuffers[mNCurrentBuffer].BTimesUp = FALSE;

		int nChkErr;

		if ((nChkErr = ConvertToBuffer(0, &mStreamBuffers[mNCurrentBuffer])) != CONVERTERR_NOERROR)
		{
			if (nChkErr == CONVERTERR_DONE)
			{
				mUCallbackStatus = STATUS_WAITINGFOREND;
				return;
			}
			else
			{
				//TRACE0("MidiProc() conversion pass failed!\n");
				return;
			}
		}

		mStreamBuffers[mNCurrentBuffer].MhBuffer.dwBytesRecorded = mStreamBuffers[mNCurrentBuffer].DwBytesRecorded;

		MMRESULT mmrRetVal;
		if ((mmrRetVal = midiStreamOut(mHStream, &mStreamBuffers[mNCurrentBuffer].MhBuffer, sizeof(MIDIHDR))) !=
			MMSYSERR_NOERROR)
		{
			MidiError(mmrRetVal);
			return;
		}
		mNCurrentBuffer = (mNCurrentBuffer + 1) % NUM_STREAM_BUFFERS;
		mNEmptyBuffers--;
	}
}


void CMIDI::OnMidiOutPositionCB(MIDIHDR& rHdr, MIDIEVENT& rEvent)
{
	if (MIDIEVENT_TYPE(rEvent.dwEvent) == MIDI_CTRLCHANGE)
	{
		if (MIDIEVENT_DATA1(rEvent.dwEvent) == MIDICTRL_VOLUME)
		{
			// Mask off the channel number and cache the volume data byte
			mVolumes[MIDIEVENT_CHANNEL(rEvent.dwEvent)] = DWORD(MIDIEVENT_VOLUME(rEvent.dwEvent) * 100 / VOLUME_MAX);
			if (mPWndParent/* && ::IsWindow(m_pWndParent->GetSafeHwnd())*/)
				// Do not use SendMessage(), because a change of the midi stream has no effect
				// during callback handling, so if the owner wants to adjust the volume, as a
				// result of the windows message, (s)he will not hear that change.
				PostMessage(*mPWndParent,
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


void CMIDI::OnMidiOutClose()
{
}

//////////////////////////////////////////////////////////////////////
// CMIDI -- static members
//////////////////////////////////////////////////////////////////////

void CMIDI::MidiProc(HMIDIOUT hMidi, const UINT uMsg, const uintptr_t dwInstanceData, const uintptr_t dwParam1, uintptr_t dwParam2)
{
	auto* pMidi = reinterpret_cast<CMIDI*>(dwInstanceData);
	//ASSERT(pMidi != 0);
	auto* pHdr = reinterpret_cast<MIDIHDR*>(dwParam1);

	switch (uMsg)
	{
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
		pMidi->OnMidiOutPositionCB(*pHdr, *reinterpret_cast<MIDIEVENT*>(pHdr->lpData + pHdr->dwOffset));
		break;

	default:
		break;
	}
}
