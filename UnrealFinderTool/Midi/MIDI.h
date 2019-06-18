#pragma once
#ifndef MIDI_h
#define MIDI_h

#include <mmsystem.h>
#include <vector>
using namespace std;


// This message is sent to the controlling window, if the volume changes in
// another way than explicitly set by the owner of the CMIDI object.
// WPARAM	the pointer to the MIDI object
// LPARAM	lo-word: the number of the channel that changed volume
//			hi-word: the new volume in percent
#define	WM_MIDI_VOLUMECHANGED	WM_USER+23


#define MIDI_CTRLCHANGE			((BYTE)0xB0)		// + ctrlr + value
#define MIDI_PRGMCHANGE			((BYTE)0xC0)		// + new patch
#define MIDI_CHANPRESS			((BYTE)0xD0)		// + pressure (1 byte)

#define MIDICTRL_VOLUME			((BYTE)0x07)

#define MIDIEVENT_CHANNEL(dw)	(dw & 0x0000000F)
#define MIDIEVENT_TYPE(dw)		(dw & 0x000000F0)
#define MIDIEVENT_DATA1(dw)		((dw & 0x0000FF00) >> 8)
#define MIDIEVENT_VOLUME(dw)	((dw & 0x007F0000) >> 16)

#define MIDI_SYSEX				((BYTE)0xF0)		// SysEx begin
#define MIDI_SYSEXEND			((BYTE)0xF7)		// SysEx end
#define MIDI_META				((BYTE)0xFF)		// Meta event begin
#define MIDI_META_TEMPO			((BYTE)0x51)		// Tempo change
#define MIDI_META_EOT			((BYTE)0x2F)		// End-of-track


// flags for the ConvertToBuffer() method
#define CONVERTF_RESET				0x00000001
#define CONVERTF_STATUS_DONE		0x00000001
#define CONVERTF_STATUS_STUCK		0x00000002
#define CONVERTF_STATUS_GOTEVENT	0x00000004

// Return values from the ConvertToBuffer() method
#define CONVERTERR_NOERROR		0		// No error occured
#define CONVERTERR_CORRUPT		-101	// The input file is corrupt
// The converter has already encountered a corrupt file and cannot convert any
// more of this file -- must reset the converter
#define CONVERTERR_STUCK		-102
#define CONVERTERR_DONE			-103	// Converter is done
#define CONVERTERR_BUFFERFULL	-104	// The buffer is full
#define CONVERTERR_METASKIP		-105	// Skipping unknown meta event

#define STATUS_KILLCALLBACK		100		// Signals that the callback should die
#define STATUS_CALLBACKDEAD		200		// Signals callback is done processing
#define STATUS_WAITINGFOREND	300		// Callback's waiting for buffers to play

// Description of a track
//
struct TRACK
{
	DWORD	fdwTrack;		// Track's flags
	DWORD	dwTrackLength;	// Total bytes in track
	LPBYTE	pTrackStart;	// -> start of track data buffer
	LPBYTE	pTrackCurrent;	// -> next byte to read in buffer
	DWORD	tkNextEventDue;	// Absolute time of next event in track
	BYTE	byRunningStatus;// Running status from last channel msg

	TRACK()
		: fdwTrack(0)
		, dwTrackLength(0)
		, pTrackStart(0)
		, pTrackCurrent(0)
		, tkNextEventDue(0)
		, byRunningStatus(0)
	{
	}
};

#define ITS_F_ENDOFTRK		0x00000001


// This structure is used to pass information to the ConvertToBuffer()
// system and then internally by that function to send information about the
// target stream buffer and current state of the conversion process to internal
// lower level conversion routines.
struct CONVERTINFO
{
    MIDIHDR	mhBuffer;			// Standard Windows stream buffer header
    DWORD	dwStartOffset;		// Start offset from mhStreamBuffer.lpStart
    DWORD	dwMaxLength;		// Max length to convert on this pass
    DWORD	dwBytesRecorded;
    DWORD	tkStart;
    BOOL	bTimesUp;

	CONVERTINFO()
		: dwStartOffset(0)
		, dwMaxLength(0)
		, dwBytesRecorded(0)
		, tkStart(0)
		, bTimesUp(FALSE)
	{
		memset(&mhBuffer, 0, sizeof(MIDIHDR));
	}
};

// Temporary event structure which stores event data until we're ready to
// dump it into a stream buffer
struct TEMPEVENT
{
	DWORD	tkEvent;		// Absolute time of event
	BYTE	byShortData[4];	// Event type and parameters if channel msg
	DWORD	dwEventLength;	// Length of data which follows if meta or sysex
	LPBYTE	pLongData;		// -> Event data if applicable
};

class CMIDI  
{
protected:
	typedef vector<TRACK>		TrackArray_t;
	typedef vector<DWORD>		VolumeArray_t;
	typedef vector<CONVERTINFO>	ConvertArray_t;

	enum {
		NUM_CHANNELS = 16,		// 16 volume channels
		VOLUME_INIT = 100,		// 100% volume by default
		NUM_STREAM_BUFFERS = 2,
		OUT_BUFFER_SIZE = 1024,	// Max stream buffer size in bytes
		DEBUG_CALLBACK_TIMEOUT = 2000,
		VOLUME_MIN = 0,
		VOLUME_MAX = 127		// == 100%
	};

public:
	CMIDI();
	virtual ~CMIDI();

