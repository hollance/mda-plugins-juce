#include "PluginProcessor.h"

MDADegradeAudioProcessor::MDADegradeAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDADegradeAudioProcessor::~MDADegradeAudioProcessor()
{
}

const juce::String MDADegradeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDADegradeAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDADegradeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDADegradeAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDADegradeAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDADegradeAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDADegradeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetState();
}

void MDADegradeAudioProcessor::releaseResources()
{
}

void MDADegradeAudioProcessor::reset()
{
    resetState();
}

bool MDADegradeAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDADegradeAudioProcessor::resetState()
{
    _accum = 0.0f;
    _currentSample = 0.0f;
    _buf1 = _buf2 = _buf3 = _buf4 = _buf6 = _buf7 = _buf8 = _buf9 = 0.0f;
    _sampleIndex = 1;
}

float MDADegradeAudioProcessor::filterFreq(float hz)
{
    /*
      The filter is a one-pole filter: y(n) = b*x(n) + a*y(n - 1).

      This function calculates the value of the coefficient a, given a cutoff
      frequency. And then we set b = (1 - a) to get unity gain.

      I'm not sure where the formula used here comes from. It does not give a
      response that has -3 dB at the cutoff frequency.

      The formula that does give a -3 dB cutoff is:
            k = 1.0 - cos(2.0 * pi * hz / sampleRate)
            a = 1 + k - sqrt(k * k + 2.0 * k)

      Another way to calculate the coefficient is the approximation:
            a = exp(-2 * pi * hz / sampleRate)
     */
    float r = 0.999f;
    float j = r * r - 1.0f;
    float k = 2.0f - 2.0f * r * r * std::cos(0.647f * hz / float(getSampleRate()));
    return (std::sqrt(k * k - 4.0f * j * j) - k) / (2.0f * j);
}

void MDADegradeAudioProcessor::update()
{
    // The sample interval is a number between 1 and 10. If 1, we read from the
    // input buffer at the project's sample rate. If the interval is 2, we skip
    // every other sample, cutting the effective sample rate in half; and so on.
    _sampleInterval = apvts.getRawParameterValue("Sample Rate")->load();

    // In sample-and-hold mode, we do read every single sample but add them up
    // over the sampleInterval and take the mean once we have enough samples.
    _mode = apvts.getRawParameterValue("Sample Mode")->load();

    // Headroom: convert from decibels to a linear value.
    float headroom = apvts.getRawParameterValue("Headroom")->load();
    _clip = juce::Decibels::decibelsToGain(headroom);

    // Filtering coefficients. The filter does: y(n) = fi * x(n) + fo * y(n - 1).
    // fo is the coefficient for the feedback loop, fi controls the overall gain.
    // We choose fi so that it keeps the amplitude of the unfiltered frequencies
    // at 0 dB (unity gain). Since fi = 1 - fo, that makes this an "exponentially
    // weighted moving average" or EWMA filter.
    _fo = filterFreq(apvts.getRawParameterValue("PostFilter")->load());
    _fi = 1.0f - _fo;

    // The filter stage actually consists of several identical filters in series.
    // As an optimization, make the gain coefficient (1 - fo)^4, which will save
    // doing a few multiplications.
    _fi = _fi * _fi;
    _fi = _fi * _fi;

    // The formula used for quantization is:
    //   x = int(x * 2^bits) / (2^bits) = int(x * g1) / g1 = int(x * g1) * g2
    //
    // Note that g2 = 1.0f / (2.0f * g1) instead of just 1.0f / g1. That extra
    // 1/2 compensates for the signal being twice as loud because we will convert
    // from stereo to mono by adding up both channels.
    //
    // The original code had a bug here, where it went from 2 - 14 bits instead
    // of the 4 - 16 bits that was intended.
    int numBits = apvts.getRawParameterValue("Quantize")->load();
    _g1 = float(1 << numBits);
    _g2 = 1.0f / (2.0f * _g1);

    // In sample-and-hold mode, the samples get accumulated over the interval,
    // which means the accumulator can grow very large. To take the mean value,
    // we divide by the number of samples added. We can do this in g1 and save
    // an extra instruction in the processing loop.
    if (_mode == 1.0f) {
        _g1 /= float(_sampleInterval);
    }

    // Note: the original plug-in made g1 negative which serves no purpose except
    // to invert the signal's phase. Not sure if that was intended or not; enable
    // it by uncommenting the line below.
    // _g1 = -_g1;

    // The output level is in dB, so convert to linear gain.
    float outputLevel = apvts.getRawParameterValue("Output")->load();
    _g3 = juce::Decibels::decibelsToGain(outputLevel);

    // Non-linearity: 0 = x^1 ... 1 = x^0.707. The plug-in uses an exponential
    // curve between these two points but a linear interpolation using jmap()
    // between 1 and 1/sqrt(2) would give virtually the same result.
    float nonlinearity = apvts.getRawParameterValue("Non-Linearity")->load();
    _linNeg = std::pow(10.0f, -0.15f * nonlinearity);

    // If the mode is odd, the non-linearity is not applied to positive samples.
    if (apvts.getRawParameterValue("Non-Lin Mode")->load() == 0.0f) {
        _linPos = 1.0f;
    } else {
        _linPos = _linNeg;
    }
}

void MDADegradeAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    update();

    const float *in1 = buffer.getReadPointer(0);
    const float *in2 = buffer.getReadPointer(1);
    float *out1 = buffer.getWritePointer(0);
    float *out2 = buffer.getWritePointer(1);

    // Make local copies for moar speeed!
    const float linNeg = _linNeg;
    const float linPos = _linPos;
    const float clip = _clip;
    const float mode = _mode;
    const float fi = _fi, fo = _fo;
    const float g1 = _g1, g2 = _g2, g3 = _g3;
    const int sampleInterval = _sampleInterval;

    int sampleIndex = _sampleIndex;
    float accum = _accum;
    float x = _currentSample;
    float b1 = _buf1, b2 = _buf2, b3 = _buf3, b4 = _buf4,
    b6 = _buf6, b7 = _buf7, b8 = _buf8, b9 = _buf9;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        /*
          Order of the FX:

          1. Convert stereo to mono
          2. Skip samples depending on the sampling interval
          3. Quantize
          4. Apply non-linearity x^lin
          5. Headroom clipping
          6. Apply output gain
          7. Filter
         */

        // Combine the stereo channels into a single mono value.
        // In regular sample mode (mode = 0.0), accum holds only the most
        // recent sample value. If sample-and-hold is active (mode = 1.0), we
        // add the new value to accum so that it holds the sum of the previous
        // samples in this sampling interval. Multiplying by mode (0 or 1) is
        // simpler and possibly faster than using an if-statement.
        accum = in1[i] + in2[i] + mode * accum;

        // Time to take a new sample?
        // The original code used == to check if the sample interval is over,
        // but sampleInterval can change when the user adjusts the parameters.
        // Now the current index may end up being larger than the interval, so
        // we use >= to do the check.
        if (sampleIndex >= sampleInterval) {

            // Quantize. In sample-and-hold mode, this also takes the mean over
            // the sampling interval (in g1). It also divides by 2 (in g2) to
            // compensate for the stereo -> mono conversion which made the signal
            // twice as loud.
            x = float(g2 * int(accum * g1));

            // Apply the non-linearity, followed by headroom clipping.
            if (x > 0.0f) {
                x = std::pow(x, linPos);
                if (x > clip) x = clip;
            } else {
                x = -std::pow(-x, linNeg);
                if (x < -clip) x = -clip;
            }

            // Reset the accumulator.
            accum = 0.0f;
            sampleIndex = 0;
        }
        sampleIndex += 1;

        // Apply output gain (g3) and then filter.
        // The filter is eight identical first-order feedback filters in a row.
        // Each filter does: y(n) = fi * x(n) + fo * y(n - 1) where fi = 1 - fo.
        // In the implementation below only the first filter (b1) and the fifth
        // filter (b6) actually apply the fi coefficient, but the coefficient
        // they're using is (1 - fo)^4. This saves some multiplications but is
        // otherwise equivalent. You could also do (1 - fo)^8 and only apply it
        // to b1, not b6, but that can get numerically unstable when fo is large.
        b1 = fi * (x * g3) + fo * b1;
        b2 =       b1      + fo * b2;
        b3 =       b2      + fo * b3;
        b4 =       b3      + fo * b4;
        b6 = fi *  b4      + fo * b6;
        b7 =       b6      + fo * b7;
        b8 =       b7      + fo * b8;
        b9 =       b8      + fo * b9;

        // If you're wondering what happened to b5, that's the name the original
        // code used for x, but I found this confusing. b9 is the final output of
        // the filter stage. The other b1-b8 are the intermediate output values
        // for the individual filters and also their delay units (z^-1).

        out1[i] = b9;
        out2[i] = b9;
    }

    // Reset the state if we have numeric underflow in the output.
    if (std::abs(b1) < 1.0e-10f) {
        _buf1 = 0.0f; _buf2 = 0.0f; _buf3 = 0.0f; _buf4 = 0.0f;
        _buf6 = 0.0f; _buf7 = 0.0f; _buf8 = 0.0f; _buf9 = 0.0f;
        _accum = 0.0f;
        _currentSample = 0.0f;
    } else {
        // Copy the local variables back into the member variables, so we can
        // resume from where we left off in the next call to processBlock.
        _buf1 = b1; _buf2 = b2; _buf3 = b3; _buf4 = b4;
        _buf6 = b6; _buf7 = b7; _buf8 = b8; _buf9 = b9;

        // We also keep track of x and accum in between calls to processBlock,
        // in case the sampleInterval is greater than 1.
        _accum = accum;
        _currentSample = x;
    }
    _sampleIndex = sampleIndex;
}

juce::AudioProcessorEditor *MDADegradeAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDADegradeAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDADegradeAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDADegradeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Headroom", 1),
        "Headroom",
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.01f),
        -6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("Quantize", 1),
        "Quantize",
        4, 16, 10,
        juce::AudioParameterIntAttributes().withLabel("bits")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Sample Rate", 1),
        "Sample Rate",
        juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f),
        3.0f));

    // Note: In the original plug-in, the sample rate parameter was shown as
    // getSampleRate() / sampleInterval. For an interval of 1, it would show
    // 44100; for an interval of 2, it showed 22050; and so on, depending on
    // the project's actual sample rate. We simply display the sample interval
    // but a custom UI could display the effective sample rate.

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Sample Mode", 1),
        "Sample Mode",
        juce::StringArray({ "Sample", "Hold" }),
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("PostFilter", 1),
        "PostFilter",
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.5f),
        10000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Non-Linearity", 1),
        "Non-Linearity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.58f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Non-Lin Mode", 1),
        "Non-Lin Mode",
        juce::StringArray({ "Odd", "Even" }),
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Output", 1),
        "Output",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDADegradeAudioProcessor();
}
