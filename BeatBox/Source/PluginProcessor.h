#pragma once

#include <JuceHeader.h>

class MDABeatBoxAudioProcessor : public juce::AudioProcessor
{
public:
    MDABeatBoxAudioProcessor();
    ~MDABeatBoxAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
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
    void synth();

    float sampleRate;

    float hthr;           // hi-hat threshold
    float hfil;           // high-pass filter state
    float hlev;           // hat mix
    int hdel;             // minimum delay in samples between successive hi-hats

    float kthr;           // kick threshold
    float ksf1, ksf2;     // filter coeffs
    float ksb1, ksb2;     // filter state
    float klev;           // kick mix
    int kdel;             // minimum delay in samples between successive kicks

    float kww;            // Kik Trig parameter
    int ksfx;             // if 0, normal operation; if > 0, key listen mode

    float sthr;           // snare threshold
    float sf1, sf2, sf3;  // filter coeffs
    float sb1, sb2;       // filter state
    float slev;           // snare mix
    int sdel;             // minimum delay in samples between successive snares

    float ww;             // Snr Trig parameter
    int sfx;              // if 0, normal operation; if > 0, key listen mode

    float dyne;           // current envelope level
    float dyna;           // envelope attack
    float dynr;           // envelope release

    float dynm;           // Dynamics parameter
    float mix;            // Thru Mix parameter

    std::unique_ptr<float[]> hbuf;   // buffer containing hi-hat sound
    std::unique_ptr<float[]> kbuf;   // kick sound
    std::unique_ptr<float[]> sbufL;  // snare is stereo
    std::unique_ptr<float[]> sbufR;

    int hbuflen;          // hi-hat buffer length
    int kbuflen;          // kick buffer length
    int sbuflen;          // snare buffer length

    int hbufpos;          // current position in hi-hat buffer
    int kbufpos;          // in kick buffer
    int sbufpos;          // in snare buffer

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDABeatBoxAudioProcessor)
};
