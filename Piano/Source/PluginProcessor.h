#pragma once

#include <JuceHeader.h>

#define NPARAMS 12       // number of parameters
#define NPROGS  8        // number of programs
#define NVOICES 32       // max polyphony
#define SILENCE 0.0001f  // voice choking
#define WAVELEN 586348   // wave data bytes

// Describes a factory preset.
struct MDAPianoProgram
{
  MDAPianoProgram();
  MDAPianoProgram(const char *name,
                  float p0, float p1, float p2, float p3,
                  float p4, float p5, float p6, float p7,
                  float p8, float p9, float p10, float p11);
  char name[24];
  float param[NPARAMS];
};

struct VOICE  //voice state
{
  int delta;  //sample playback
  int frac;
  int pos;
  int end;
  int loop;

  float env;  //envelope
  float dec;

  float f0;   //first-order LPF
  float f1;
  float ff;

  float outl;
  float outr;
  int note; //remember what note triggered this
};

struct KGRP  //keygroup
{
  int root;  //MIDI root note
  int high;  //highest note
  int pos;
  int end;
  int loop;
};

class MDAPianoAudioProcessor : public juce::AudioProcessor,
                               private juce::ValueTree::Listener
{
public:
  MDAPianoAudioProcessor();
  ~MDAPianoAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void reset() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override;

  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout() };

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override
  {
    _parametersChanged.store(true);
  }

  std::atomic<bool> _parametersChanged { false };

  void update();
  void resetState();

  void createPrograms();

  void processEvents(juce::MidiBuffer &midiMessages);
  void noteOn(int note, int velocity);
//  void guiGetDisplay(int index, char *label);

  // The factory presets.
  std::vector<MDAPianoProgram> _programs;

  // Index of the active preset.
  int _currentProgram;

  // The current sample rate and 1 / sample rate.
  float Fs, iFs;

  // MIDI note on / note off events for the current block. Each event is
  // described by 3 values: delta time, note number, velocity.
  #define EVENTBUFFER 120
  int notes[EVENTBUFFER + 8];

  // Special event code that marks the end of the MIDI events list.
  #define EVENTS_DONE 99999999

  // Special event code that ends all sustained notes.
  #define SUSTAIN 128


  ///global internal variables
  KGRP  kgrp[16];
  VOICE voice[NVOICES];
  int  activevoices, poly, cpos;
  short *waves;
  int  cmax;
  float *comb, cdep, width, trim;
  int  size, sustain;
  float tune, fine, random, stretch;
  float muff, muffvel, sizevel, velsens, volume;

  int guiUpdate;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDAPianoAudioProcessor)
};
