// cardnames.c
// Lists the names of all ALSA sound cards in the system.
//
// Compile as:
// gcc -o cardnames cardnames.c -lasound

#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>

int main(int argc, char **argv)
{
  register int err;
  int cardNum;
  char str[64];

  // Start with first card
  cardNum = -1;

  for (;;)
    {
      snd_ctl_t *cardHandle;
      snd_ctl_card_info_t *cardInfo;
      if ((err = snd_card_next(&cardNum)) < 0){
	  printf("Can't get the next card number: %s\n", snd_strerror(err));
	  break;
	}
      if (cardNum < 0) break;
      sprintf(str, "hw:%i", cardNum);
      if ((err = snd_ctl_open(&cardHandle, str, 0)) < 0){
	printf("Can't open card %i: %s\n", cardNum, snd_strerror(err));
	continue;
      }
      snd_ctl_card_info_alloca(&cardInfo);
      if ((err = snd_ctl_card_info(cardHandle, cardInfo)) < 0)
	printf("Can't get info for card %i: %s\n", cardNum, snd_strerror(err));
      else
	printf("Card %i = %s\n", cardNum, snd_ctl_card_info_get_name(cardInfo));
      snd_ctl_close(cardHandle);
    }
  snd_config_update_free_global();
}



