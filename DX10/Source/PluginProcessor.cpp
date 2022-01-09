#include "PluginProcessor.h"

DX10Program::DX10Program(const char *name,
                         float p0,  float p1,  float p2,  float p3,
                         float p4,  float p5,  float p6,  float p7,
                         float p8,  float p9,  float p10, float p11,
                         float p12, float p13, float p14, float p15)
{
  strcpy(this->name, name);
  param[0]  = p0;  param[1]  = p1;  param[2]  = p2;  param[3]  = p3;
  param[4]  = p4;  param[5]  = p5;  param[6]  = p6;  param[7]  = p7;
  param[8]  = p8;  param[9]  = p9;  param[10] = p10; param[11] = p11;
  param[12] = p12; param[13] = p13; param[14] = p14; param[15] = p15;
}

DX10AudioProcessor::DX10AudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  _sampleRate = 44100.0f;
  _inverseSampleRate = 1.0f / _sampleRate;

  createPrograms();
  setCurrentProgram(0);

  apvts.state.addListener(this);
}

DX10AudioProcessor::~DX10AudioProcessor()
{
  apvts.state.removeListener(this);
}

const juce::String DX10AudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int DX10AudioProcessor::getNumPrograms()
{
  return int(_programs.size());
}

int DX10AudioProcessor::getCurrentProgram()
{
  return _currentProgram;
}

void DX10AudioProcessor::setCurrentProgram(int index)
{
  _currentProgram = index;

  const char *paramNames[] = {
    "Attack",
    "Decay",
    "Release",
    "Coarse",
    "Fine",
    "Mod Init",
    "Mod Dec",
    "Mod Sus",
    "Mod Rel",
    "Mod Vel",
    "Vibrato",
    "Octave",
    "FineTune",
    "Waveform",
    "Mod Thru",
    "LFO Rate",
  };

  for (int i = 0; i < NPARAMS; ++i) {
    apvts.getParameter(paramNames[i])->setValueNotifyingHost(_programs[index].param[i]);
  }
}

const juce::String DX10AudioProcessor::getProgramName(int index)
{
  return { _programs[index].name };
}

void DX10AudioProcessor::changeProgramName(int index, const juce::String &newName)
{
  // not implemented
}

void DX10AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  _sampleRate = sampleRate;
  _inverseSampleRate = 1.0f / _sampleRate;

  resetState();
  _parametersChanged.store(true);
}

void DX10AudioProcessor::releaseResources()
{
}

void DX10AudioProcessor::reset()
{
  resetState();
}

