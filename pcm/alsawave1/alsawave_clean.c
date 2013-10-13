// A simple C example to play a mono or stereo, 16-bit 44KHz
// WAVE file using ALSA. This goes directly to the first
// audio card (ie, its first set of audio out jacks). It
// uses the snd_pcm_writei() mode of outputting waveform data,
// blocking.
//
// Compile as so to create "alsawave":
// gcc -o alsawave alsawave.c -lasound
//
// Run it from a terminal, specifying the name of a WAVE file to play:
// ./alsawave MyWaveFile.wav

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

// Size of the audio card hardware buffer. Here we want it
// set to 1024 16-bit sample points. This is relatively
// small in order to minimize latency. If you have trouble
// with underruns, you may need to increase this, and PERIODSIZE
// (trading off lower latency for more stability)
#define BUFFERSIZE	(2*1024)

// How many sample points the ALSA card plays before it calls
// our callback to fill some more of the audio card's hardware
// buffer. Here we want ALSA to call our callback after every
// 64 sample points have been played
#define PERIODSIZE	(2*64)


/////////////////////// WAVE File Stuff /////////////////////
// An IFF file header looks like this
typedef struct _FILE_head
{
  unsigned char	ID[4];	// could be {'R', 'I', 'F', 'F'} or {'F', 'O', 'R', 'M'}
  unsigned int	Length;	// Length of subsequent file (including remainder of header). This is in
  // Intel reverse byte order if RIFF, Motorola format if FORM.
  unsigned char	Type[4];	// {'W', 'A', 'V', 'E'} or {'A', 'I', 'F', 'F'}
} FILE_head;


// An IFF chunk header looks like this
typedef struct _CHUNK_head
{
  unsigned char ID[4];	// 4 ascii chars that is the chunk ID
  unsigned int	Length;	// Length of subsequent data within this chunk. This is in Intel reverse byte
  // order if RIFF, Motorola format if FORM. Note: this doesn't include any
  // extra byte needed to pad the chunk out to an even size.
} CHUNK_head;

// WAVE fmt chunk
typedef struct _FORMAT {
  short	wFormatTag;
  unsigned short wChannels;
  unsigned int dwSamplesPerSec;
  unsigned int dwAvgBytesPerSec;
  unsigned short wBlockAlign;
  unsigned short wBitsPerSample;
  // Note: there may be additional fields here
} FORMAT;

snd_pcm_t           *PlaybackHandle; // Handle to ALSA (audio card's) playback port
snd_async_handler_t *CallbackHandle; // Handle to our callback thread
unsigned char       *WavePtr;        // Points to loaded WAVE file's data
snd_pcm_uframes_t    WaveSize;       // Size (in frames) of loaded WAVE file's data
unsigned short	     WaveRate;       // Sample rate
unsigned char	     WaveBits;       // Bit resolution
unsigned char	     WaveChannels; // Number of channels in the wave file

// The name of the ALSA port we output to. In this case, we're
// directly writing to hardware card 0,0 (ie, first set of audio
// outputs on the first audio card)
static const char  SoundCardPortName[] = "plughw:0,0";

// For WAVE file loading
static const unsigned char Riff[4] = { 'R', 'I', 'F', 'F' };
static const unsigned char Wave[4] = { 'W', 'A', 'V', 'E' };
static const unsigned char Fmt[4]  = { 'f', 'm', 't', ' ' };
static const unsigned char Data[4] = { 'd', 'a', 't', 'a' };


/**
 * Compares the passed ID str (ie, a ptr to 4 Ascii
 * bytes) with the ID at the passed ptr. Returns TRUE if
 * a match, FALSE if not.
 */

static unsigned char compareID(const unsigned char * id, unsigned char * ptr)
{
  register unsigned char i = 4;
  while (i--)
    {
      if ( *(id)++ != *(ptr)++ ) return(0);
    }
  return(1);
}


/**
 * Loads a WAVE file.
 *
 * @param fn Filename to load.
 * @return 0 if success, non-zero if not.
 *
 * NOTE: Sets the global "WavePtr" to an allocated buffer
 * containing the wave data, and "WaveSize" to the size
 * in sample points.
 */

