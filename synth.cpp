#include "synth.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <iostream>
#include <cstring>

int CaseStrCmp(const char* str1, const char* str2)
{
    while (*str1 && *str2) 
	{
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) 
		{
            return *str1 - *str2;
        }
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

void Synth::AudioCallbackStatic(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    // Access the Synth instance from user data
    Synth* synth = static_cast<Synth*>(pDevice->pUserData);
    synth->AudioCallback(pDevice, pOutput, frameCount);
}
		
// Callback function called by the audio thread
void Synth::AudioCallback(ma_device* pDevice, void* pOutput, ma_uint32 frameCount)
{
	ma_format format = pDevice->playback.format;
    ma_uint32 channels = pDevice->playback.channels;
    ma_uint32 bytesPerSample = ma_get_bytes_per_sample(format);
    ma_uint32 len = frameCount * channels * bytesPerSample;
	unsigned char* stream = static_cast<unsigned char*>(pOutput);

	//Number of samples to process
	int SampleBlock, SampleCount = (len / (2 * sizeof(float))); //2 output channels
	for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (2 * sizeof(float))))
	{
		//We progress the MIDI playback and then process TSF_RENDER_EFFECTSAMPLEBLOCK samples at once
		if (SampleBlock > SampleCount) SampleBlock = SampleCount;
		double tempo_us = customTempo > 0 ? customTempo : currentTempo; // Use custom or MIDI-defined tempo
		double timeAdvance = SampleBlock * (tempo_us / 44100.0 / 1000.0);
		if (!paused)
		{//Loop through all MIDI messages which need to be played up until the current playback time
		for (g_Msec += timeAdvance; g_MidiMessage && g_Msec >= g_MidiMessage->time; g_MidiMessage = g_MidiMessage->next)
		{
			
			if (mutedTracks.find(g_MidiMessage->trackNum) != mutedTracks.end())
			{
				// Track is muted, skip this event
				continue;
			}
			
			switch (g_MidiMessage->type)
			{
				
				case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
					tsf_channel_set_presetnumber(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->program, (g_MidiMessage->channel == 9));
					break;
				case TML_NOTE_ON: //play a note
					tsf_channel_note_on(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key, g_MidiMessage->velocity / 127.0f);
					break;
				case TML_NOTE_OFF: //stop a note
					tsf_channel_note_off(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->key);
					break;
				case TML_PITCH_BEND: //pitch wheel modification
					tsf_channel_set_pitchwheel(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->pitch_bend);
					break;
				case TML_CONTROL_CHANGE: //MIDI controller messages
					tsf_channel_midi_control(g_TinySoundFont, g_MidiMessage->channel, g_MidiMessage->control, g_MidiMessage->control_value);
					break;
				
			}
		if(g_MidiMessage->type == TML_MARKER && CaseStrCmp(g_MidiMessage->markerName, "loopstart") == 0)
		{
			//printf("Found Loop Start marker!\n");
			loopStartPosition = g_Msec;
			loopStartPositionMessage = g_MidiMessage;
		}

		}}

		// Render the block of audio samples in float format
		tsf_render_float(g_TinySoundFont, (float*)stream, SampleBlock, 0);
	//Looping code
	if (!g_MidiMessage || g_MidiMessage->type == TML_MARKER && CaseStrCmp(g_MidiMessage->markerName, "loopend") == 0) 
	{
		if(loops <= -1 || loopCount < loops)
		{
			g_Msec = (double)loopStartPosition;
			g_MidiMessage = loopStartPositionMessage;
			if (loops > -1)
			{loopCount++;}
		}
	}
	//Muting channel control
	for(unsigned char mc = 0; mc < 16; mc++)
	{
		oldVolume = tsf_channel_get_volume(g_TinySoundFont,mc);
		if(mutedChannels[mc] == true)
		{
		
		tsf_channel_set_volume(g_TinySoundFont, mc, 0.0f);
		}	
		else
		{tsf_channel_set_volume(g_TinySoundFont,mc,oldVolume);}
	}

	//Master pitch control
	if(pitchWheel != 8192)
	{
		for(char c=0;c<16;c++)
		{tsf_channel_set_pitchwheel(g_TinySoundFont, c, pitchWheel);}
		if(pitchRangeCustom !=2)
		{
			for(char c=0;c<16;c++)
			{tsf_channel_set_pitchrange(g_TinySoundFont, c, pitchRangeCustom);}
		}
	}
	
	}
}