bool DX10AudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DX10AudioProcessor::createPrograms()
{
                                        // Att     Dec     Rel   | Rat C   Rat F   Att     Dec     Sus     Rel     Vel   | Vib     Oct     Fine    Rich    Thru    LFO
  _programs.emplace_back("Bright E.Piano", 0.000f, 0.650f, 0.441f, 0.842f, 0.329f, 0.230f, 0.800f, 0.050f, 0.800f, 0.900f, 0.000f, 0.500f, 0.500f, 0.447f, 0.000f, 0.414f);
  _programs.emplace_back("Jazz E.Piano",   0.000f, 0.500f, 0.100f, 0.671f, 0.000f, 0.441f, 0.336f, 0.243f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.178f, 0.000f, 0.500f);
  _programs.emplace_back("E.Piano Pad",    0.000f, 0.700f, 0.400f, 0.230f, 0.184f, 0.270f, 0.474f, 0.224f, 0.800f, 0.974f, 0.250f, 0.500f, 0.500f, 0.428f, 0.836f, 0.500f);
  _programs.emplace_back("Fuzzy E.Piano",  0.000f, 0.700f, 0.400f, 0.320f, 0.217f, 0.599f, 0.670f, 0.309f, 0.800f, 0.500f, 0.263f, 0.507f, 0.500f, 0.276f, 0.638f, 0.526f);
  _programs.emplace_back("Soft Chimes",    0.400f, 0.600f, 0.650f, 0.760f, 0.000f, 0.390f, 0.250f, 0.160f, 0.900f, 0.500f, 0.362f, 0.500f, 0.500f, 0.401f, 0.296f, 0.493f);
  _programs.emplace_back("Harpsichord",    0.000f, 0.342f, 0.000f, 0.280f, 0.000f, 0.880f, 0.100f, 0.408f, 0.740f, 0.000f, 0.000f, 0.600f, 0.500f, 0.842f, 0.651f, 0.500f);
  _programs.emplace_back("Funk Clav",      0.000f, 0.400f, 0.100f, 0.360f, 0.000f, 0.875f, 0.160f, 0.592f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.303f, 0.868f, 0.500f);
  _programs.emplace_back("Sitar",          0.000f, 0.500f, 0.704f, 0.230f, 0.000f, 0.151f, 0.750f, 0.493f, 0.770f, 0.500f, 0.000f, 0.400f, 0.500f, 0.421f, 0.632f, 0.500f);
  _programs.emplace_back("Chiff Organ",    0.600f, 0.990f, 0.400f, 0.320f, 0.283f, 0.570f, 0.300f, 0.050f, 0.240f, 0.500f, 0.138f, 0.500f, 0.500f, 0.283f, 0.822f, 0.500f);
  _programs.emplace_back("Tinkle",         0.000f, 0.500f, 0.650f, 0.368f, 0.651f, 0.395f, 0.550f, 0.257f, 0.900f, 0.500f, 0.300f, 0.800f, 0.500f, 0.000f, 0.414f, 0.500f);
  _programs.emplace_back("Space Pad",      0.000f, 0.700f, 0.520f, 0.230f, 0.197f, 0.520f, 0.720f, 0.280f, 0.730f, 0.500f, 0.250f, 0.500f, 0.500f, 0.336f, 0.428f, 0.500f);
  _programs.emplace_back("Koto",           0.000f, 0.240f, 0.000f, 0.390f, 0.000f, 0.880f, 0.100f, 0.600f, 0.740f, 0.500f, 0.000f, 0.500f, 0.500f, 0.526f, 0.480f, 0.500f);
  _programs.emplace_back("Harp",           0.000f, 0.500f, 0.700f, 0.160f, 0.000f, 0.158f, 0.349f, 0.000f, 0.280f, 0.900f, 0.000f, 0.618f, 0.500f, 0.401f, 0.000f, 0.500f);
  _programs.emplace_back("Jazz Guitar",    0.000f, 0.500f, 0.100f, 0.390f, 0.000f, 0.490f, 0.250f, 0.250f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.263f, 0.145f, 0.500f);
  _programs.emplace_back("Steel Drum",     0.000f, 0.300f, 0.507f, 0.480f, 0.730f, 0.000f, 0.100f, 0.303f, 0.730f, 1.000f, 0.000f, 0.600f, 0.500f, 0.579f, 0.000f, 0.500f);
  _programs.emplace_back("Log Drum",       0.000f, 0.300f, 0.500f, 0.320f, 0.000f, 0.467f, 0.079f, 0.158f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.151f, 0.020f, 0.500f);
  _programs.emplace_back("Trumpet",        0.000f, 0.990f, 0.100f, 0.230f, 0.000f, 0.000f, 0.200f, 0.450f, 0.800f, 0.000f, 0.112f, 0.600f, 0.500f, 0.711f, 0.000f, 0.401f);
  _programs.emplace_back("Horn",           0.280f, 0.990f, 0.280f, 0.230f, 0.000f, 0.180f, 0.400f, 0.300f, 0.800f, 0.500f, 0.000f, 0.400f, 0.500f, 0.217f, 0.480f, 0.500f);
  _programs.emplace_back("Reed 1",         0.220f, 0.990f, 0.250f, 0.170f, 0.000f, 0.240f, 0.310f, 0.257f, 0.900f, 0.757f, 0.000f, 0.500f, 0.500f, 0.697f, 0.803f, 0.500f);
  _programs.emplace_back("Reed 2",         0.220f, 0.990f, 0.250f, 0.450f, 0.070f, 0.240f, 0.310f, 0.360f, 0.900f, 0.500f, 0.211f, 0.500f, 0.500f, 0.184f, 0.000f, 0.414f);
  _programs.emplace_back("Violin",         0.697f, 0.990f, 0.421f, 0.230f, 0.138f, 0.750f, 0.390f, 0.513f, 0.800f, 0.316f, 0.467f, 0.678f, 0.500f, 0.743f, 0.757f, 0.487f);
  _programs.emplace_back("Chunky Bass",    0.000f, 0.400f, 0.000f, 0.280f, 0.125f, 0.474f, 0.250f, 0.100f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.579f, 0.592f, 0.500f);
  _programs.emplace_back("E.Bass",         0.230f, 0.500f, 0.100f, 0.395f, 0.000f, 0.388f, 0.092f, 0.250f, 0.150f, 0.500f, 0.200f, 0.200f, 0.500f, 0.178f, 0.822f, 0.500f);
  _programs.emplace_back("Clunk Bass",     0.000f, 0.600f, 0.400f, 0.230f, 0.000f, 0.450f, 0.320f, 0.050f, 0.900f, 0.500f, 0.000f, 0.200f, 0.500f, 0.520f, 0.105f, 0.500f);
  _programs.emplace_back("Thick Bass",     0.000f, 0.600f, 0.400f, 0.170f, 0.145f, 0.290f, 0.350f, 0.100f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.441f, 0.309f, 0.500f);
  _programs.emplace_back("Sine Bass",      0.000f, 0.600f, 0.490f, 0.170f, 0.151f, 0.099f, 0.400f, 0.000f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.118f, 0.013f, 0.500f);
  _programs.emplace_back("Square Bass",    0.000f, 0.600f, 0.100f, 0.320f, 0.000f, 0.350f, 0.670f, 0.100f, 0.150f, 0.500f, 0.000f, 0.200f, 0.500f, 0.303f, 0.730f, 0.500f);
  _programs.emplace_back("Upright Bass 1", 0.300f, 0.500f, 0.400f, 0.280f, 0.000f, 0.180f, 0.540f, 0.000f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.296f, 0.033f, 0.500f);
  _programs.emplace_back("Upright Bass 2", 0.300f, 0.500f, 0.400f, 0.360f, 0.000f, 0.461f, 0.070f, 0.070f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.546f, 0.467f, 0.500f);
  _programs.emplace_back("Harmonics",      0.000f, 0.500f, 0.500f, 0.280f, 0.000f, 0.330f, 0.200f, 0.000f, 0.700f, 0.500f, 0.000f, 0.500f, 0.500f, 0.151f, 0.079f, 0.500f);
  _programs.emplace_back("Scratch",        0.000f, 0.500f, 0.000f, 0.000f, 0.240f, 0.580f, 0.630f, 0.000f, 0.000f, 0.500f, 0.000f, 0.600f, 0.500f, 0.816f, 0.243f, 0.500f);
  _programs.emplace_back("Syn Tom",        0.000f, 0.355f, 0.350f, 0.000f, 0.105f, 0.000f, 0.000f, 0.200f, 0.500f, 0.500f, 0.000f, 0.645f, 0.500f, 1.000f, 0.296f, 0.500f);
}

