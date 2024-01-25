#pragma once

#include <JuceHeader.h>

class MDADetuneAudioProcessor : public juce::AudioProcessor
{
public:
    MDADetuneAudioProcessor();
    ~MDADetuneAudioProcessor() override;

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

    static constexpr int BUFMAX = 4096;

    float buf[BUFMAX];  // circular buffer for delay line (mono)
    float win[BUFMAX];  // crossfade window

    float sampleRate;
    int buflen;         // delay length

    int pos0;           // write pointer in the circular buffer
    float pos1, dpos1;  // read pointer and step size for left channel
    float pos2, dpos2;  // and for right channel

    float wet, dry;     // output levels

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDADetuneAudioProcessor)
};
