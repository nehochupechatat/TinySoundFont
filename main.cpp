#include "synth.h"
#include "minisdl_audio.h"
int main(int argc, char **argv)
{
	if(argc < 3){
		fprintf(stderr, "usage: %s SF2BANK MIDIFILE\n", argv[0]);
		return 1;
	}

	if(argc >= 3)
	{
		Synth synth(argv[1]);
		synth.PlaySong(argv[2],0);
		while (!synth.HasFileEnded()) SDL_Delay(100);
	}
	return 0;
}