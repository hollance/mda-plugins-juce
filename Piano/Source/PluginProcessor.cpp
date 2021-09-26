#include "PluginProcessor.h"
#include "mdaPianoData.h"

#include <math.h>

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

MDAPianoProgram::MDAPianoProgram()
{
  param[0]  = 0.50f;  // Decay
  param[1]  = 0.50f;  // Release
  param[2]  = 0.50f;  // Hardness

  param[3]  = 0.50f;  // Vel>Hard
  param[4]  = 1.00f;  // Muffle
  param[5]  = 0.50f;  // Vel>Muff

  param[6]  = 0.33f;  // Vel Curve
  param[7]  = 0.50f;  // Stereo
  param[8]  = 0.33f;  // Max Poly

  param[9]  = 0.50f;  // Tune
  param[10] = 0.00f;  // Random
  param[11] = 0.50f;  // Stretch

  strcpy(name, "mda Piano");
}

MDAPianoProgram::MDAPianoProgram(const char *name,
                                 float p0, float p1, float p2, float p3,
                                 float p4, float p5, float p6, float p7,
                                 float p8, float p9, float p10, float p11)
{
  strcpy(this->name, name);
  param[0]  = p0;
  param[1]  = p1;
  param[2]  = p2;
  param[3]  = p3;
  param[4]  = p4;
  param[5]  = p5;
  param[6]  = p6;
  param[7]  = p7;
  param[8]  = p8;
  param[9]  = p9;
  param[10] = p10;
  param[11] = p11;
}

MDAPianoAudioProcessor::MDAPianoAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  Fs = 44100.0f;  iFs = 1.0f/Fs;  cmax = 0x7F;  //just in case...

  createPrograms();
  _currentProgram = 0;
//    setProgram(0);

  waves = pianoData;

  // Waveform data and keymapping is hard-wired in *this* version
  kgrp[ 0].root = 36;  kgrp[ 0].high = 37;  kgrp[ 0].pos = 0;       kgrp[ 0].end = 36275;   kgrp[ 0].loop = 14774;
  kgrp[ 1].root = 40;  kgrp[ 1].high = 41;  kgrp[ 1].pos = 36278;   kgrp[ 1].end = 83135;   kgrp[ 1].loop = 16268;
  kgrp[ 2].root = 43;  kgrp[ 2].high = 45;  kgrp[ 2].pos = 83137;   kgrp[ 2].end = 146756;  kgrp[ 2].loop = 33541;
  kgrp[ 3].root = 48;  kgrp[ 3].high = 49;  kgrp[ 3].pos = 146758;  kgrp[ 3].end = 204997;  kgrp[ 3].loop = 21156;
  kgrp[ 4].root = 52;  kgrp[ 4].high = 53;  kgrp[ 4].pos = 204999;  kgrp[ 4].end = 244908;  kgrp[ 4].loop = 17191;
  kgrp[ 5].root = 55;  kgrp[ 5].high = 57;  kgrp[ 5].pos = 244910;  kgrp[ 5].end = 290978;  kgrp[ 5].loop = 23286;
  kgrp[ 6].root = 60;  kgrp[ 6].high = 61;  kgrp[ 6].pos = 290980;  kgrp[ 6].end = 342948;  kgrp[ 6].loop = 18002;
  kgrp[ 7].root = 64;  kgrp[ 7].high = 65;  kgrp[ 7].pos = 342950;  kgrp[ 7].end = 391750;  kgrp[ 7].loop = 19746;
  kgrp[ 8].root = 67;  kgrp[ 8].high = 69;  kgrp[ 8].pos = 391752;  kgrp[ 8].end = 436915;  kgrp[ 8].loop = 22253;
  kgrp[ 9].root = 72;  kgrp[ 9].high = 73;  kgrp[ 9].pos = 436917;  kgrp[ 9].end = 468807;  kgrp[ 9].loop = 8852;
  kgrp[10].root = 76;  kgrp[10].high = 77;  kgrp[10].pos = 468809;  kgrp[10].end = 492772;  kgrp[10].loop = 9693;
  kgrp[11].root = 79;  kgrp[11].high = 81;  kgrp[11].pos = 492774;  kgrp[11].end = 532293;  kgrp[11].loop = 10596;
  kgrp[12].root = 84;  kgrp[12].high = 85;  kgrp[12].pos = 532295;  kgrp[12].end = 560192;  kgrp[12].loop = 6011;
  kgrp[13].root = 88;  kgrp[13].high = 89;  kgrp[13].pos = 560194;  kgrp[13].end = 574121;  kgrp[13].loop = 3414;
  kgrp[14].root = 93;  kgrp[14].high = 999; kgrp[14].pos = 574123;  kgrp[14].end = 586343;  kgrp[14].loop = 2399;

  for (int v = 0; v < NVOICES; ++v) {
    voice[v].env = 0.0f;
    voice[v].dec = 0.99f;  // all notes off
  }

  comb = new float[256];

  guiUpdate = 0;


  apvts.state.addListener(this);
}

