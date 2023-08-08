#pragma once

#include <JuceHeader.h>

class MDALoudnessAudioProcessor : public juce::AudioProcessor
{
public:
    MDALoudnessAudioProcessor();
    ~MDALoudnessAudioProcessor() override;

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

    float z0, z1, z2, z3;  // filter delays (0+1 = left channel, 2+3 = right)
    float a0, a1, a2;      // filter coefficients
    float gain;            // output gain
    int mode;              // 0 = cut, 1 = boost

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDALoudnessAudioProcessor)
};
