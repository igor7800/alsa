/**
 * Simple sound capture using ALSA API and libasound.
 *
 * Compile:
 * gcc  capture.c -o capture -lasound
 * 
 */
 
#include "mypcm.h"
#define SIZE 128
#define CHANNELS 2
#define RATE 44100
#define LOOPS 10

int main (int argc, char *argv[])
{
    int i;
    char buf[SIZE];
    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *capture_params;
    snd_pcm_hw_params_t *playback_params;
    snd_pcm_uframes_t frames;

  
    open_pcm(&capture_handle,PCM_DEVICE,SND_PCM_STREAM_CAPTURE,0); 
    open_pcm(&playback_handle,PCM_DEVICE,SND_PCM_STREAM_PLAYBACK,0);
    //-----------------------------------------------------
    snd_pcm_hw_params_malloc (&playback_params);
    snd_pcm_hw_params_malloc (&capture_params);
    //----------------------------------------------------
    snd_pcm_hw_params_any (playback_handle, playback_params);
    snd_pcm_hw_params_any (capture_handle, capture_params);
    //----------------------------------------------------
    set_params(playback_handle,playback_params,CHANNELS,RATE);
    set_params(capture_handle,capture_params,CHANNELS,RATE);
    //----------------------------------------------------
    write_params(playback_handle,playback_params);    
    write_params(capture_handle,capture_params);
    //----------------------------------------------------
    prepair_interface(capture_handle);
    prepair_interface(playback_handle);
    snd_pcm_hw_params_get_period_size(capture_params, &frames, 0);
    //----------------------------------------------------
    snd_pcm_hw_params_free (playback_params);
    snd_pcm_hw_params_free (capture_params);

    for (i = 0; i < LOOPS; i++)
    {
	record(capture_handle,buf,SIZE);
	//for (i=0;i<SIZE ;i++ ) 
	//printf ("%d\n",buf[i]);
	play(playback_handle,frames,buf,SIZE); 
    }

    snd_pcm_drain(playback_handle);
    snd_pcm_drain(capture_handle);
    snd_pcm_close(playback_handle);
    snd_pcm_close (capture_handle);
    return 0;
}
