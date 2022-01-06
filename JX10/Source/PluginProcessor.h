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
  // What note triggered this voice, or SUSTAIN when the key is released
  // but the sustain pedal is held down.
  int note;

  // The "period" of the waveform in samples. This is actually only half the
  // period due to the way the oscillators are implemented: they count up for
  // `period` samples and then down for `period` samples.
  float period;   // current period, which may be gliding up to target
  float target;   // for gliding

  // Amount of detuning for osc 2.
  float detune;

  // Oscillator 1
  float p1;       // sinc position
  float pmax1;    // loop length
  float dp1;      // delta
  float sin01;    // sine oscillator
  float sin11;
  float sinx1;
  float dc1;      // dc offset
  float lev1;     // amplitude

  // Oscillator 2
  float p2;       // sinc position
  float pmax2;    // loop length
  float dp2;      // delta
  float sin02;    // sine oscillator
  float sin12;
  float sinx2;
  float dc2;      // dc offset
  float lev2;     // amplitude

  // Used to integrate the outputs from the oscillators to produce a saw wave.
  float saw;

  float fc;      // filter cutoff TODO
  float ff;      // modulated filter cutoff TODO

  // Filter delay units
  float f0;      // low-pass output
  float f1;      // band-pass output
  float f2;      // x[n - 1]

  // Amplitude envelope
  float env;     // current envelope level
  float envd;    // decay multiplier
  float envl;    // target level

  // Filter envelope
  float fenv;    // current envelope level
  float fenvd;   // decay multiplier
  float fenvl;   // target level
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

  // Used to smoothen changes in filter cutoff frequency.
  float _filterZip;

  // How often we update the LFO, in samples.
  const int LFO_MAX = 32;

  // The LFO only updates every 32 samples. This counter keep track of when
  // the next update is.
  int _lfoStep;

  // Most recent note that was played. Used for gliding.
  int _lastNote;

  // Pseudo random number generator.
  unsigned int _noiseSeed;

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

  // Amount of detuning for oscillator 2. This is a multiplier for the period
  // of the oscillator.
  float _detune;

  // Master tuning.
  float _tune;

  // Coefficient for the speed of the glide. 1.0 is instantaneous (no glide).
  float _glideRate;

  // Number of semitones to glide up or down into any new note. This is used
  // even if not in a LEGATO or GLIDE mode.
  float _glideBend;

  // Low-pass filter settings.
  float _filterCutoff, _filterQ;

  // LFO intensity for the filter cutoff.
  float _filterLFODepth;

  // Envelope intensity for the filter cutoff.
  float _filterEnvDepth;

  float _filterVelocitySensitivity;

  // If this is set, velocity sensitivity is completely off and all notes will
  // be played with the same velocity.
  bool _ignoreVelocity;

  // Filter ADSR settings.
  float _filterAttack, _filterDecay, _filterSustain, _filterRelease;

  // Amplitude ADSR settings.
  float _envAttack, _envDecay, _envSustain, _envRelease;

  // Current LFO value and phase increment.
  float _lfo, _lfoInc;

  // Gain for mixing the noise into the output.
  float _noiseMix;

  // Used to keep the output gain constant after changing parameters.
  float _volumeTrim;

  // LFO intensity for vibrato and PWM.
  float _vibrato, _pwmDepth;

  // === MIDI CC values ===

  // Status of the damper pedal: 64 = pressed, 0 = released.
  int _sustain;

  // Output gain in linear units. Can be changed by MIDI CC 7.
  float _volume;

  // Modulation wheel value. Sets the modulation depth for vibrato / PWM.
  float _modWheel;

  // MIDI CC amount used to modulate the cutoff frequency.
  float _filterCtl;

  // MIDI CC amount used to modulate the filter Q.
  float _resonanceCtl;

  // Amount of channel aftertouch. This is used to modulate the filter cutoff.
  float _pressure;

  // Pitch bend value, and its inverse. Also used to modulate the filter.
  float _pitchBend, _inversePitchBend;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JX10AudioProcessor)
};
