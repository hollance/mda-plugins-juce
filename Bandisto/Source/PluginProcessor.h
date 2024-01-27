#pragma once

#include <JuceHeader.h>

class MDABandistoAudioProcessor : public juce::AudioProcessor
{
public:
    MDABandistoAudioProcessor();
    ~MDABandistoAudioProcessor() override;

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

    float sampleRate;
    float driv1, trim1;   // drive and gain for low band
    float driv2, trim2;   // ... mid band
    float driv3, trim3;   // ... high band
    float fi1, fo1;       // filter coefficients
    float fi2, fo2;
    float fb1, fb2, fb3;  // filter delays
    float sideLevel;      // output level for the stereo data
    int valve;            // 1 if unipolar mode, 0 if bipolar

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDABandistoAudioProcessor)
};