MDAPianoAudioProcessor::~MDAPianoAudioProcessor()
{
  if(comb) delete[] comb;

  apvts.state.removeListener(this);
}

const juce::String MDAPianoAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int MDAPianoAudioProcessor::getNumPrograms()
{
  return NPROGS;
}

int MDAPianoAudioProcessor::getCurrentProgram()
{
  return _currentProgram;
}

void MDAPianoAudioProcessor::setCurrentProgram(int index)
{
  printf("%d %f\n", index, _programs[index].param[0]);

  _currentProgram = index;
//update();

  // TODO: I guess this should set the parameters?
  // TODO: AudioPluginHost passes in the wrong index???

  auto param = apvts.getParameter("Envelope Decay");
  param->setValueNotifyingHost(_programs[index].param[0]);
}

const juce::String MDAPianoAudioProcessor::getProgramName(int index)
{
  return { _programs[index].name };
}

void MDAPianoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

void MDAPianoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  Fs = sampleRate;
  iFs = 1.0f / Fs;

//TODO:???
  if (Fs > 64000.0f) cmax = 0xFF; else cmax = 0x7F;

  resetState();
  _parametersChanged.store(true);
}

void MDAPianoAudioProcessor::releaseResources()
{
}

void MDAPianoAudioProcessor::reset()
{
  resetState();
}

bool MDAPianoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//void mdaPiano::setParameter(int index, float value)
//{
//  programs[curProgram].param[index] = value;
//  update();
//
////  if(editor) editor->postUpdate(); //For GUI
//
//  guiUpdate = index + 0x100 + (guiUpdate & 0xFFFF00);
//}
//
//float mdaPiano::getParameter(int index)     { return programs[curProgram].param[index]; }

void MDAPianoAudioProcessor::createPrograms()
{
  _programs.push_back(MDAPianoProgram("mda Piano",        0.500f, 0.500f, 0.500f, 0.5f, 0.803f, 0.251f, 0.376f, 0.500f, 0.330f, 0.500f, 0.246f, 0.500f));
  _programs.push_back(MDAPianoProgram("Plain Piano",      0.500f, 0.500f, 0.500f, 0.5f, 0.751f, 0.000f, 0.452f, 0.000f, 0.000f, 0.500f, 0.000f, 0.500f));
  _programs.push_back(MDAPianoProgram("Compressed Piano", 0.902f, 0.399f, 0.623f, 0.5f, 1.000f, 0.331f, 0.299f, 0.499f, 0.330f, 0.500f, 0.000f, 0.500f));
  _programs.push_back(MDAPianoProgram("Dance Piano",      0.399f, 0.251f, 1.000f, 0.5f, 0.672f, 0.124f, 0.127f, 0.249f, 0.330f, 0.500f, 0.283f, 0.667f));
  _programs.push_back(MDAPianoProgram("Concert Piano",    0.648f, 0.500f, 0.500f, 0.5f, 0.298f, 0.602f, 0.550f, 0.850f, 0.356f, 0.500f, 0.339f, 0.660f));
  _programs.push_back(MDAPianoProgram("Dark Piano",       0.500f, 0.602f, 0.000f, 0.5f, 0.304f, 0.200f, 0.336f, 0.651f, 0.330f, 0.500f, 0.317f, 0.500f));
  _programs.push_back(MDAPianoProgram("School Piano",     0.450f, 0.598f, 0.626f, 0.5f, 0.603f, 0.500f, 0.174f, 0.331f, 0.330f, 0.500f, 0.421f, 0.801f));
  _programs.push_back(MDAPianoProgram("Broken Piano",     0.050f, 0.957f, 0.500f, 0.5f, 0.299f, 1.000f, 0.000f, 0.500f, 0.330f, 0.450f, 0.718f, 0.000f));
}

