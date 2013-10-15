/**
 * Simple sound playback using ALSA API and libasound.
 *
 * Compile:
 * gcc  playback.c -o playback -lasound
 * 
 * Usage:
 * $ ./play "sample_rate" "channels" "seconds" < "file"
 * 
 * Examples:
 * $ ./play 44100 2 5 < /dev/urandom
 * $ ./play 22050 1 8 < /path/to/file.wav
 *
 */
 
#include <alsa/asoundlib.h>
#include <stdio.h>
 
#define PCM_DEVICE "default"
 
 
/**
 * Open the PCM device in playback mode 
 * and writes errror if device 
 * @param **pcm_handle handle to playback
 * @param *card audio card to play to
 * @param stream Wanted stream
 * @param mode Open mode
 */
void pb_open_pcm (snd_pcm_t **pcm_handle, 
		  char *card,snd_pcm_stream_t stream, int mode)
{
  unsigned int pcm;
  pcm = snd_pcm_open(pcm_handle,card, stream, 0);
  if (pcm < 0)
    { 
      printf("ERROR: Can't open \"%s\" PCM device. %s\n",
	     card, snd_strerror(pcm));
      exit(1);
    }  

}

/**
 * Restrict a configuration space to contain only one access type 
 * and writes an error if interleaved mode can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 */
void pb_set_access(snd_pcm_t *pcm_handle, snd_pcm_hw_params_t *params)
{
  unsigned int pcm;
  pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
				     SND_PCM_ACCESS_RW_INTERLEAVED);
    if (pcm < 0)
      { 
	printf("ERROR: Can't set interleaved mode. %s\n", 
	       snd_strerror(pcm));
	exit(1);
      }
}

/**
 * Restrict a configuration space to contain only one format 
 * and writes an error if format can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 */
void pb_set_format(snd_pcm_t *pcm_handle, snd_pcm_hw_params_t *params)
{
  unsigned int pcm;
  pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
				     SND_PCM_FORMAT_S16_LE);
 if (pcm < 0)
   { 
     printf("ERROR: Can't set format. %s\n",
	    snd_strerror(pcm));
     exit(1);
   }
}


/**
 * Restrict a configuration space to one channels count 
 * and writes an error if channels number can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 * @param channels channels count
 */
void pb_set_channels(snd_pcm_t *pcm_handle,
		     snd_pcm_hw_params_t *params,int channels)
{
  unsigned int pcm;
  pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
  if (pcm < 0)
    { 
      printf("ERROR: Can't set channels number. %s\n", 
	     snd_strerror(pcm));
      exit(1);
    }
}


/**
 * Restrict a configuration space to contatin only one rate 
 * and writes an error if rate can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 * @param rate approximate rate
 */
void pb_set_rate(snd_pcm_t *pcm_handle,
		 snd_pcm_hw_params_t *params,int rate)
{
  unsigned int pcm;
  pcm = snd_pcm_hw_params_set_rate(pcm_handle, params, rate, 0);
  if (pcm < 0) 
    {
      printf("ERROR: Can't set rate. %s\n", 
	     snd_strerror(pcm));
      exit(1);
    }
}


/**
 * Set hardware parameters
 * and writes an errors if parameters can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 * @param channels channels count
 * @param rate approximate rate
 */
void pb_set_params(snd_pcm_t *pcm_handle, snd_pcm_hw_params_t *params,
		   int channels, int rate)
{
  unsigned int pcm;
  pb_set_access(pcm_handle,params);
  pb_set_format(pcm_handle,params);
  pb_set_channels(pcm_handle,params,channels);
  pb_set_rate(pcm_handle,params,rate);
}


/**
 * Write hardware parameters
 * and writes an error if hardware parameters can't be set
 * @param **pcm_handle handle to playback
 * @param *params configuration space
 */
void pb_write_params(snd_pcm_t *pcm_handle, snd_pcm_hw_params_t *params)
{
  unsigned int pcm;
  pcm = snd_pcm_hw_params(pcm_handle, params);
  if (pcm < 0)
    {
      printf("ERROR: Can't set harware parameters. %s\n",
	     snd_strerror(pcm));
      exit(1);
    }
}

/**
 * Extracts period time from a configuration space
 * and writes and error if period time can't be extracted
 * @param *params configuration space
 * @return tmp approximate period duration
 */
unsigned int pb_get_period_time(snd_pcm_hw_params_t *params)
{
  unsigned int tmp,pcm;
  pcm = snd_pcm_hw_params_get_period_time(params, &tmp, NULL);  
  if (pcm < 0)
    {
      printf("ERROR: Can't extract maximum period time.%s\n",
	     snd_strerror(pcm));
      exit(1); 
    }
  return tmp;
}

/**
 * Play the wav file for certain number of seconds
 * write error when unable to write to PCM device 
 * @param **pcm_handle handle to playback
 * @param frames period size
 * @param period length of period
 * @param *buffer pointer to buffer
 * @param buff_size size of the buffer
 * @param seconds ammount of seconds to be played
 */
void pb_play_file(snd_pcm_t *pcm_handle,  snd_pcm_uframes_t frames, 
		  int period, char *buff, int buff_size,int seconds)
{
  unsigned int pcm;
  int loops;
  for (loops = (seconds * 1000000) / period; loops > 0; loops--)
    {
      if (pcm = read(0, buff, buff_size) == 0) 
	{
	  printf("Early end of file.\n");
	  exit(1);
	}
      
      if (pcm = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE)
	{
	  printf("XRUN.\n");
	  snd_pcm_prepare(pcm_handle);
	}
      else if (pcm < 0) 
	{
	  printf("ERROR. Can't write to PCM device. %s\n",
		 snd_strerror(pcm));
	  exit(1);
	}
    }
}

int main(int argc, char *argv[])
{
  unsigned int period;
  int rate, channels,seconds;
  snd_pcm_t *pcm_handle;
  snd_pcm_hw_params_t *params;
  snd_pcm_uframes_t frames;
  char *buff;
  int buff_size;

  if (argc < 4) {
    printf("Usage: %s <sample_rate> <channels> <seconds>\n",
	   argv[0]);
    exit(1);
  }
 
  rate  = atoi(argv[1]);
  channels = atoi(argv[2]);
  seconds  = atoi(argv[3]);

  pb_open_pcm(&pcm_handle,PCM_DEVICE,SND_PCM_STREAM_PLAYBACK,0);
  /* Allocate parameters object and fill it with default values*/
  snd_pcm_hw_params_alloca(&params);  
  snd_pcm_hw_params_any(pcm_handle, params);

  pb_set_params(pcm_handle,params,channels,rate);
  pb_write_params(pcm_handle,params);    

  /* Allocate buffer to hold single period */
  snd_pcm_hw_params_get_period_size(params, &frames, 0);
   buff_size = frames * channels * 2 /* 2 -> sample size */;
  buff = (char *) malloc(buff_size);

  //snd_pcm_hw_params_get_period_time(params, &tmp, NULL);  
  period = pb_get_period_time(params);
  pb_play_file(pcm_handle,frames,period,buff,buff_size,seconds);
  snd_pcm_drain(pcm_handle);
  snd_pcm_close(pcm_handle);
  free(buff);
  return 0;
}
 
