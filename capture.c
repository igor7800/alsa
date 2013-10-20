#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "mypcm.h"
      
main (int argc, char *argv[])
{
  int i;
  int err;
  short buf[128];
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;

  
  pb_open_pcm(&capture_handle,PCM_DEVICE,SND_PCM_STREAM_CAPTURE,0); 
  snd_pcm_hw_params_malloc (&hw_params);
  snd_pcm_hw_params_any (capture_handle, hw_params);
  pb_set_params(capture_handle,hw_params,2,44100);
  pb_write_params(capture_handle,hw_params);
  snd_pcm_hw_params_free (hw_params);
  pb_prepair_interface(capture_handle);

  for (i = 0; i < 10; ++i) {
    if ((err = snd_pcm_readi (capture_handle, buf, 128)) != 128) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               snd_strerror (err));
      exit (1);
    }
  }
  
  for (i=0;i<128 ;i++ ) printf ("%d\n",buf[i]);
  
  snd_pcm_close (capture_handle);
  exit (0);
}