void MDAPianoAudioProcessor::resetState()
{
  notes[0] = EVENTS_DONE;
  volume = 0.2f;
  muff = 160.0f;
  cpos = sustain = activevoices = 0;

  memset(comb, 0, sizeof(float) * 256);
}

void MDAPianoAudioProcessor::update()
{
  // -50 to +50 --> -6 to +6
  float param2 = apvts.getRawParameterValue("Hardness Offset")->load();
  param2 = (param2 + 50.0f) / 100.0f;  // back to 0 - 1
  size = int(12.0f * param2 - 6.0f);

  // 0 to 100 --> 0 to 0.12
  float param3 = apvts.getRawParameterValue("Velocity to Hardness")->load();
  sizevel = 0.12f * param3 / 100.0f;

  // 0 to 100 --> 0 to 5 but curved
  float param5 = apvts.getRawParameterValue("Velocity to Muffling")->load();
  muffvel = (param5 / 100.0f) * (param5 / 100.0f) * 5.0f;

  // 0 to 100 -->  ???
  float param6 = apvts.getRawParameterValue("Velocity Sensitivity")->load();
  param6 = param6 / 100.0f;
  velsens = 1.0f + param6 + param6;
  if (param6 < 0.25f) velsens -= 0.75f - 3.0f * param6;

  // -50 to +50
  float param9 = apvts.getRawParameterValue("Fine Tuning")->load();
  param9 = (param9 + 50.0f) / 100.0f;  // back to 0 - 1
  fine = param9 - 0.5f;

  float param10 = apvts.getRawParameterValue("Random Detuning")->load();
  random = 0.077f * param10 * param10;

  // -50 to +50
  float param11 = apvts.getRawParameterValue("Stretch Tuning")->load();
  param11 = (param11 + 50.0f) / 100.0f;  // back to 0 - 1
  stretch = 0.000434f * (param11 - 0.5f);

  // 0 to 200
  float param7 = apvts.getRawParameterValue("Stereo Width")->load();
  param7 /= 200.0f;  // back to 0 - 1
  cdep = param7 * param7;
  trim = 1.50f - 0.79f * cdep;
  width = 0.04f * param7; if (width > 0.03f) width = 0.03f;

  float param8 = apvts.getRawParameterValue("Polyphony")->load();
  poly = 8 + int(24.9f * param8);

  // The remaining parameters are used in noteOn().
}

void MDAPianoAudioProcessor::processEvents(juce::MidiBuffer &midiMessages)
{
  // There are different ways a synth can handle MIDI events. This plug-in does
  // it by copying the events it cares about into an array. Then in the render
  // loop, we loop through this array to process the events one-by-one.
  // Note that controllers such as the sustain pedal are processed right away,
  // at the start of the block, so these are not sample-accurate.

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
        notes[npos++] = deltaFrames;   // delta
        notes[npos++] = data1 & 0x7F;  // note
        notes[npos++] = 0;             // vel
        break;

      // Note on
      case 0x90:
        notes[npos++] = deltaFrames;   // delta
        notes[npos++] = data1 & 0x7F;  // note
        notes[npos++] = data2 & 0x7F;  // vel
        break;

      // Controller
      case 0xB0:
        switch (data1) {
          case 0x01:  // mod wheel
          case 0x43:  // soft pedal
            muff = 0.01f * float((127 - data2) * (127 - data2));
            break;

          case 0x07:  // volume
            volume = 0.00002f * float(data2 * data2);
            break;

          case 0x40:  // sustain pedal
          case 0x42:  // sustenuto pedal
            sustain = data2 & 0x40;
            if (sustain == 0) {
              notes[npos++] = deltaFrames;
              notes[npos++] = SUSTAIN;  // end all sustained notes
              notes[npos++] = 0;
            }
            break;

          default:  // all notes off
            if (data1 > 0x7A) {
              for (int v = 0; v < NVOICES; ++v) voice[v].dec = 0.99f;
              sustain = 0;
              muff = 160.0f;
            }
            break;
        }
        break;

//TODO: how do I cleanly do this?
      // Program change
//      case 0xC0:
//        if (data1 < NPROGS) setProgram(data1);
//        break;

      default: break;
    }

    // Discard events if buffer full!
    if (npos > EVENTBUFFER) npos -= 3;
	}
  notes[npos] = EVENTS_DONE;
  midiMessages.clear();
}

void MDAPianoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
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

  const int sampleFrames = buffer.getNumSamples();

  float *out0 = buffer.getWritePointer(0);
  float *out1 = buffer.getWritePointer(1);

  int event = 0;
  int frame = 0;   // how many samples are already rendered

  while (frame < sampleFrames) {
    // Get the timestamp of the next note on/off event.
    int frames = notes[event++];

    // This catches the EVENTS_DONE special event. There are no events left.
    if (frames > sampleFrames) frames = sampleFrames;

    // The timestamps for the events are relative to the start of the block.
    // Make it relative to the previous event. This tells us how many samples
    // to process until this new event actually happens.
    frames -= frame;

    // When we're done with this event, this is how many samples we will have
    // processed in total.
    frame += frames;

    // Until it's time to process the upcoming event, render the active voices.
    while (--frames >= 0) {
      VOICE *V = voice;
      float l = 0.0f, r = 0.0f;

      for (int v = 0; v < activevoices; ++v) {
        V->frac += V->delta;  //integer-based linear interpolation
        V->pos += V->frac >> 16;
        V->frac &= 0xFFFF;
        if (V->pos > V->end) V->pos -= V->loop;

        //i = (i << 7) + (V->frac >> 9) * (waves[V->pos + 1] - i) + 0x40400000;   //not working on intel mac !?!
        int i = waves[V->pos] + ((V->frac * (waves[V->pos + 1] - waves[V->pos])) >> 16);
        float x = V->env * (float)i / 32768.0f;
        //x = V->env * (*(float *)&i - 3.0f);  //fast int->float

        V->env = V->env * V->dec;  //envelope
        V->f0 += V->ff * (x + V->f1 - V->f0);  //muffle filter
        V->f1 = x;

        l += V->outl * V->f0;
        r += V->outr * V->f0;

// if(!(l > -2.0f) || !(l < 2.0f))
// {
//   printf("what is this shit?   %d,  %f,  %f\n", i, x, V->f0);
//   l = 0.0f;
// }
//if(!(r > -2.0f) || !(r < 2.0f))
// {
//   r = 0.0f;
// }

        V++;
      }

      comb[cpos] = l + r;
      ++cpos &= cmax;
      float x = cdep * comb[cpos];  // stereo simulator

      *out0++ = l + x;
      *out1++ = r - x;
    }

    // Start the new note, or stop it if velocity is 0.
    if (frame < sampleFrames) {
      int note = notes[event++];
      int vel  = notes[event++];
      noteOn(note, vel);
    }
  }

  for (int v = 0; v < activevoices; ++v) {
    if (voice[v].env < SILENCE) voice[v] = voice[--activevoices];
  }

  // Mark events buffer as done.
  notes[0] = EVENTS_DONE;

  protectYourEars(out0, buffer.getNumSamples());
  protectYourEars(out1, buffer.getNumSamples());
}

