#pragma once

#include <JuceHeader.h>

#define NPARAMS 24       // number of parameters
#define NVOICES 8        // max polyphony

#define SILENCE 0.0001f  // voice choking
#define ANALOG  0.002f   // oscillator drift

#define PI    3.1415926535897932f
#define TWOPI 6.2831853071795864f

// Describes a factory preset.
struct JX10Program
{
  JX10Program();
  JX10Program(const char *name,
              float p0,  float p1,  float p2,  float p3,
              float p4,  float p5,  float p6,  float p7,
              float p8,  float p9,  float p10, float p11,
              float p12, float p13, float p14, float p15,
              float p16, float p17, float p18, float p19,
              float p20, float p21, float p22, float p23);
  char name[24];
  float param[NPARAMS];
};

// State for an active voice.
struct Voice
{
  float period;
  float p;    //sinc position
  float pmax; //loop length
  float dp;   //delta
  float sin0; //sine osc
  float sin1;
  float sinx;
  float dc;   //dc offset

  float detune;
  float p2;    //sinc position
  float pmax2; //loop length
  float dp2;   //delta
  float sin02; //sine osc
  float sin12;
  float sinx2;
  float dc2;   //dc offset

  float fc;  //filter cutoff root
  float ff;  //filter cutoff
  float f0;  //filter buffers
  float f1;
  float f2;

  float saw;

  float env;     // current amplitude envelope level
  float envd;    // envelope decay value
  float envl;    // envelope target level

  float fenv;    // current filter envelope level
  float fenvd;   // envelope decay value
  float fenvl;   // envelope target level

  float lev;     // osc levels
  float lev2;

  float target; //period target

  // What note triggered this voice, or SUSTAIN when the key is released
  // but the sustain pedal is held down.
  int note;
};

class JX10AudioProcessor : public juce::AudioProcessor,
                           private juce::ValueTree::Listener
{
public:
  JX10AudioProcessor();
  ~JX10AudioProcessor() override;

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

  // The factory presets.
  std::vector<JX10Program> _programs;

  // Index of the active preset.
  int _currentProgram;

  // The current sample rate and 1 / sample rate.
  float _sampleRate, _inverseSampleRate;

  // MIDI note on / note off events for the current block. Each event is
  // described by 3 values: delta time, note number, velocity.
  #define EVENTBUFFER 120
  int _notes[EVENTBUFFER + 8];

  // Special event code that marks the end of the MIDI events list.
  #define EVENTS_DONE 99999999

  // Special "note number" that says this voice is now kept alive by the
  // sustain pedal being pressed down. As soon as the pedal is released,
  // this voice will fade out.
  #define SUSTAIN -1

  // List of the active voices.
  Voice _voices[NVOICES] = { 0 };

  // How many voices are currently in use.
  int _numActiveVoices;

  // === MIDI CC values ===

  // Status of the damper pedal: 64 = pressed, 0 = released.
  int _sustain;

  // Output gain in linear units. Can be changed by MIDI CC 7.
  float _volume;

  // Modulation wheel value.
  float _modWheel;

  // Setting of the filter cutoff controller (MIDI CC 0x02, 0x03, or 0x4a).
  float _filterCtl;

  // Setting of the resonance controller (MIDI CC 0x10 or 0x47).
  float _resonanceCtl;

  // Amount of channel aftertouch.
  float _pressure;

  // Pitch bend value, and its inverse.
  float _pitchBend, _inversePitchBend;

  // === Parameter values ===

  // Mono / poly / glide mode.
  // 0, 1: POLY
  //    2: P-LEGATO
  //    3: P-GLIDE
  // 4, 5: MONO
  //    6: M-LEGATO
  //    7: M-GLIDE
  int _mode;

  // How much oscillator 2 is mixed into the sound; 0.0 = osc 2 is silent,
  // 1.0 = osc2 has same level as osc 1.
  float _oscMix;

  // Amount of detuning for oscillator 2.
  float _detune;

  // Master tuning.
  float _tune;

  float _glide, _glidedisp;

  // Low-pass filter settings.
  float _filterCutoff, _filterQ;

  float _filterEnv;

  float _filterLFO;

  float _filterVelocitySensitivity;

  bool _ignoreVelocity;

  // Filter ADSR settings. TODO: units
  float _filterAttack, _filterDecay, _filterSustain, _filterRelease;

  // Amplitude ADSR settings.
  float _envAttack, _envDecay, _envSustain, _envRelease;

  // Current LFO value and phase increment.
  float _lfo, _lfoInc;

  float _noiseMix;

  float _volumeTrim;

  float _vibrato, _pwmdep;

  // === Synthesizer state ===

//TODO: what is this?
  #define KMAX 32

  float fzip;
  int K;
  int lastnote;

  // Pseudo random number generator.
  unsigned int _noiseSeed;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JX10AudioProcessor)
};