static unsigned char waveLoad(const char *fn)
{
  const char *message;
  FILE_head head;
  register int inHandle;
  if ((inHandle = open(fn, O_RDONLY)) == -1) message = "didn't open";
  // Read in IFF File header
  else{
    if (read(inHandle, &head, sizeof(FILE_head)) == sizeof(FILE_head)){
      // Is it a RIFF and WAVE?
      if (!compareID(&Riff[0], &head.ID[0]) || !compareID(&Wave[0], &head.Type[0])){
	  message = "is not a WAVE file";
	  goto bad;
      }
      // Read in next chunk header
      while (read(inHandle, &head, sizeof(CHUNK_head)) == sizeof(CHUNK_head))
	{
	  // ============================ Is it a fmt chunk? ===============================
	  if (compareID(&Fmt[0], &head.ID[0])){
	    FORMAT format;	 
	    // Read in the remainder of chunk
	    if (read(inHandle, &format.wFormatTag, sizeof(FORMAT)) != sizeof(FORMAT)) break;	    
	    // Can't handle compressed WAVE files
	    if (format.wFormatTag != 1)
	      {
		message = "compressed WAVE not supported";
		goto bad;
	      }
	    WaveBits = (unsigned char)format.wBitsPerSample;
	    WaveRate = (unsigned short)format.dwSamplesPerSec;
	    WaveChannels = format.wChannels;
	  }	  
	  // ============================ Is it a data chunk? ===============================
	  else if (compareID(&Data[0], &head.ID[0])){
	    // Size of wave data is head.Length. Allocate a buffer and read in the wave data
	    if (!(WavePtr = (unsigned char *)malloc(head.Length))){
	      message = "won't fit in RAM";
	      goto bad;
	    }	   
	    if (read(inHandle, WavePtr, head.Length) != head.Length){
	      free(WavePtr);
	      break;
	    }
	    // Store size (in frames)
	    WaveSize = (head.Length * 8) / ((unsigned int)WaveBits * (unsigned int)WaveChannels);	    
	    close(inHandle);
	    return(0);
	  }	   
	  // ============================ Skip this chunk ===============================
	  else{
	      if (head.Length & 1) ++head.Length;  // If odd, round it up to account for pad byte
	      lseek(inHandle, head.Length, SEEK_CUR);
	    }
	}
    }
    message = "is a bad WAVE file";
  bad:	close(inHandle);
  }
  printf("%s %s\n", fn, message);
  return(1);
}



/**
 * Plays the loaded waveform.
 *
 * NOTE: ALSA sound card's handle must be in the global
 * "PlaybackHandle". A pointer to the wave data must be in
 * the global "WavePtr", and its size of "WaveSize".
 */
static void play_audio(void)
{
  register snd_pcm_uframes_t count, frames;
  // Output the wave data
  count = 0;
  do  
    {
      frames = snd_pcm_writei(PlaybackHandle, WavePtr + count, WaveSize - count);      
      // If an error, try to recover from it
      if (frames < 0) frames = snd_pcm_recover(PlaybackHandle, frames, 0);
      if (frames < 0){
	printf("Error playing wave: %s\n", snd_strerror(frames));
	break;
      }
      // Update our pointer
      count += frames;
    } while (count < WaveSize); 
    // Wait for playback to completely finish
    if (count == WaveSize) snd_pcm_drain(PlaybackHandle);
}


/**
 * Frees any wave data we loaded.
 *
 * NOTE: A pointer to the wave data be in the global
 * "WavePtr".
 */
static void free_wave_data(void)
{
  if (WavePtr) free(WavePtr);
  WavePtr = 0;
}



int main(int argc, char **argv)
{
  // No wave data loaded yet
  WavePtr = 0;
  if (argc < 2) printf("You must supply the name of a 16-bit mono WAVE file to play\n");
  else if (!waveLoad(argv[1])){   // Load the wave file
    register int err;
    // Open audio card we wish to use for playback
    if ((err = snd_pcm_open(&PlaybackHandle, &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0)) < 0)
      printf("Can't open audio %s: %s\n", &SoundCardPortName[0], snd_strerror(err));
    else {
      switch (WaveBits)
	{
	case 8:
	  err = SND_PCM_FORMAT_U8;
	  break;			
	case 16:
	  err = SND_PCM_FORMAT_S16;
	  break;	  
	case 24:
	  err = SND_PCM_FORMAT_S24;
	  break;	  
	case 32:
	  err = SND_PCM_FORMAT_S32;
	  break;
	}		
      // Set the audio card's hardware parameters (sample rate, bit resolution, etc)
      if ((err = snd_pcm_set_params(PlaybackHandle, err, SND_PCM_ACCESS_RW_INTERLEAVED, WaveChannels, WaveRate, 1, 500000)) < 0)
	printf("Can't set sound parameters: %s\n", snd_strerror(err));

      else play_audio();             // Play the waveform      
      snd_pcm_close(PlaybackHandle); // Close sound card
    }
  }
  free_wave_data();   // Free the WAVE data
  return(0);
}

