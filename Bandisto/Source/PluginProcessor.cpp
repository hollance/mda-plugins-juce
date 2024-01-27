#include "PluginProcessor.h"

MDABandistoAudioProcessor::MDABandistoAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MDABandistoAudioProcessor::~MDABandistoAudioProcessor()
{
}

const juce::String MDABandistoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int MDABandistoAudioProcessor::getNumPrograms()
{
    return 1;
}

int MDABandistoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MDABandistoAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDABandistoAudioProcessor::getProgramName(int index)
{
    return {};
}

void MDABandistoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDABandistoAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = float(newSampleRate);
    resetState();
}

void MDABandistoAudioProcessor::releaseResources()
{
}

void MDABandistoAudioProcessor::reset()
{
    resetState();
}

bool MDABandistoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDABandistoAudioProcessor::resetState()
{
    fb1 = 0.0f;
    fb2 = 0.0f;
    fb3 = 0.0f;
}

void MDABandistoAudioProcessor::update()
{
    float param4 = apvts.getRawParameterValue("L Dist")->load();
    float param5 = apvts.getRawParameterValue("M Dist")->load();
    float param6 = apvts.getRawParameterValue("H Dist")->load();

    // The drive parameter follows a rather dramatic curve from 0.1 to 100000.
    // On the UI this is shown as 0 - 60 dB, which I don't think corresponds
    // to what actually happens.
    driv1 = std::pow(10.0f, 6.0f * param4 * param4 - 1.0f);
    driv2 = std::pow(10.0f, 6.0f * param5 * param5 - 1.0f);
    driv3 = std::pow(10.0f, 6.0f * param6 * param6 - 1.0f);

    // Calculate gain based on bipolar (transistor) or unipolar (value) mode.
    // In unipolar mode, the gain is 0.5 because the mid-side signal is 6 dB
    // too loud, and so this compensates for that. In bipolar mode, the higher
    // the drive is for a band, the larger the gain for that band is too, from
    // 0.3 all the way up to 3000! This is because at high drive, the output
    // from the waveshaper is tiny and so it needs a big boost.
    valve = int(apvts.getRawParameterValue("Mode")->load());
    if (valve) {  // valve (tube)
        trim1 = 0.5f;
        trim2 = 0.5f;
        trim3 = 0.5f;
    } else {  // transistor
        trim1 = 0.3f * std::pow(10.0f, 4.0 * std::pow(param4, 3.0f));
        trim2 = 0.3f * std::pow(10.0f, 4.0 * std::pow(param5, 3.0f));
        trim3 = 0.3f * std::pow(10.0f, 4.0 * std::pow(param6, 3.0f));
    }

    float param7 = apvts.getRawParameterValue("L Out")->load();
    float param8 = apvts.getRawParameterValue("M Out")->load();
    float param9 = apvts.getRawParameterValue("H Out")->load();

    // The output level goes from -20 dB to 20 dB. Convert to linear gain
    // and combine it with the gain that is based on the drive.
    trim1 *= std::pow(10.0f, 2.0f * param7 - 1.0f);
    trim2 *= std::pow(10.0f, 2.0f * param8 - 1.0f);
    trim3 *= std::pow(10.0f, 2.0f * param9 - 1.0f);

    // Determine which band will be output by setting the levels of the
    // other bands to zero. In "Output" mode, all bands are included.
    int param1 = int(apvts.getRawParameterValue("Listen")->load());
    switch (param1) {
        case 0: trim2 = 0.0f; trim3 = 0.0f; sideLevel = 0.0f; break;
        case 1: trim1 = 0.0f; trim3 = 0.0f; sideLevel = 0.0f; break;
        case 2: trim1 = 0.0f; trim2 = 0.0f; sideLevel = 0.0f; break;
        default: sideLevel = 0.5f; break;
    }

    // Calculate the coefficients for the crossover filters. The actual
    // frequencies in Hz depend on the sample rate.
    float param2 = apvts.getRawParameterValue("L <> M")->load();
    float param3 = apvts.getRawParameterValue("M <> H")->load();
    fi1 = std::pow(10.0f, param2 - 1.70f);
    fi2 = std::pow(10.0f, param3 - 1.05f);
    fo1 = 1.0f - fi1;
    fo2 = 1.0f - fi2;
}

void MDABandistoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float a = in1[i];
        float b = in2[i];

        // Keep stereo component for later. This side signal won't be distorted.
        float s = (a - b) * sideLevel;

        // Create mono signal (mids). Only these mids will be distorted.
        // Also add a small DC offset. The original comment said "dope filter",
        // so I figured this made the filter extra dope but they probably meant
        // doping as in adding small "impurities" that give the filter something
        // to do?! Not sure it actually matters.
        a += b + 0.00002f;

        // Crossover filters. Three basic 6 dB/oct one-pole low-pass filters in
        // series. Their frequency responses look a little funky, but `l + m + h`
        // does reconstruct the signal, since `fb3 + (fb2 - fb3) + (a - fb2) = a`.
        fb2 = fi2 * a   + fo2 * fb2;
        fb1 = fi1 * fb2 + fo1 * fb1;
        fb3 = fi1 * fb1 + fo1 * fb3;

        float l = fb3;       // low band
        float m = fb2 - l;   // mid band
        float h = a - fb2;   // high band

        // Distort. First rectify the signal so it's always positive, then use
        // the magnitude to calculate a gain for each band. Because the formula
        // is 1 / (1 + drive * amplitude), the higher the amplitude, the lower
        // the gain, so this compresses the louder parts of the signal. And the
        // higher the drive, the more extreme this compression is, eventually
        // turning the distortion into hard clipping.
        float g1 = (l > 0.0f) ? l : -l;
        g1 = 1.0f / (1.0f + driv1 * g1);

        float g2 = (m > 0.0f) ? m : -m;
        g2 = 1.0f / (1.0f + driv2 * g2);

        float g3 = (h > 0.0f) ? h : -h;
        g3 = 1.0f / (1.0f + driv3 * g3);

        /*
            To plot the output of this waveshaper in something like Desmos,
            use the following:

            // the drive parameter
            k: a number between 0 and 1

            // the drive value (0.1 - 100000)
            d = 10^(6k^2 - 1)

            // the gain compensation value (trim)
            t = 0.3 * 10^(4 * k^3)

            // the waveshaper formula
            x * t / (1 + d * x)  { 0 <= x <= 1 }

            This plots the transfer curve. You'll see that for low values of
            `k`, the line is straight. But for higher values of `k` the curve
            will bend and eventually drops in height.
        */

        // In unipolar mode, only distort samples with a negative polarity.
        if (valve) {
            if (l > 0.0f) { g1 = 1.0f; }
            if (m > 0.0f) { g2 = 1.0f; }
            if (h > 0.0f) { g3 = 1.0f; }
        }

        // Apply the distortion to each band and recombine the bands.
        a = l*g1*trim1 + m*g2*trim2 + h*g3*trim3;

        // Add the side signal back in to restore the stereo nature.
        out1[i] = a + s;
        out2[i] = a - s;
    }
}

juce::AudioProcessorEditor *MDABandistoAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void MDABandistoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDABandistoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDABandistoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Listen", 1),
        "Listen",
        juce::StringArray { "Low", "Mid", "High", "Output" },
        3));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("L <> M", 1),
        "L <> M",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.4f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([this](float value, int)
            {
                // Convert the filter coefficient to a frequency in Hz.
                // Not sure where this formula comes from, it might just
                // be an approximation of the actual cutoff frequency.
                float fi1 = std::pow(10.0f, value - 1.7f);
                float hz = sampleRate * fi1 * (0.098f + 0.09f*fi1 + 0.5f*std::pow(fi1, 8.2f));
                return juce::String(int(hz));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("M <> H", 1),
        "M <> H",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([this](float value, int)
            {
                float fi2 = std::pow(10.0f, value - 1.05f);
                float hz = sampleRate * fi2 * (0.015f + 0.15f*fi2 + 0.9f*std::pow(fi2, 8.2f));
                return juce::String(int(hz));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("L Dist", 1),
        "L Dist",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(60.0f * value));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("M Dist", 1),
        "M Dist",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(60.0f * value));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("H Dist", 1),
        "H Dist",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(60.0f * value));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("L Out", 1),
        "L Out",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(40.0f * value - 20.0f));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("M Out", 1),
        "M Out",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(40.0f * value - 20.0f));
            })));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("H Out", 1),
        "H Out",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([this](float value, int)
            {
                return juce::String(int(40.0f * value - 20.0f));
            })));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("Mode", 1),
        "Mode",
        juce::StringArray { "Bipolar", "Unipolar" },
        3));

    return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MDABandistoAudioProcessor();
}