void DX10AudioProcessor::resetState()
{
  // Turn off all playing voices.
  for (int v = 0; v < NVOICES; ++v) {
    _voices[v].env = 0.0f;
    _voices[v].car = 0.0f;
    _voices[v].dcar = 0.0f;
    _voices[v].mod0 = 0.0f;
    _voices[v].mod1 = 0.0f;
    _voices[v].dmod = 0.0f;
    _voices[v].cdec = 0.99f;
  }
  _numActiveVoices = 0;

  // Clear out any pending MIDI events.
  _notes[0] = EVENTS_DONE;

  // These variables are changed by MIDI CC, reset to defaults.
  _modWheel = 0.0f;
  _pitchBend = 1.0f;
  _volume = 0.0035f;
  _sustain = 0;

  // Reset other state.
  _lfoStep = 0;
  _lfo0 = 0.0f;
  _lfo1 = 1.0f;
  _modulationAmount = 0.0f;
}

void DX10AudioProcessor::update()
{
  /*
    Calculate a multiplier for the pitches of the notes based on the amount of
    tuning. The number of octaves is -3 to +3. To calculate a multiplier for N
    semitones, we do `2^(N/12)`. Since an octave has 12 semitones, we can just
    do `2^(oct)`. Then we multiply this by 8.175 Hz, which is the pitch of the
    lowest possible note (with MIDI note number 0).

    It's totally not obvious, but there is also an additional factor 2 in here.
    This is needed for the oscillator algorithm. Instead of [-3, +3] we treat
    the number of octaves actually as [-2, +4], so `2^(oct)` is always 2x higher.
    More about why this is done in noteOn().

    We also divide by the sample rate already to save doing this division later
    when calculating the phase increment for the carrier.
  */
  float param11 = apvts.getRawParameterValue("Octave")->load();
  _tune = 8.175798915644f * _inverseSampleRate * std::pow(2.0f, std::floor(param11 * 6.9f) - 2.0f);

  // Fine-tuning: -100 cents to +100 cents, a value from -1.0 to +1.0.
  float param12 = apvts.getRawParameterValue("FineTune")->load();
  _fineTune = param12 + param12 - 1.0f;

  // The carrier:modulator ratio is a whole number between 0 and 40.
  float coarse = apvts.getRawParameterValue("Coarse")->load();
  coarse = std::floor(40.1f * coarse * coarse);

  // Fine-tuning the c:m ratio is a much smaller number, between 0 and 0.050,
  // or a fixed ratio picked from a list.
  float fine = apvts.getRawParameterValue("Fine")->load();
  if (fine < 0.5f) {
    fine = 0.2f * fine * fine;
  } else {
    switch (int(8.9f * fine)) {
      case  4: fine = 0.25f;       break;
      case  5: fine = 0.33333333f; break;
      case  6: fine = 0.50f;       break;
      case  7: fine = 0.66666667f; break;
      default: fine = 0.75f;
    }
  }

  // The final carrier:modulator ratio combines the coarse and fine settings.
  // To save some multiplications later the factor PI is already included here.
  // Note that this uses PI/2, which causes the modulator to be actually at
  // half the pitch given by the ratio. For example, with Coarse set to 1 and
  // Fine to 0, the modulator pitch will be half that of the carrier, not equal
  // to the carrier pitch! (Not sure if this was intentional or a mistake.)
  _ratio = 1.570796326795f * (coarse + fine);

  // Velocity sensitivity: a value between 0 and 1.
  _velocitySensitivity = apvts.getRawParameterValue("Mod Vel")->load();

  // Vibrato amount: this is a skewed parameter that goes from 0.0 to 0.001.
  float param10 = apvts.getRawParameterValue("Vibrato")->load();
  _vibrato = 0.001f * param10 * param10;

  // The carrier's envelope is simply a value that gets multiplied by a value
  // less than one, which means it exponentially decays over time. The attack
  // is made by exponentially going up rather than down.

  float param0 = apvts.getRawParameterValue("Attack")->load();
  _attack = 1.0f - std::exp(-_inverseSampleRate * std::exp(8.0f - 8.0f * param0));

  float param1 = apvts.getRawParameterValue("Decay")->load();
  if (param1 > 0.98f) {
    _decay = 1.0f;
  } else {
    _decay = std::exp(-_inverseSampleRate * std::exp(5.0f - 8.0f * param1));
  }

  float param2 = apvts.getRawParameterValue("Release")->load();
  _release = std::exp(-_inverseSampleRate * std::exp(5.0f - 5.0f * param2));

  // The modulator envelope does not have an attack but starts out at a given
  // initial level, then takes decay time to reach the sustain level. As with
  // the carrier envelope, this is done using a basic smoothing filter.

  float param5 = apvts.getRawParameterValue("Mod Init")->load();
  _modInitialLevel = 0.0002f * param5 * param5;

  float param6 = apvts.getRawParameterValue("Mod Dec")->load();
  _modDecay = 1.0f - std::exp(-_inverseSampleRate * std::exp(6.0f - 7.0f * param6));

  float param7 = apvts.getRawParameterValue("Mod Sus")->load();
  _modSustain = 0.0002f * param7 * param7;

  float param8 = apvts.getRawParameterValue("Mod Rel")->load();
  _modRelease = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.0f - 8.0f * param8));

  // The value of this parameter changes the shape of the carrier wave.
  // At 0%, it's a plain sine wave. At 100%, it's more like a sawtooth.
  float param13 = apvts.getRawParameterValue("Waveform")->load();
  _richness = 0.50f - 3.0f * param13 * param13;

  // Modulator mix is a skewed value between 0 and 0.25.
  float param14 = apvts.getRawParameterValue("Mod Thru")->load();
  _modMix = 0.25f * param14 * param14;

  // The LFO rate is between 0 and 25 Hz. This parameter is skewed. The factor
  // 628.3 in the formula below is 100 * 2 pi, because the LFO is only updated
  // every 100 samples.
  float param15 = apvts.getRawParameterValue("LFO Rate")->load();
  _lfoInc = 628.3f * _inverseSampleRate * 25.0f * param15 * param15;
}

