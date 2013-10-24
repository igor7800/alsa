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
 
#include "mypcm.h"
int main(int argc, char *argv[])
{
    char *buf;
    unsigned int period;
    int rate;
    int channels;
    int seconds;
    int buf_size;
    int i;
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;

    if (argc < 4)
    {
	printf("Usage: %s <sample_rate> <channels> <seconds>\n",
	       argv[0]);
	exit(1);
    }
 
    rate  = atoi(argv[1]);
    channels = atoi(argv[2]);
    seconds  = atoi(argv[3]);

    open_pcm(&playback_handle,PCM_DEVICE,SND_PCM_STREAM_PLAYBACK,0);
    snd_pcm_hw_params_malloc (&params);
    snd_pcm_hw_params_any(playback_handle, params);

    set_params(playback_handle,params,channels,rate);
    write_params(playback_handle,params);    
  
  /* Allocate buffer to hold single period */
  snd_pcm_hw_params_get_period_size(params, &frames, 0);
  buf_size = frames * channels * 2 /* 2 -> sample size */;
  buf = (char *) malloc(buf_size);  

  period = get_period_time(params);
  snd_pcm_hw_params_free(params);
  for (i = (seconds * 1000000) / period; i > 0; i--)
    play(playback_handle,frames,buf,buf_size); 

  snd_pcm_drain(playback_handle);
  snd_pcm_close(playback_handle);
  free(buf);
  return 0;
}
 
