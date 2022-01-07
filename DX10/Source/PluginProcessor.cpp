#include "PluginProcessor.h"


void protectYourEars(float *buffer, int frameCount) {
  #ifdef DEBUG
  bool firstWarning = true;
  #endif
  for (int i = 0; i < frameCount; ++i) {
    float x = buffer[i];
    bool silence = false;
    if (std::isnan(x)) {
      #ifdef DEBUG
      printf("!!! WARNING: nan detected in audio buffer, silencing !!!\n");
      #endif
      silence = true;
    } else if (std::isinf(x)) {
      #ifdef DEBUG
      printf("!!! WARNING: inf detected in audio buffer, silencing !!!\n");
      #endif
      silence = true;
    } else if (x < -2.0f || x > 2.0f) {  // screaming feedback
      #ifdef DEBUG
      printf("!!! WARNING: sample out of range (%f), silencing !!!\n", x);
      #endif
      silence = true;
    } else if (x < -1.0f) {
      #ifdef DEBUG
      if (firstWarning) {
        printf("!!! WARNING: sample out of range (%f), clamping !!!\n", x);
        firstWarning = false;
      }
      #endif
      buffer[i] = -1.0f;
    } else if (x > 1.0f) {
      #ifdef DEBUG
      if (firstWarning) {
        printf("!!! WARNING: sample out of range (%f), clamping !!!\n", x);
        firstWarning = false;
      }
      #endif
      buffer[i] = 1.0f;
    }
    if (silence) {
      memset(buffer, 0, frameCount * sizeof(float));
      return;
    }
  }
}




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
    _voices[v].cdec = 0.99f;  // all notes off
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
  lfo0 = 0.0f;
  lfo1 = 1.0f;
  MW = 0.0f;
}

