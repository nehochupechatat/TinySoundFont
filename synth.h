#pragma once
struct tml_message;
struct tsf;
#include <set>
#include "stdio.h"
#include "miniaudio.h"

int CaseStrCmp(const char* str1, const char* str2);

class Synth 
{
    public:
        Synth(const char *filename);
        ~Synth();
		static void AudioCallbackStatic(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
		void AudioCallback(ma_device* pDevice, void* pOutput, ma_uint32 frameCount);
        void PlaySong(const char *filename, short loops);
		void MuteChannel(unsigned char channel);
		void SetChannelVolume(unsigned char channel, unsigned char volume);
		void UnmuteChannel(unsigned char channel);
		void ResetChannelVolume(unsigned char channel);
		void SetTempo(double bpm);
		void ResetTempo();
		void MuteTrack(unsigned short track);
		void UnmuteTrack(unsigned short track);
		void ChangeSoundfont(const char* filename);
		void ChangeInstrument(char instrument, char channel, char msbBank);
		void PauseSongToggle();
		void PauseSong();
		void MuteChannelToggle(unsigned char channel);
		void MuteTrackToggle(unsigned short track);
		void SetMasterVolume(unsigned char volume);
		void SetMasterPitch(unsigned short pitch, char pitchRange);
		bool HasFileEnded();
		void SetChannelPan(char pan, char channel);
		void SetMasterPan(char pan);
    private:
		ma_device_config deviceConfig;
		ma_device device;
        const char *filename;
		short loopCount;
		short loops;
		float oldVolume;
		float masterVolume = 1.0f;
		bool midiPlaybackEnded;
		unsigned short pitchWheel = 8192;
		char pitchRangeCustom = 2;
		tml_message* TinyMidiLoader;
		tml_message* g_MidiMessage;
		double g_Msec;
		double loopStartPosition;
		tml_message* loopStartPositionMessage;
		tsf* g_TinySoundFont;
		std::set<unsigned short> mutedTracks;
		double customTempo = 0;
		double currentTempo;
		char oldInstruments[16];
		char oldMSB[16];
		bool paused = false;
		bool mutedChannels[16]={false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
};