void DX10AudioProcessor::processEvents(juce::MidiBuffer &midiMessages)
{
  // There are different ways a synth can handle MIDI events. This plug-in does
  // it by copying the events it cares about -- note on and note off -- into an
  // array. In the render loop, we step through this array to process the events
  // one-by-one. Controllers such as the sustain pedal are processed right away,
  // at the start of the block, so these are not sample accurate.

  int npos = 0;
  for (const auto metadata : midiMessages) {
    if (metadata.numBytes != 3) continue;

    const auto data0 = metadata.data[0];
    const auto data1 = metadata.data[1];
    const auto data2 = metadata.data[2];
    const int deltaFrames = metadata.samplePosition;

    switch (data0 & 0xf0) {  // status byte (all channels)
      // Note off
      case 0x80:
        _notes[npos++] = deltaFrames;   // time offset
        _notes[npos++] = data1 & 0x7F;  // note
        _notes[npos++] = 0;             // vel
        break;

      // Note on
      case 0x90:
        _notes[npos++] = deltaFrames;   // time offset
        _notes[npos++] = data1 & 0x7F;  // note
        _notes[npos++] = data2 & 0x7F;  // vel
        break;

      // Controller
      case 0xB0:
        switch (data1) {
          case 0x01:  // mod wheel
            // This maps the position of the mod wheel to a parabolic curve
            // starting at 0.0 (position 0) up to 0.000806 (position 127).
            // This amount is added to the LFO intensity for vibrato.
            _modWheel = 0.00000005f * float(data2 * data2);
            break;

          case 0x07:  // volume
            // Map the position of the volume control to a parabolic curve
            // starting at 0.0 (position 0) up to 0.00564 (position 127).
            _volume = 0.00000035f * float(data2 * data2);
            break;

          case 0x40:  // sustain pedal
            // Make the variable 64 when the pedal is pressed and 0 when released.
            _sustain = data2 & 0x40;

            // Pedal released? Then end all sustained notes. This sends a fake
            // note-off event with note = SUSTAIN, meaning all sustained notes
            // will be moved into their envelope release stage.
            if (_sustain == 0) {
              _notes[npos++] = deltaFrames;
              _notes[npos++] = SUSTAIN;
              _notes[npos++] = 0;
            }
            break;

          default:  // all notes off
            if (data1 > 0x7A) {
              // Setting the decay to 0.99 will fade out the voice very quickly.
              for (int v = 0; v < NVOICES; ++v) {
                _voices[v].cdec = 0.99f;
              }
              _sustain = 0;
            }
            break;
        }
        break;

      // Program change
      case 0xC0:
        if (data1 < _programs.size()) {
          setCurrentProgram(data1);
        }
        break;

      // Pitch bend
      case 0xE0:
        // This maps the pitch bend value from [-8192, 8191] to [0.89, 1.12]
        // where 1.0 means the pitch wheel is centered. This value is used to
        // shift the carrier up or down 2 semitones (since 2^(-2/12) = 0.89
        // and 2^(2/12) = 1.12).
        _pitchBend = float(data1 + 128 * data2 - 8192);
        if (_pitchBend > 0.0f) {
          _pitchBend = 1.0f + 0.000014951f * _pitchBend;
        } else {
          _pitchBend = 1.0f + 0.000013318f * _pitchBend;
        }
        break;

      default: break;
    }

    // Discard events if buffer full!
    if (npos > EVENTBUFFER) npos -= 3;
	}
  _notes[npos] = EVENTS_DONE;
  midiMessages.clear();
}