	BOOL	Create(LPBYTE pSoundData, DWORD dwSize, HWND  pParent = 0);
	BOOL	Create(LPCTSTR pszResID, HWND  pParent = 0);
	BOOL	Create(UINT uResID, HWND  pParent = 0);

	BOOL	Play(BOOL bInfinite = FALSE);
	BOOL	Stop(BOOL bReOpen = TRUE);
	BOOL	IsPlaying() const { return m_bPlaying; }

	BOOL	Pause();
	BOOL	Continue();
	BOOL	IsPaused() const { return m_bPaused; }

		// Set playback position back to the start
	BOOL	Rewind();

		// Get the number of volume channels
	DWORD	GetChannelCount() const;

		// Set the volume of a channel in percent. Channels are from 0 to (GetChannelCount()-1)
	void	SetChannelVolume(DWORD channel, DWORD percent);

		// Get the volume of a channel in percent
	DWORD	GetChannelVolume(DWORD channel) const;
	
		// Set the volume for all channels in percent
	void	SetVolume(DWORD percent);

		// Get the average volume for all channels
	DWORD	GetVolume() const;

		// Set the tempo of the playback. Default: 100%
	void	SetTempo(DWORD percent);

		// Get the current tempo in percent (usually 100)
	DWORD	GetTempo() const;

		// You can (un)set an infinite loop during playback.
		// Note that "Play()" resets this setting!
	void	SetInfinitePlay(BOOL bSet = TRUE);

protected:	// implementation
		// This function converts MIDI data from the track buffers.
	int		ConvertToBuffer(DWORD dwFlags, CONVERTINFO * lpciInfo);

		// Fills in the event struct with the next event from the track
	BOOL	GetTrackEvent(TRACK * ptsTrack, TEMPEVENT * pteTemp);

		// Retrieve the next byte from the track buffer, refilling the buffer from
		// disk if necessary.
	BOOL	GetTrackByte(TRACK * ptsTrack, LPBYTE lpbyByte) {
				if( DWORD(ptsTrack->pTrackCurrent - ptsTrack->pTrackStart) == ptsTrack->dwTrackLength )
					return FALSE;
				*lpbyByte = *ptsTrack->pTrackCurrent++;
				return TRUE;
			}

		// Attempts to parse a variable length DWORD from the given track.
	BOOL	GetTrackVDWord(TRACK * ptsTrack, LPDWORD lpdw);

		// Put the given event into the given stream buffer at the given location.
	int		AddEventToStreamBuffer( TEMPEVENT * pteTemp, CONVERTINFO * lpciInfo );

		// Opens a MIDI stream. Then it goes about converting the data into a midiStream buffer for playback.
	BOOL	StreamBufferSetup();

	void	FreeBuffers();


protected:	// error handling
		// The default implementation writes the error message in the
		// debuggers output window. Override if you want a different
		// behavior.
	virtual void	MidiError(MMRESULT Result);

		// Failure in converting track into stream.
		// The default implementation displays the offset and the total
		// number of bytes of the failed track and the error message in
		// the debuggers output window. 
	virtual void	TrackError(TRACK *, LPSTR ErrMsg);


protected:	// overridables
		// NOTE THAT, IF YOU OVERRIDE ONE OF THESE METHODS, YOU MUST CALL
		// THE BASE CLASS IMPLEMENTATION TOO!

		// called when a MIDI output device is opened
	virtual void	OnMidiOutOpen();
	
		// called when the MIDI output device is closed
	virtual void	OnMidiOutClose();

		// called when the specified system-exclusive or stream buffer
		// has been played and is being returned to the application
	virtual void	OnMidiOutDone(MIDIHDR &);

		// called when a MEVT_F_CALLBACK event is reached in the MIDI output stream
	virtual void	OnMidiOutPositionCB(MIDIHDR &, MIDIEVENT &);


private:	// callback procedure
		// This procedure calls the overridables above.
	static void	CALLBACK	MidiProc(HMIDIOUT, UINT, uintptr_t, uintptr_t, uintptr_t);


protected:	// data members
	DWORD			m_dwSoundSize;
	LPVOID			m_pSoundData;
	DWORD			m_dwFormat;
	DWORD			m_dwTrackCount;
	DWORD			m_dwTimeDivision;
	BOOL			m_bPlaying;
	HMIDISTRM		m_hStream;
	DWORD			m_dwProgressBytes;
	BOOL			m_bLooped;
	DWORD			m_tkCurrentTime;
	DWORD			m_dwBufferTickLength;
	DWORD			m_dwCurrentTempo;
	DWORD			m_dwTempoMultiplier;
	BOOL			m_bInsertTempo;
	BOOL			m_bBuffersPrepared;
	int				m_nCurrentBuffer;
	UINT			m_uMIDIDeviceID;
	int				m_nEmptyBuffers;
	BOOL			m_bPaused;
	UINT			m_uCallbackStatus;
	HANDLE			m_hBufferReturnEvent;
	HWND *			m_pWndParent;
	TrackArray_t	m_Tracks;
	VolumeArray_t	m_Volumes;
	ConvertArray_t	m_StreamBuffers;

	// data members especially for ConvertToBuffer()
    TRACK *			m_ptsTrack;
	TRACK *			m_ptsFound;
    DWORD			m_dwStatus;
    DWORD			m_tkNext;
	DWORD			m_dwMallocBlocks;
    TEMPEVENT		m_teTemp;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // MIDI_h