Synth::Synth(const char *fName):filename(fName),loops(0),loopCount(0), g_MidiMessage(nullptr), g_Msec(0.0), g_TinySoundFont(nullptr)
{
	// Define the desired audio output format we request
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.sampleRate = 44100;
	deviceConfig.playback.format = ma_format_f32;
	deviceConfig.playback.channels = 2;
	deviceConfig.dataCallback = AudioCallbackStatic;
	deviceConfig.pUserData = this;
	currentTempo = 1000000;


	loopStartPosition = 0.0f;
	g_TinySoundFont = tsf_load_filename(fName);
	//Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
	tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);

	// Set the SoundFont rendering output mode
	tsf_set_output(g_TinySoundFont, TSF_STEREO_INTERLEAVED, deviceConfig.sampleRate, 0.0f);

	// Request the desired audio output format
	ma_device_init(TSF_NULL, &deviceConfig, &device);
	
}

Synth::~Synth()
{
	tsf_close(g_TinySoundFont);
	tml_free(TinyMidiLoader);
}

void Synth::PlaySong(const char *filename, short loops)

{
	this->loops = loops;
	TinyMidiLoader = tml_load_filename(filename);
	g_MidiMessage = TinyMidiLoader;
	loopStartPositionMessage = g_MidiMessage;
	ma_device_start(&device);

}

void Synth::MuteChannel(unsigned char channel)
{
	mutedChannels[channel]=true;
}

void Synth::SetChannelVolume(unsigned char channel, unsigned char volume)
{
	float channelVolume = volume / 100.0f;
	tsf_channel_set_volume(g_TinySoundFont, channel, channelVolume);	
}

void Synth::UnmuteChannel(unsigned char channel)
{
	mutedChannels[channel]=false;
}

void Synth::ResetChannelVolume(unsigned char channel)
{
	tsf_channel_set_volume(g_TinySoundFont, channel, oldVolume);	
}

void Synth::SetTempo(double bpm)
{
	customTempo = (25000/3) * bpm;
}

void Synth::ResetTempo()
{
	customTempo = 0;
}

void Synth::ChangeInstrument(char instrument, char channel, char msbBank)
{
	if (channel == 9)
	{tsf_channel_set_bank_preset(g_TinySoundFont, channel, 128, instrument);}
	else
	{tsf_channel_set_bank_preset(g_TinySoundFont, channel, msbBank, instrument);}
}

void Synth::ChangeSoundfont(const char* filename)
{
	for(char oi = 0; oi < 16; oi++)
	{oldInstruments[oi] = tsf_channel_get_preset_number(g_TinySoundFont, oi);
	oldMSB[oi] = tsf_channel_get_preset_bank(g_TinySoundFont, oi);
	std::cout << "instrument on channel " << oi << "is " << tsf_channel_get_preset_number(g_TinySoundFont, oi) << "\n";
	}

	g_TinySoundFont = tsf_load_filename(filename);
	tsf_channel_set_bank_preset(g_TinySoundFont, 9, 128, 0);
	for(char oi = 0; oi < 16; oi++)
	{ChangeInstrument(oldInstruments[oi],oi,oldMSB[oi]);}
}

void Synth::MuteTrack(unsigned short track)
{
	mutedTracks.insert(track);										
}

void Synth::UnmuteTrack(unsigned short track)
{
	if (mutedTracks.find(track) != mutedTracks.end())
	{
		mutedTracks.erase(track);
    }
}

void Synth::PauseSongToggle()
{
	paused = !paused;
	if (paused)
	{for(char i = 0; i<16; i++)
	{tsf_channel_note_off_all(g_TinySoundFont, i);}}
}

void Synth::PauseSong()
{
	paused = true;
	for(char i = 0; i<16; i++)
	{tsf_channel_note_off_all(g_TinySoundFont, i);}	
}

void Synth::MuteChannelToggle(unsigned char channel)
{
	mutedChannels[channel]=!mutedChannels[channel];
}

void Synth::MuteTrackToggle(unsigned short track)
{	
	if (mutedTracks.find(track) != mutedTracks.end())
	{
		UnmuteTrack(track);
	}
	else
	{
		MuteTrack(track);
	}
}

void Synth::SetMasterVolume(unsigned char volume)
{
	masterVolume = volume / 100.0f;
	tsf_set_volume(g_TinySoundFont, masterVolume);
}

void Synth::SetMasterPitch(unsigned short pitch, char pitchRange)
{
	pitchWheel = pitch;
	pitchRangeCustom = pitchRange;
}

bool Synth::HasFileEnded()
{
	if(!g_MidiMessage && loops == 0)
	{return true;}
	else
	{return false;}
}

void Synth::SetChannelPan(char pan, char channel)
{
	float customChannelPan = pan / 128.0f;
	tsf_channel_set_pan(g_TinySoundFont, channel, customChannelPan);
}

void Synth::SetMasterPan(char pan)
{
	float customMasterPan = pan / 128.0f;	
	for(char c=0;c<16;c++)
			tsf_channel_set_pan(g_TinySoundFont, c, customMasterPan);
}