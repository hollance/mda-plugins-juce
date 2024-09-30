#pragma once

#include <JuceHeader.h>

class MDASplitterAudioProcessor : public juce::AudioProcessor
{
public:
    MDASplitterAudioProcessor();
    ~MDASplitterAudioProcessor() override;

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

    float freq;                // filter coefficient
    float a0, a1, b0, b1;      // filter states (a = left, b = right channel)

    float level;               // gate threshold
    float env;                 // current envelope level
    float att, rel;            // attack and release constants

    float ff, ll, pp;          // routing: freq, level, polarity
    float i2l, i2r, o2l, o2r;  // routing: gain for left/right dry&wet

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDASplitterAudioProcessor)
};