void DX10AudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  processEvents(midiMessages);

  int sampleFrames = buffer.getNumSamples();

  float *out1 = buffer.getWritePointer(0);
  float *out2 = buffer.getWritePointer(1);

  int event = 0;
  int frame = 0;  // how many samples are already rendered

  // Is there at least one active voice, or any pending MIDI event?
  if (_numActiveVoices > 0 || _notes[event] < sampleFrames) {
    while (frame < sampleFrames) {
      // Get the timestamp of the next note on/off event. This is usually in the
      // future, i.e. a number of samples after the current sample.
      int frames = _notes[event++];

      // This catches the EVENTS_DONE special event. There are no events left.
      if (frames > sampleFrames) frames = sampleFrames;

      // The timestamp for the event is relative to the start of the block.
      // Make it relative to the previous event; this tells us how many samples
      // to process until the new event actually happens.
      frames -= frame;

      // When we're done with this event, this is how many samples we will have
      // processed in total.
      frame += frames;

      // Until it's time to process the upcoming event, render the active voices.
      while (--frames >= 0) {
        Voice *V = _voices;

        // This variable adds up the output values of all the active voices.
        // DX10 is a mono synth, so there is only one channel.
        float o = 0.0f;

        // The LFO and any things it modulates are updated every 100 samples.
        if (--_lfoStep < 0) {
          // This formula is a simple method to approximate a sine wave, but
          // it only works for low frequencies such as with an LFO.
          _lfo0 += _lfoInc * _lfo1;
          _lfo1 -= _lfoInc * _lfo0;

          // Calculate the new amount of modulation. This value swings between
          // -0.001806 and +0.001806 and is directly added to the carrier phase.
          _modulationAmount = _lfo1 * (_modWheel + _vibrato);

          _lfoStep = 100;  // reset counter
        }

        // Loop through all the voices...
        for (int v = 0; v < NVOICES; ++v) {
          // ...but only render the voices that have an active envelope.
          float e = V->env;
          if (e > SILENCE) {

            // The envelope is always decaying.
            V->env = e * V->cdec;

            // To add an attack to the envelope, apply a smoothing filter that
            // raises `cenv` from 0.0 to the current envelope level `env`.
            // The longer the attack, the smaller `catt` and the slower this
            // filter increments the level. All the while, `env` is decaying
            // and pulling things downward, so with a long attack the smoothed
            // envelope `cenv` never gets as high as with a shorter attack.
            // When the note is released, `catt` is set to 1 so that the attack
            // ends and `cenv` only decays from that point onwards.
            V->cenv += V->catt * (e - V->cenv);

            // Simple sine wave oscillator. This creates a sine wave that first
            // goes down and then goes up, i.e. it has been phase inverted.
            // The LFO also uses a sine wave oscillator but the one used for
            // the modulator wave is more reliable at higher frequencies.
            float y = V->dmod * V->mod0 - V->mod1;
            V->mod1 = V->mod0;
            V->mod0 = y;

            // The envelope for the modulator is a simple smoothing filter that
            // gradually moves from `menv` to the target level `mlev` in an
            // exponential fashion. Which is what we want because frequencies
            // are logarithmic, so to move through them at a constant speed we
            // must move in exponential steps.
            V->menv += V->mdec * (V->mlev - V->menv);

            // Could add more modulator blocks for a wider range of FM sounds.
            // Most FM synths allow for different configurations but DX10 keeps
            // it simple with just one modulator.

            // Calculate the new carrier phase. This phase is also changed by
            // the modulator wave (FM!) and also the mouse wheel and vibrato.
            float x = V->car + V->dcar + y * V->menv + _modulationAmount;

            // Wrap the phase if it goes out of bounds. Note that modulation
            // may cause the carrier phase to go backwards.
            while (x >  1.0f) x -= 2.0f;
            while (x < -1.0f) x += 2.0f;
            V->car = x;

            // Create a 5th-order sine approximation. If you plot this formula,
            // you'll get a sine-like shape between x = -1 and x = +1. That's
            // why the carrier phase is restricted to the range [-1, +1].
            // The richness parameter "distorts" this shape into something that
            // looks more like a saw wave (which also boosts its amplitude).
            float s = x + x * x * x * (_richness * x * x - 1.0f - _richness);

            // Mix in the modulator waveform and apply the amplitude envelope.
            o += V->cenv * (_modMix * V->mod1 + s);
          }

          // Go to the next active voice.
          V++;
        }

        // Write the result into the output buffer.
        *out1++ = o;
        *out2++ = o;
      }

      // It's time to handle the event. This starts the new note, or stops the
      // note if velocity is 0. Also handles the sustain pedal being lifted.
      if (frame < sampleFrames) {
        int note = _notes[event++];
        int vel  = _notes[event++];
        noteOn(note, vel);
      }
    }

    // Turn off voices whose envelope has dropped below the minimum level.
    _numActiveVoices = NVOICES;
    for (int v = 0; v < NVOICES; ++v) {
      if (_voices[v].env < SILENCE) {
        _voices[v].env = 0.0f;
        _voices[v].cenv = 0.0f;
        _numActiveVoices--;
      }
      if (_voices[v].menv < SILENCE) {  // stop modulation envelope
        _voices[v].menv = 0.0f;
        _voices[v].mlev = 0.0f;
      }
    }
  } else {
    // No voices playing and no events, so render an empty block.
    while (--sampleFrames >= 0) {
      *out1++ = 0.0f;
      *out2++ = 0.0f;
    }
  }

  // Mark the events buffer as done.
  _notes[0] = EVENTS_DONE;
}

