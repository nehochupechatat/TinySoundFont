#include "synth.h"
#include <thread>
#include <chrono>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
void SongDelay(int ms);

void SongDelay(int ms) 
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

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
		while (!synth.HasFileEnded()) SongDelay(250);
	}
	return 0;
}