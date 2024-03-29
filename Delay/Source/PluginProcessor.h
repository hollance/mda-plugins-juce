#pragma once

#include <JuceHeader.h>

class MDADelayAudioProcessor : public juce::AudioProcessor
{
public:
    MDADelayAudioProcessor();
    ~MDADelayAudioProcessor() override;

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

    // Maps the position of the right delay slider to a percentage of the left
    // channel delay time. Moving the slider to the left gives you a variable
    // ratio between 0% and 200%. Moving the slider to the right chooses from a
    // set of fixed ratios. At the center position, the ratio is 200%.
    static float rightDelayRatio(float param);

    // Maximum delay time in milliseconds. Feel free to make this smaller or
    // larger. The original plug-in used a fixed number of samples, but that
    // would make the maximum delay time different if the sample rate changed.
    static constexpr float _delayMaxMsec = 500.0f;

    // Maximum length of the delay buffer in samples.
    int _delayMax;

    // This buffer stores the delayed samples. There is only one delay buffer,
    // which means any echos from the delayed signal are actually mono.
    std::vector<float> _delayBuffer;

    // Delay time in samples for the left & right channels.
    int _ldel, _rdel;

    // Write position in the delay buffer. This is where we will write the next
    // new sample value.
    int _pos;

    // Wet & dry mix.
    float _wet, _dry;

    // Amount of echo feedback.
    float _feedback;

    // Low & high mix for the crossover filter.
    float _lmix, _hmix;

    // Low-pass filter coefficient.
    float _filt;

    // Delay unit for the low-pass filter.
    float _filt0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MDADelayAudioProcessor)
};
