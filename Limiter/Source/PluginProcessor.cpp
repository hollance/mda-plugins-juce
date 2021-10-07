#include "PluginProcessor.h"

MDALimiterAudioProcessor::MDALimiterAudioProcessor()
  : AudioProcessor(BusesProperties()
                   .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                   .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  apvts.state.addListener(this);
}

MDALimiterAudioProcessor::~MDALimiterAudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String MDALimiterAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDALimiterAudioProcessor::getNumPrograms()
{
  return 1;
}

int MDALimiterAudioProcessor::getCurrentProgram()
{
  return 0;
}

void MDALimiterAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MDALimiterAudioProcessor::getProgramName(int index)
{
  return {};
}

void MDALimiterAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDALimiterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  resetState();
  _parametersChanged.store(true);
}

void MDALimiterAudioProcessor::releaseResources()
{
}

void MDALimiterAudioProcessor::reset()
{
  resetState();
}

bool MDALimiterAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MDALimiterAudioProcessor::resetState()
{
  // Always start at maximum volume.
  _gain = 1.0f;
}

void MDALimiterAudioProcessor::update()
{
  _softKnee = apvts.getRawParameterValue("Knee")->load() == 1.0f;

  // The threshold parameter is in decibels. The formulas below assume this is
  // a value between 0 and 1, so ask APVTS for the normalized parameter value.
  float fParam1 = apvts.getParameter("Thresh")->getValue();

  // What the threshold means depends on the knee mode: in hard knee mode it's
  // literally the maximum amplitude the sound is allowed to have, but I'm not
  // sure exactly how to interpret it in soft knee mode. In both modes, moving
  // the slider to the left gives you more limiting than sliding to the right.
  // But it's not entirely clear to me how the numbers on the slider relate to
  // the actual amount of limiting that happens in soft knee mode -- the amount
  // of reduction is less than the dB value you see on the slider!
  if (_softKnee) {
    // Soft knee sets the threshold between 10 (+20 dB) and 0.1 (-20 dB), but
    // this isn't the actual level that the signal will be limited to.
    // Experiments have shown this results in a reduction of -20 dB when the
    // slider is to the left, about -6 dB when it's in the center, and only a
    // tiny bit when the slider is all the way to the right. I'm half convinced
    // the formula below contains a typo and that the 1.0 should be a 2.0,
    // which would give this a range of +40 dB to 0 dB, which makes more sense.
    _threshold = std::pow(10.0f, 1.0f - (2.0f * fParam1));
  } else {
    // Hard knee sets the threshold between 0.01 (-40 dB) and 1.0 (0 dB).
    // If you're wondering how this calculation works, it's the usual formula
    // for converting decibels to linear gain, 10^(dB/20), except the decibels
    // are already represented as a number between 0 and 1, where 0 = -40 dB.
    // See the README for more info.
    _threshold = std::pow(10.0f, (2.0f * fParam1) - 2.0f);
  }

  // The output level is in decibels; convert to a linear gain.
  float fParam2 = apvts.getRawParameterValue("Output")->load();
  _trim = juce::Decibels::decibelsToGain(fParam2);

  // The attack parameter goes between 0 and 1. Convert this into a value from
  // 1 to 0.01, where 1 means the attack is fast and 0.01 means it's slow.
  float fParam3 = apvts.getRawParameterValue("Attack")->load();
  _attack = std::pow(10.0f, -2.0f * fParam3);

  // The release parameter goes between 0 and 1. Convert this into a value from
  // 0.01 to 0.00001. Like with the attack, a smaller number means slower.
  float fParam4 = apvts.getRawParameterValue("Release")->load();
  _release = std::pow(10.0f, -2.0f - (3.0f * fParam4));
}

void MDALimiterAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear any output channels that don't contain input data.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  // Only recalculate when a parameter has changed.
  bool expected = true;
  if (_parametersChanged.compare_exchange_strong(expected, false)) {
    update();
  }

  const float *in1 = buffer.getReadPointer(0);
  const float *in2 = buffer.getReadPointer(1);
  float *out1 = buffer.getWritePointer(0);
  float *out2 = buffer.getWritePointer(1);

  const float threshold = _threshold;
  const float attack = _attack;
  const float release = _release;
  const float trim = _trim;

  float g = _gain;

  /*
    How this works:

    The limiter algorithm computes a gain signal by comparing the audio input
    to the threshold level. And then it multiplies the audio by this gain to
    prevent the sound from becoming too loud.

    The gain level smoothly goes down whenever the threshold is exceeded. Once
    the audio is no longer exceeding the threshold, the gain signal can go back
    up again, so that softer passages in the audio will play at their original
    loudness as much as possible.

    You can think of this as a tug-of-war: on any given sample, we're either
    reducing the gain because the sound is too loud, or we're increasing the
    gain to bring the volume back to its original level.

    In practice, the gain signal is constantly going up and down. The attack
    and release settings determine how smooth this motion is.

    If the attack is too slow, the gain reduction can't keep up with the sound
    and it will keep exceeding the threshold. If the attack is too fast, the
    sound level will change too abruptly (although for a limiter it's usually
    OK to have a really fast attack).

    The release should generally be slower than the attack. But if the release
    is too slow, the audio may end up sounding too soft because it will never
    recover back to its full level. On the other hand, if the release is too
    fast, the gain signal may end up rapidly bouncing around the threshold.

    This plug-in provides too modes: soft knee and hard knee. Hard knee waits
    until the threshold has been crossed and will then limit the audio signal.
    Soft knee does not wait until the threshold has been reached, but gradually
    starts to attentuate the audio as it approaches the threshold. Generally,
    soft knee gives a more gentle effect, while hard knee tends to be harsher.
  */

  if (_softKnee) {
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      float inL = in1[i];
      float inR = in2[i];

      // In soft knee mode we don't wait until the audio exceeds the threshold.
      // Instead, calculate what the current audio amplitude should be to stay
      // below the threshold. If the audio is silent, the level variable is 1.
      // The louder the audio becomes, the smaller level will be and the more
      // the gain will be reduced. Also, the larger the threshold, the quicker
      // this variable will drop towards 0 (although it will never reach 0).
      float level = 1.0f / (1.0f + threshold * std::abs(inL + inR));

      // If we wanted to immediately squash the audio signal to fit within the
      // desired levels, we could use the value of the level variable as the
      // gain for the current sample. In fact, that is what happens when attack
      // and release are both 1.0. However, this also distorts the signal.
      // By using smaller attack and release values, the gain signal becomes a
      // smoothed version of the instantaneous audio level.
      if (g > level) {
        g = g - attack * (g - level);
      } else {
        g = g + release * (level - g);
      }

      out1[i] = inL * trim * g;
      out2[i] = inR * trim * g;
    }

  // Hard knee mode
  } else {
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      float inL = in1[i];
      float inR = in2[i];

      // To find out whether the sound exceeds the threshold, we need to find
      // out how loud it is. First, convert the signal from stereo to mono by
      // adding up the two channels. To compensate for the increased amplitude
      // from this, multiply by 0.5 (i.e. we average the two channels). Then
      // take the absolute value so that negative sample values don't cancel
      // out positive sample values. Also multiply by the current gain level,
      // because we'll be using this variable to determine if that gain level
      // is still appropriate.
      // This is a very simple level detector: it only looks at the current
      // sample. If this sample is way below the threshold but the overall
      // sound is still too loud, we'll actually start incrementing the gain
      // again. One way to improve that is to use an envelope detector, which
      // looks at the general trend of the signal instead of single samples.
      float level = 0.5f * g * std::abs(inL + inR);

      if (level > threshold) {
        // If the signal level goes over the threshold, the current gain value
        // is too high and we must reduce it. The attack determines how quickly
        // the gain responds -- we don't immediately drop the gain all the way
        // but only reduce it a little bit. We keep doing this for the next
        // samples too, until the sound level no longer exceeds the threshold.
        g = g - (attack * (level - threshold));
      } else {
        // If the signal is below the threshold, the gain setting might now be
        // too low. Slowly bring the gain level back to 1.0. This follows a
        // logarithmic curve. The smaller the release, the longer this takes.
        // This continues until the gain is 1 or until the sound level crosses
        // the threshold again.
        g = g + (release * (1.0f - g));
      }

      out1[i] = inL * trim * g;
      out2[i] = inR * trim * g;
    }
  }

  _gain = g;
}

juce::AudioProcessorEditor *MDALimiterAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDALimiterAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDALimiterAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    _parametersChanged.store(true);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MDALimiterAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Thresh",
    "Thresh",
    juce::NormalisableRange<float>(-40.0f, 0.0f, 0.01f),
    -16.0f,
    "dB"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Output",
    "Output",
    juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
    4.0f,
    "dB"));

  // In the original plug-in, the attack and release times are kind of weird.
  // For example, the time in milliseconds was computed as follows:
  //   -301030.1 / (getSampleRate() * log10(1.0 - att)))
  // where att is a value between 1 (fastest attack) and 0.01 (slowest).
  // I don't understand where that formula comes from. It might be some kind of
  // approximation based on the actual behavior of the limiting algorithm?
  // In the JUCE version, we just keep the attack and release sliders between
  // 0 and 1 where 0 means fastest and 1 means slowest.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Attack",
    "Attack",
    juce::NormalisableRange<float>(0.0f, 1.0f),
    0.15f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Release",
    "Release",
    juce::NormalisableRange<float>(0.0f, 1.0f),
    0.5f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
    "Knee",
    "Knee",
    juce::StringArray({ "Hard", "Soft" }),
    0));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDALimiterAudioProcessor();
}
