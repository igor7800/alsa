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
    snd_pcm_hw_params_t *params;
  
  
    open_pcm(&capture_handle,PCM_DEVICE,SND_PCM_STREAM_CAPTURE,0); 
    snd_pcm_hw_params_malloc (&params);
    snd_pcm_hw_params_any (capture_handle, params);
    set_params(capture_handle,params,CHANNELS,RATE);
    write_params(capture_handle,params);
    prepair_interface(capture_handle);
    snd_pcm_hw_params_free (params);
    
    for (i = 0; i < LOOPS; i++)
    {
	record(capture_handle,buf,SIZE);	
	for (i=0;i<SIZE ;i++ ) 
	    printf ("%d\n",buf[i]);
    }
    snd_pcm_drain(capture_handle);
    snd_pcm_close (capture_handle);
    exit (0);
    return 0;
}