void DX10AudioProcessor::noteOn(int note, int velocity)
{
  if (velocity > 0) {
    // Find quietest voice. Voices that are not playing have env = 0.
    float l = 1.0f;
    int vl = 0;
    for (int v = 0; v < NVOICES; v++) {
      if (_voices[v].env < l) {
        l = _voices[v].env;
        vl = v;
      }
    }

    /*
      Calculate the pitch for this MIDI note.

      The formula for this is: `440 * 2^((note - 69)/12)`. However, the code
      below does `exp(0.05776 * note)`, which is equivalent to `2^(note/12)`.

      The note number here is treated as the number of semitones away from the
      base frequency. Instead of the usual 440 Hz, the base frequency is now
      of the lowest possible note (MIDI note number 0), which is 8.1757989156.
      That is why that factor 8.1757989156 is part of the `_tune` variable.

      We also add the amount of fine-tuning to the number of semitones in the
      formula. Since fine-tuning is ±100 cents, the pitch can go up or down by
      one additional semitone.
    */
    float p = std::exp(0.05776226505f * (float(note) + _fineTune));

    _voices[vl].note = note;

    // Reset the carrier oscillator phase and calculate its phase increment.
    // Also apply pitch bending here already rather than during rendering,
    // because apparently that is a bit tricky... which means you can't really
    // pitch bend the note while it's playing.
    _voices[vl].car = 0.0f;
    _voices[vl].dcar = _tune * _pitchBend * p;

    /*
      Some more details:

      The phase increment `dcar` describes how many samples it takes to count
      from 0 up to 1. If the pitch is 100 Hz and the sample rate is 44100 Hz,
      then dcar is 0.002268, so that it takes 441 samples to count from 0 to 1.

      However... `dcar` actually only describes the length of a half period!

      If you look at the computed pitch for the MIDI note number, as given by
      `_tune * p * sampleRate`, you'll notice it's an octave too high. Since
      the pitch is 2x higher, the period is half the size of what it should be.
      (See also the comment in update() where _tune is calculated.)

      Why? In processBlock() the carrier phase value does not go from 0 to 1,
      but from -1 to +1. This is a distance of 2.0, and so two half-periods
      make one full period.
    */

    // Change the modulator's envelope levels based on the note that's played.
    // We can re-use the pitch for this. If the note is higher than G4, disable
    // key tracking (not sure why that is done). Keep in mind that the pitch
    // is p * 8.1757, so p = 50 is 408.8 Hz, which is in between G4 and G#4.
    if (p > 50.0f) p = 50.0f;

    // Also modify the modulator envelope using velocity. If sensitivity = 0%,
    // then velocity is always 64. If sensitivity = 100%, then the velocity is
    // used unchanged. This means p gets multiplied by a value in between 1 and
    // 127. The max value for p is then 6350; the minimum value is 1.
    p *= (64.0f + _velocitySensitivity * (velocity - 64));

    // Set the envelope levels for the modulator.
    _voices[vl].menv = _modInitialLevel * p;
    _voices[vl].mlev = _modSustain * p;
    _voices[vl].mdec = _modDecay;

    // The phase increment for the modulator is based on that of the carrier.
    // As explained elsewhere, since `_ratio` contains the factor PI/2 instead
    // of PI, the true ratio is actually half the ratio shown in the UI, i.e.
    // if you choose Coarse = 1 and play a 440 Hz tone, the modulator is 220 Hz.
    // If ratio is 0, the modulator is disabled.
    _voices[vl].dmod = _ratio * _voices[vl].dcar;

    // The modulator is a basic sine wave. To create this sine wave, it uses
    // a simple oscillator algorithm. Here, we initialize the sine oscillator
    // for the modulator.
    _voices[vl].mod0 = 0.0f;
    _voices[vl].mod1 = std::sin(_voices[vl].dmod);
    _voices[vl].dmod = 2.0f * std::cos(_voices[vl].dmod);

    // The carrier envelope starts out at its maximum level and immediately
    // begins decaying. We set the initial value based on the note velocity
    // and volume CC. We also scale the level with the richness of the sound,
    // given by the Waveform parameter, to compensate for the amplitude boost
    // that the sawtooth shape gives.
    float param13 = apvts.getRawParameterValue("Waveform")->load();
    _voices[vl].env = (1.5f - param13) * _volume * (velocity + 10);
    _voices[vl].cdec = _decay;

    // Even though `env` will always be decaying, if an attack is set, we will
    // need to fade in the voice. `cenv` is derived from `env`, but with attack
    // applied, and this is the actual envelope level that we'll use.
    _voices[vl].catt = _attack;
    _voices[vl].cenv = 0.0f;
  }

  else {  // note off
    // We also get here when the sustain pedal is released. In that case,
    // the note number is SUSTAIN and any voices in SUSTAIN are released.

    for (int v = 0; v < NVOICES; v++) {
      // Any voices playing this note?
      if (_voices[v].note == note) {
        // If the sustain pedal is not pressed, then start envelope release.
        if (_sustain == 0) {
          _voices[v].cdec = _release;
          _voices[v].env  = _voices[v].cenv;
          _voices[v].catt = 1.0f;  // finish attack, if any
          _voices[v].mlev = 0.0f;
          _voices[v].mdec = _modRelease;
        } else {
          // Sustain pedal is pressed, so put the note in sustain mode.
          _voices[v].note = SUSTAIN;
        }
      }
    }
  }
}

