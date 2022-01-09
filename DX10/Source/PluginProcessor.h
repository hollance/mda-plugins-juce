#pragma once

#include <JuceHeader.h>

#define NPARAMS 16       // number of parameters
#define NVOICES 8        // max polyphony

#define SILENCE 0.0003f  // voice choking

// Describes a factory preset.
struct DX10Program
{
  DX10Program(const char *name,
              float p0,  float p1,  float p2,  float p3,
              float p4,  float p5,  float p6,  float p7,
              float p8,  float p9,  float p10, float p11,
              float p12, float p13, float p14, float p15);
  char name[24];
  float param[NPARAMS];
};

// State for an active voice.
struct Voice
{
  // What note triggered this voice, or SUSTAIN when the key is released
  // but the sustain pedal is still held down. 0 if the voice is inactive.
  int note;

  // Carrier oscillator
  float car;   // current phase value
  float dcar;  // phase increment

  // Modulator sine oscillator
  float dmod;  // phase increment
  float mod0;
  float mod1;

  // Carrier envelope
  float env;   // current envelope level
  float cenv;  // smoothed envelope that includes the attack portion
  float catt;  // smoothing coefficient for attack
  float cdec;  // decay mutiplier

  // Modulator envelope
  float menv;  // current envelope level
  float mlev;  // target level
  float mdec;  // decay multiplier
};

class DX10AudioProcessor : public juce::AudioProcessor,
                           private juce::ValueTree::Listener
{
public:
  DX10AudioProcessor();
  ~DX10AudioProcessor() override;

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
  std::vector<DX10Program> _programs;

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
  #define SUSTAIN 128

  // List of the active voices.
  Voice _voices[NVOICES] = { 0 };

  // How many voices are currently in use.
  int _numActiveVoices;

  // The LFO only updates every 100 samples. This counter keeps track of when
  // the next update is.
  int _lfoStep;

  // Used by the LFO to approximate a sine wave.
  float _lfo0, _lfo1;

  // Current amount of mod wheel + vibrato modulation. Because the LFO is only
  // updated every 100 samples, we need to keep track of this across calls to
  // processBlock().
  float _modulationAmount;

  // === Parameter values ===

  // Tuning: number of octaves up or down.
  float _tune;

  // Fine-tuning: between -1.0 and +1.0 semitones (or -100 to +100 cents).
  float _fineTune;

  // Modulator ratio as a multiple of the carrier frequency.
  float _ratio;

  // Carrier envelope settings.
  float _attack, _decay, _release;

  // Modulator envelope settings.
  float _modInitialLevel, _modDecay, _modSustain, _modRelease;

  // Velocity sensitivity for the modulator envelope (for brightness).
  float _velocitySensitivity;

  // The amount of vibrato to apply.
  float _vibrato;

  // Amount of waveshaping to add extra harmonics.
  float _richness;

  // How much to mix the modulator waveform into the final sound by itself.
  // Normally the modulator is only used to change the carrier, but for some
  // extra snazz you can make the modulator waveform audible as well.
  float _modMix;

  // Phase increment for the LFO.
  float _lfoInc;

  // === MIDI CC values ===

  // Status of the damper pedal: 64 = pressed, 0 = released.
  int _sustain;

  // Output gain in linear units. Can be changed by MIDI CC 7.
  float _volume;

  // Modulation wheel value. Used to add more vibrato.
  float _modWheel;

  // Pitch bend value.
  float _pitchBend;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DX10AudioProcessor)
};
