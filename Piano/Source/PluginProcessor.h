#pragma once

#include <JuceHeader.h>

const int NPARAMS = 12;       // number of parameters
const int NPROGS = 8;         // number of programs
const int NVOICES = 32;       // max polyphony

const float SILENCE = 0.0001f;  // voice choking

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

// A keygroup maps notes to waveforms. All the waveforms are stored inside one
// big lookup table (see mdaPianoData.h). There are 15 keygroups, each covering
// 4 semitones. That's 60 notes in total. A piano has 88 keys so very low notes
// all share the same waveform; likewise for very high notes. The waveforms are
// mono, 22050 Hz, and consist of an attack portion and a loop portion.
struct Keygroup
{
    int root;  // MIDI root note (usually the note in the middle of the keygroup)
    int high;  // highest note this waveform should be used for
    int pos;   // index of the first sample in the lookup table
    int end;   // index of the last sample in the lookup table
    int loop;  // how far from end the first sample of the loop is
};

// State for an active voice.
struct Voice
{
    // What note triggered this voice, or SUSTAIN when the key is released
    // but the sustain pedal is held down.
    int note;

    // The increment for stepping through the waveform table. This is stored as
    // a fixed point value, where 65536 = 1.0. For example, a step size of 0.5
    // has a delta of 32768.
    int delta;

    // The fractional part of the current position in the waveform. This is
    // stored using 16 bits. The highest value it can have is 65535, which is
    // approximately 0.99999.
    int frac;

    // These are copied from the keygroup for this note. When we start playing
    // the note, pos is incremented by delta until it reaches end, and then it
    // wraps back to the loop index.
    int pos;
    int end;
    int loop;

    // The current envelope level and the exponential decay value that the
    // envelope is multiplied with on every step. Setting the decay to 0.99
    // will fade out the sound almost immediately (used for all notes off).
    float env;
    float decay;

    // "Muffling" filter. This is a first-order LPF.
    float f0;  // delay for y(n - 1)
    float f1;  // delay for x(n - 1)
    float ff;  // filter coefficient

    // Panning volumes for the left and right channels.
    float outl;
    float outr;
};

class MDAPianoAudioProcessor : public juce::AudioProcessor
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

    void update();
    void resetState();

    void createPrograms();
    void processEvents(juce::MidiBuffer &midiMessages);
    void noteOn(int note, int velocity);

    // The factory presets.
    std::vector<MDAPianoProgram> _programs;

    // Index of the active preset.
    int _currentProgram;

    // The current sample rate and 1 / sample rate.
    float _sampleRate, _inverseSampleRate;

    // MIDI note on / note off events for the current block. Each event is
    // described by 3 values: delta time, note number, velocity.
    static const int EVENTBUFFER = 120;
    int _notes[EVENTBUFFER + 8];

    // Special event code that marks the end of the MIDI events list.
    const int EVENTS_DONE = 99999999;

    // Special "note number" that says this voice is now kept alive by the
    // sustain pedal being pressed down. As soon as the pedal is released,
    // this voice will fade out.
    const int SUSTAIN = 128;

    // Big lookup table with waveforms containing the piano samples.
    short *_waves;

    // Maps the waveforms from the _waves lookup table to ranges of notes.
    Keygroup _keygroups[15];

    // List of the active voices.
    Voice _voices[NVOICES];

    // How many voices are currently in use.
    int _numActiveVoices;

    // Status of the damper pedal: 64 = pressed, 0 = released.
    int _sustain;

    // Output gain in linear units. Can be changed by MIDI CC 7.
    float _volume;

    // Max number of voices of polyphony.
    int _polyphony;

    // Envelope decay length (lower is shorter). Used when playing a new note.
    float _envDecay;

    // Value of the envelope release parameter, 0.0 - 1.0. This is used after a
    // note off event (if the sustain pedal is not pressed).
    float _envRelease;

    // Value of the muffling filter parameter, 0.0 - 1.0.
    float _mufflingFilter;

    // How sensitive the muffling filter is to note velocity, 0.0 - 5.0
    float _mufflingVelocity;

    // Adjusts the amount of filtering. This variable is changed when the mod
    // wheel or soft pedal is used. Default value = 160.
    float _muff;

    // These two settings change which waveform to use for a given note, by not
    // selecting the note's keygroup but one of its neighboring keygroups.
    int _hardnessOffset;
    float _hardnessVelocity;

    // Used to set the envelope level for new notes. Value between 0.25 and 3.0.
    float _velocitySensitivity;

    // Amount of fine-tuning, in semitones.
    float _fine;

    // Amount of random detuning.
    float _random;

    // Amount of stretch tuning.
    float _stretch;

    // For calculating the stereo width.
    float _width, _trim;

    // Delay line for comb filter. Used to enhance the stereo width.
    int _delayPos;
    int _delayMax;
    float _combDelay[256];

    // Amount of comb filtering. More means a wider stereo effect.
    float _comb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDAPianoAudioProcessor)
};