void MDAPianoAudioProcessor::noteOn(int note, int velocity)
{
  float l = 99.0f;
  int vl = 0;

  if (velocity > 0) {
    if (activevoices < poly) {  //add a note
      vl = activevoices;
      activevoices++;
    } else {  //steal a note
      for (int v = 0; v < poly; ++v) {  //find quietest voice
        if (voice[v].env < l) {
          l = voice[v].env;
          vl = v;
        }
      }
    }

    int k = (note - 60) * (note - 60);
    l = fine + random * ((float)(k % 13) - 6.5f);  //random & fine tune
    if(note > 60) l += stretch * (float)k; //stretch

    int s = size;
    if(velocity > 40) s += (int)(sizevel * (float)(velocity - 40));

    k = 0;
    while(note > (kgrp[k].high + s)) k++;  //find keygroup

    l += (float)(note - kgrp[k].root); //pitch
    l = 22050.0f * iFs * (float)exp(0.05776226505 * l);
    voice[vl].delta = (int)(65536.0f * l);
    voice[vl].frac = 0;
    voice[vl].pos = kgrp[k].pos;
    voice[vl].end = kgrp[k].end;
    voice[vl].loop = kgrp[k].loop;

    voice[vl].env = (0.5f + velsens) * (float)pow(0.0078f * velocity, velsens); //velocity

    float param4 = apvts.getRawParameterValue("Muffling Filter")->load();
    param4 = param4 / 100.0f;
    l = 50.0f + param4 * param4 * muff + muffvel * (float)(velocity - 64); //muffle
    if(l < (55.0f + 0.25f * (float)note)) l = 55.0f + 0.25f * (float)note;
    if(l > 210.0f) l = 210.0f;
    voice[vl].ff = l * l * iFs;
    voice[vl].f0 = voice[vl].f1 = 0.0f;

    voice[vl].note = note; //note->pan
    if(note <  12) note = 12;
    if(note > 108) note = 108;
    l = volume * trim;
    voice[vl].outr = l + l * width * (float)(note - 60);
    voice[vl].outl = l + l - voice[vl].outr;

    float param0 = apvts.getRawParameterValue("Envelope Decay")->load();
    param0 = param0 / 100.0f;

    if(note < 44) note = 44; //limit max decay length
    l = 2.0f * param0;
    if(l < 1.0f) l += 0.25f - 0.5f * param0;
    voice[vl].dec = (float)exp(-iFs * exp(-0.6 + 0.033 * (double)note - l));
  }
  else {  //note off
    float param1 = apvts.getRawParameterValue("Envelope Release")->load();
    param1 = param1 / 100.0f;

    for (int v = 0; v < NVOICES; ++v) {
      if (voice[v].note==note) { //any voices playing that note?
        if(sustain==0)
        {
          if(note < 94 || note == SUSTAIN) //no release on highest notes
            voice[v].dec = (float)exp(-iFs * exp(2.0 + 0.017 * (double)note - 2.0 * param1));
        }
        else voice[v].note = SUSTAIN;
      }
    }
  }
}

juce::AudioProcessorEditor *MDAPianoAudioProcessor::createEditor()
{
  return new juce::GenericAudioProcessorEditor(*this);
}

void MDAPianoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
  copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MDAPianoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

//void MDAPianoAudioProcessor::guiGetDisplay(int index, char *label)
//{
////  getParameterName(index,  label);
////  strcat(label, " = ");
////  getParameterDisplay(index, label + strlen(label));
////  getParameterLabel(index, label + strlen(label));
//}

juce::AudioProcessorValueTreeState::ParameterLayout MDAPianoAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Envelope Decay",
    "Envelope Decay",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Envelope Release",
    "Envelope Release",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Hardness Offset",
    "Hardness Offset",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity to Hardness",
    "Velocity to Hardness",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  // The "Muffling Filter" goes from 100 to 0 but JUCE doesn't let us specify
  // the range starting at 100 and ending at 0, so we reverse it for display.

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Muffling Filter",
    "Muffling Filter",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    0.0f,
    "%",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(100.0f - value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity to Muffling",
    "Velocity to Muffling",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    50.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Velocity Sensitivity",
    "Velocity Sensitivity",
    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
    33.0f,
    "%"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Stereo Width",
    "Stereo Width",
    juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
    100.0f,
    "%"));

//TODOL could just do 8 - 32 with a step of 1
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Polyphony",
    "Polyphony",
    juce::NormalisableRange<float>(),
    0.33f,
    "voices",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      int poly = 8 + int(24.9f * value);
      return juce::String(poly);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Fine Tuning",
    "Fine Tuning",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "cents"));

//TODO: could do this with a skew
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Random Detuning",
    "Random Detuning",
    juce::NormalisableRange<float>(),
    0.0f,
    "cents",
    juce::AudioProcessorParameter::genericParameter,
    [](float value, int) {
      return juce::String(50.0f * value * value, 1);
    }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
    "Stretch Tuning",
    "Stretch Tuning",
    juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f),
    0.0f,
    "cents"));

  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new MDAPianoAudioProcessor();
}