void DX10AudioProcessor::update()
{
  float param11 = apvts.getRawParameterValue("Octave")->load();
  tune = 8.175798915644f * _inverseSampleRate * std::pow(2.0f, std::floor(param11 * 6.9f) - 2.0f);

  rati = apvts.getRawParameterValue("Coarse")->load();
  rati = std::floor(40.1f * rati * rati);

  float ratf = apvts.getRawParameterValue("Fine")->load();
  if (ratf < 0.5f) {
    ratf = 0.2f * ratf * ratf;
  } else {
    switch (int(8.9f * ratf)) {
      case  4: ratf = 0.25f;       break;
      case  5: ratf = 0.33333333f; break;
      case  6: ratf = 0.50f;       break;
      case  7: ratf = 0.66666667f; break;
      default: ratf = 0.75f;
    }
  }

  ratio = 1.570796326795f * (rati + ratf);

  float param5 = apvts.getRawParameterValue("Mod Init")->load();
  depth = 0.0002f * param5 * param5;

  float param7 = apvts.getRawParameterValue("Mod Sus")->load();
  dept2 = 0.0002f * param7 * param7;

  velsens = apvts.getRawParameterValue("Mod Vel")->load();

  float param10 = apvts.getRawParameterValue("Vibrato")->load();
  vibrato = 0.001f * param10 * param10;

  float param0 = apvts.getRawParameterValue("Attack")->load();
  catt = 1.0f - std::exp(-_inverseSampleRate * std::exp(8.0f - 8.0f * param0));

  float param1 = apvts.getRawParameterValue("Decay")->load();
  if (param1 > 0.98f) {
    cdec = 1.0f;
  } else {
    cdec = std::exp(-_inverseSampleRate * std::exp(5.0f - 8.0f * param1));
  }

  float param2 = apvts.getRawParameterValue("Release")->load();
  crel = std::exp(-_inverseSampleRate * std::exp(5.0f - 5.0f * param2));

  float param6 = apvts.getRawParameterValue("Mod Dec")->load();
  mdec = 1.0f - std::exp(-_inverseSampleRate * std::exp(6.0f - 7.0f * param6));

  float param8 = apvts.getRawParameterValue("Mod Rel")->load();
  mrel = 1.0f - std::exp(-_inverseSampleRate * std::exp(5.0f - 8.0f * param8));

  float param13 = apvts.getRawParameterValue("Waveform")->load();
  rich = 0.50f - 3.0f * param13 * param13;

  float param14 = apvts.getRawParameterValue("Mod Thru")->load();
  modmix = 0.25f * param14 * param14;

  float param15 = apvts.getRawParameterValue("LFO Rate")->load();
  dlfo = 628.3f * _inverseSampleRate * 25.0f * param15 * param15;
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
//TODO: docs
            // This maps the position of the mod wheel to a parabolic curve
            // starting at 0.0 (position 0) up to 0.0806 (position 127).
            // This amount is added to the LFO intensity for vibrato / PWM.
            _modWheel = 0.00000005f * float(data2 * data2);
            break;

          case 0x07:  // volume
//TODO: docs
            // Map the position of the volume control to a parabolic curve
            // starting at 0.0 (position 0) up to 0.000806 (position 127).
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
//TODO: docs
        // This maps the pitch bend value from [-8192, 8191] to an exponential
        // curve from 0.89 to 1.12 and its reciprocal from 1.12 down to 0.89.
        // When the pitch wheel is centered, both values are 1.0. This value
        // is used to multiply the oscillator period, a shift up or down of 2
        // semitones (note: 2^(-2/12) = 0.89 and 2^(2/12) = 1.12).
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

  const float w = rich;
  const float m = modmix;

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
          lfo0 += dlfo * lfo1; //sine LFO
          lfo1 -= dlfo * lfo0;
          MW = lfo1 * (_modWheel + vibrato);
          _lfoStep = 100;
        }

        // Loop through all the voices...
        for (int v = 0; v < NVOICES; ++v) {
          // ...but only render the voices that have an active envelope.
          float e = V->env;
          if (e > SILENCE) {

            V->env = e * V->cdec; //decay & release
            V->cenv += V->catt * (e - V->cenv); //attack

            float x = V->dmod * V->mod0 - V->mod1; //could add more modulator blocks like
            V->mod1 = V->mod0;               //this for a wider range of FM sounds
            V->mod0 = x;
            V->menv += V->mdec * (V->mlev - V->menv);

            x = V->car + V->dcar + x * V->menv + MW; //carrier phase
            while(x >  1.0f) x -= 2.0f;  //wrap phase
            while(x < -1.0f) x += 2.0f;
            V->car = x;
            o += V->cenv * (m * V->mod1 + (x + x * x * x * (w * x * x - 1.0f - w)));
                //amp env //mod thru-mix //5th-order sine approximation
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
      if (_voices[v].menv < SILENCE) {
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

  protectYourEars(out1, buffer.getNumSamples());
  protectYourEars(out2, buffer.getNumSamples());
}

void DX10AudioProcessor::noteOn(int note, int velocity)
{
  if (velocity > 0) {
    // Find quietest voice
    float l = 1.0f;
    int vl = 0;
    for (int v = 0; v < NVOICES; v++) {
      if (_voices[v].env < l) {
        l = _voices[v].env;
        vl = v;
      }
    }

    float param12 = apvts.getRawParameterValue("FineTune")->load();
    l = std::exp(0.05776226505f * (float(note) + param12 + param12 - 1.0f));
    _voices[vl].note = note;                         //fine tuning
    _voices[vl].car  = 0.0f;
    _voices[vl].dcar = tune * _pitchBend * l; //pitch bend not updated during note as a bit tricky...

    if (l > 50.0f) l = 50.0f;  // key tracking
    l *= (64.0f + velsens * (velocity - 64)); //vel sens
    _voices[vl].menv = depth * l;
    _voices[vl].mlev = dept2 * l;
    _voices[vl].mdec = mdec;

    _voices[vl].dmod = ratio * _voices[vl].dcar; //sine oscillator
    _voices[vl].mod0 = 0.0f;
    _voices[vl].mod1 = std::sin(_voices[vl].dmod);
    _voices[vl].dmod = 2.0f * std::cos(_voices[vl].dmod);

    //scale volume with richness
    float param13 = apvts.getRawParameterValue("Waveform")->load();
    _voices[vl].env  = (1.5f - param13) * _volume * (velocity + 10);
    _voices[vl].catt = catt;
    _voices[vl].cenv = 0.0f;
    _voices[vl].cdec = cdec;
  }

  else {  // note off
    // We also get here when the sustain pedal is released. In that case,
    // the note number is SUSTAIN and any voices in SUSTAIN are released.

    for (int v = 0; v < NVOICES; v++) {
      // Any voices playing this note?
      if (_voices[v].note == note) {
        // If the sustain pedal is not pressed, then start envelope release.
        if (_sustain == 0) {
          _voices[v].cdec = crel; //release phase
          _voices[v].env  = _voices[v].cenv;
          _voices[v].catt = 1.0f;
          _voices[v].mlev = 0.0f;
          _voices[v].mdec = mrel;
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
      float ratf = 0.0f;
      if (value < 0.5f) {
        ratf = 0.2f * value * value;
      } else {
        switch (int(8.9f * value)) {
          case  4: ratf = 0.25f;       break;
          case  5: ratf = 0.33333333f; break;
          case  6: ratf = 0.50f;       break;
          case  7: ratf = 0.66666667f; break;
          default: ratf = 0.75f;
        }
      }
      return juce::String(ratf, 3);
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