juce::AudioProcessorEditor *DX10AudioProcessor::createEditor()
{
  auto editor = new juce::GenericAudioProcessorEditor(*this);
  editor->setSize(500, 1000);
  return editor;
}

void DX10AudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void DX10AudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    _parametersChanged.store(true);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout DX10AudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // The original plug-in, being written for VST2, used parameters from 0 to 1.
  // It would be nicer to change the parameters to more natural ranges, which
  // JUCE allows us to do. For example, the octave setting could be -3 to +3,
  // rather than having to map [0, 1] to [-3, +3]. However, the factory presets
  // are specified as 0 - 1 too and I didn't feel like messing with those.
  // This is why we're keeping the parameters as they were originally.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Attack",
    "Attack",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Decay",
    "Decay",
    juce::NormalisableRange<float>(),
    0.65f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Release",
    "Release",
    juce::NormalisableRange<float>(),
    0.441f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Coarse",
    "Coarse",
    juce::NormalisableRange<float>(),
    0.842f,
    "ratio",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(std::floor(40.1f * value * value)));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Fine",
    "Fine",
    juce::NormalisableRange<float>(),
    0.329f,
    "ratio",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      float fine = 0.0f;
      if (value < 0.5f) {
        fine = 0.2f * value * value;
      } else {
        switch (int(8.9f * value)) {
          case  4: fine = 0.25f;       break;
          case  5: fine = 0.33333333f; break;
          case  6: fine = 0.50f;       break;
          case  7: fine = 0.66666667f; break;
          default: fine = 0.75f;
        }
      }
      return juce::String(fine, 3);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Init",
    "Mod Init",
    juce::NormalisableRange<float>(),
    0.23f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Dec",
    "Mod Dec",
    juce::NormalisableRange<float>(),
    0.8f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Sus",
    "Mod Sus",
    juce::NormalisableRange<float>(),
    0.05f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Rel",
    "Mod Rel",
    juce::NormalisableRange<float>(),
    0.8f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Vel",
    "Mod Vel",
    juce::NormalisableRange<float>(),
    0.9f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Vibrato",
    "Vibrato",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Octave",
    "Octave",
    juce::NormalisableRange<float>(),
    0.5f,
    "",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 6.9f) - 3);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "FineTune",
    "FineTune",
    juce::NormalisableRange<float>(),
    0.5f,
    "cents",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(200.0f * value - 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Waveform",
    "Waveform",
    juce::NormalisableRange<float>(),
    0.447f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Mod Thru",
    "Mod Thru",
    juce::NormalisableRange<float>(),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(int(value * 100.0f));
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "LFO Rate",
    "LFO Rate",
    juce::NormalisableRange<float>(),
    0.414f,
    "Hz",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(25.0f * value * value, 2);
    }));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new DX10AudioProcessor();
}
