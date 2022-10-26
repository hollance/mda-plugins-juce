# MDA plug-ins in JUCE

This repo contains the [MDA freeware plug-ins](http://mda.smartelectronix.com) implemented in [JUCE](https://juce.com).

The [original plug-in source code](https://sourceforge.net/projects/mda-vst/) is for VST2, which is no longer supported. For my own elucidation and enjoyment, I decided to rewrite these plug-ins in JUCE.

The MDA plug-ins were originally written by Paul Kellett, about two decades ago. I also consulted the AU versions by [Sophia Poirier](http://destroyfx.org).

These source examples are provided mainly for **learning purposes**! As Paul said of his original source code release:

> This code is definitely not an example of how to write plug-ins! It's obvious that I didn't know much C++ when I started, and some of the optimizations might have worked on a 486 processor but are not relevant today.  The code is very raw with no niceties like parameter de-zipping, but maybe you'll find some useful stuff in there.

I've tried to fix some of these issues but did not add any new features such as the parameter dezippering. The code here is 20 years old, so it may no longer be the most optimal way to implement these algorithms. Consider this project to be a kind of plug-in archeology. ;-)

That said, it's still **a good place to get started** if you're learning about audio DSP, which is why I added lots of comments to describe what's going on. If you have a [basic understanding of JUCE](https://www.youtube.com/c/TheAudioProgrammer), you should be able to follow along. (See below for tips on getting started.)

Some notes:

- The code has been cleaned up a bit and documented, and occasionally bug fixed.
- These plug-ins have no UI and use the default generic JUCE UI.
- I'm not using the JUCE coding style because it's ugly. ;-)
- The code has only been tested with Xcode on a Mac using JUCE 7, but should work on Windows too.

This source code is licensed under the [MIT license](LICENSE.txt) but keep in mind that JUCE has its own licensing terms. JUCE conversion done by Matthijs Hollemans.

## Included plug-ins

You can find the [original documentation here](http://mda.smartelectronix.com/vst/help/mdaplugs.htm) — if you're not sure how to use a particular plug-in, check there first.

If you're new to audio programming and want to learn how these plug-ins work, start with one of the following easy plug-ins:

- RingMod
- Overdrive
- Degrade
- Delay

If you're interested in synths, check out:

- MDA Piano
- MDA EPiano
- JX10 analog synth
- DX10 FM synth

### Ambience

Small space reverberator. Designed to simulate a distant mic in small rooms, without the processor overhead of a full reverb plug-in. Can be used to add "softness" to drums and to simulate someone talking "off mic".

[Docs](http://mda.smartelectronix.com/vst/help/ambience.htm)

### Degrade

This plug-in reduces the bit-depth and sample rate of the input audio (using the Quantize and Sample Rate parameters) and has some other features for attaining the sound of vintage digital hardware.  The headroom control is a peak clipper, and the Non-Linearity controls add some harmonic distortion to thicken the sound.  Post Filter is a low-pass filter to remove some of the grit introduced by the other controls.

[Docs](http://mda.smartelectronix.com/vst/help/degrade.htm)

### Delay

Simple stereo delay with feedback tone control. [Docs](http://mda.smartelectronix.com/vst/help/delay.htm)

### DX10

Simple FM synthesizer. Sounds similar to the later Yamaha DX synths including the heavy bass but with a warmer, cleaner tone.

[Docs](http://mda.smartelectronix.com/vst/help/dx10.htm)

### EPiano

Rhodes Piano. This plug-in is extremely similar to MDA Piano, but uses different samples. It also has some basic LFO modulation.

### JX10

Simple 2-oscillator analog synthesizer. 8 voice polyphonic.

[Docs](http://mda.smartelectronix.com/vst/help/jx10.htm)

### Limiter

Opto-electronic style limiter. [Docs](http://mda.smartelectronix.com/vst/help/limiter.htm)

### Overdrive

Soft distortion plug-in. [Docs](http://mda.smartelectronix.com/vst/help/overdrive.htm)

### Piano

Acoustic piano instrument — this was quite a [popular free piano synth](https://www.kvraudio.com/product/piano-by-mda) back in the day. [Docs](http://mda.smartelectronix.com/vst/help/piano.htm)

### RingMod

This was the first "mda" effect, made way back in 1998.  It is a simple ring modulator, multiplying the input signal by a sine wave, the frequency of which is set using the Frequency and Fine Tune controls.  As if ring modulation wasn't harsh enough already, the Feedback parameter can add even more edge to the sound!

[Docs](http://mda.smartelectronix.com/vst/help/ringmod.htm)

### Shepard

Shepard tone generator

This plug-in generates a continuously rising or falling tone.  Or rather, that's what it sounds like but really new harmonics are always appearing at one end of the spectrum and disappearing at the other. (Using some EQ can improve the psychoacoustic effect, depending on your listening environment.)

These continuous tones are actually called "Risset tones", developed from the earlier "Shepard tones" which change in series of discrete steps. The Mode control allows the input signal to be mixed or ring modulated with the tone - this works well as one element of a complex chain of effects.

[Docs](http://mda.smartelectronix.com/vst/help/shepard.htm)

### Stereo

Stereo Simulator: Add artificial width to a mono signal. Haas delay and comb filtering. [Docs](http://mda.smartelectronix.com/vst/help/stereo.htm)

### SubSynth

Sub-Bass Synthesizer: Several low frequency enhancement methods. [Docs](http://mda.smartelectronix.com/vst/help/subsynth.htm)

### TestTone

Signal generator with pink and white noise, impulses and sweeps. [Docs](http://mda.smartelectronix.com/vst/help/testtone.htm)

Note: the AU version of this plug-in has a bunch of extra features and improvements that I didn't convert yet.

## Plug-ins that have not been converted yet

- Bandisto - Multi-band distortion
- BeatBox - Drum replacer
- Combo - Amp & speaker simulator
- De-ess - High frequency dynamics processor
- Detune - Simple up/down pitch shifting thickener
- Dither - Range of dither types including noise shaping
- DubDelay - Delay with feedback saturation and time/pitch modulation
- Dynamics - Compressor / Limiter / Gate
- Envelope - Envelope follower / VCA
- Image - Stereo image adjustment and M-S matrix
- Leslie - Rotary speaker simulator
- Looplex - ?
- Loudness - Equal loudness contours for bass EQ and mix correction
- Multiband - Multi-band compressor with M-S processing modes
- Re-Psycho! - Drum loop pitch changer
- RezFilter - Resonant filter with LFO and envelope follower
- Round Panner - 3D panner
- SpecMeter
- Splitter - Frequency / level crossover for setting up dynamic processing
- Talkbox - High resolution vocoder
- Thru-Zero Flanger - Classic tape-flanging simulation
- Tracker - Pitch tracking oscillator, or pitch tracking EQ
- Vocoder - Switchable 8 or 16 band vocoder
- VocInput - Pitch tracking oscillator for generating vocoder carrier input

## How the code is structured

Read this section if you're new to JUCE or audio programming! Or if you're wondering why the code is structured the way it is...

Each plug-in is in its own folder. Go into the folder, open the **.jucer** file in Projucer and then launch your IDE from the Projucer project. Build the VST3 target and open it in JUCE's AudioPluginHost for testing.

Since there is no UI for these plug-ins, the only source files are **PluginProcessor.h** and **.cpp**.

### PluginProcessor

The main functions in PluginProcessor are:

- `createParameterLayout()`: this is where the parameters are defined
- `prepareToPlay()`: initializes the plug-in
- `resetState()`: sets things back to zero, clears out any delay lines, and so on. This method is called when the plug-in is initialized in `prepareToPlay` but also when the plug-in is `reset`.
- `update()`: takes the current parameter values and recomputes whatever depends on these parameters. This method is called from `processBlock` but only when parameters have actually changed.
- `processBlock()`: does the actual audio processing

The PluginProcessor has a number of member variables. Their names begin with an underscore, such as `_level`.

There are two types of member variables:

1. "cooked" parameters
2. rendering state

The cooked parameters are filled in by the `update()` method. This method is called by the audio thread at the start of `processBlock()`.  It reads the current parameter values from the APVTS and then puts the cooked version into the member variable. For example:

```c++
void MDARingModAudioProcessor::update()
{
    // Convert from decibels into a linear gain value.
    float level = apvts.getRawParameterValue("Level")->load();
    _level = juce::Decibels::decibelsToGain(level);
}
```

The second type of variable keeps track of rendering state. This is something like the current phase of an oscillator or the delay unit of a filter. These variables are given their initial value by `resetState()` and will be changed by `processBlock`.

Inside `processBlock()` we first read the member variables into local variables (for state) or constants (for parameters). Then the processing loop uses these local variables instead of the member variables. If the audio processing loop updates any of the state (which it usually does), the latest values get copied back into the corresponding member variables after the loop. I'm not sure how useful it is to copy the member variables into local variables, since both will be implemented as a load from a register using an offset, but this is how the original plug-ins did it.

### Parameter calculations

The MDA plug-ins were VST2, meaning that the parameters are always 0 - 1. Since JUCE allows us to have parameters in any range, they were changed to whatever felt more more natural (in dB, Hz, etc).

Let's say the plug-in defines an Output Level parameter. Like all VST2 parameters, this has a range of 0 - 1. But in the UI we want to display this as decibels. In the original code, the `getParameterDisplay()` function converted this 0 - 1 into a decibel value:

```c++
int2strng((VstInt32)(40.0 * fParam - 20.0), text);
```

Here, `fParam` is the 0 - 1 value. The formula used here will show a range from -20 dB (when `fParam = 0`) to +20 dB (when `fParam = 1`). Note that this `getParameterDisplay()` function is only used for drawing the UI, not for anything else.

In the audio processing code, the parameter is still 0 - 1. This value, which represents a range of decibels, is converted into a linear gain so that we can multiply it with the audio.

```c+++
g3 = (float)(pow(10.0, 2.0 * fParam - 1.0));
```

You may be wondering where this formula comes from. To go from decibels to a linear value, we do `pow(10, dB / 20)`. That is similar to what happens here, but not exactly the same. That's because our parameter isn't in decibels, it's 0 - 1. We first need to convert it to decibels:

```c++
  pow(10, dB / 20)
= pow(10, (40 * fParam - 20) / 20)
= pow(10, (40 * fParam / 20) - (20 / 20))
= pow(10, 2 * fParam - 1)
```

And that is indeed the formula the original plug-in used.

Fortunately, we don't need to do any of this. In the JUCE implementation, we can simply define the parameter to go from -20 to +20 dB. And then the `update()` function does:

```c++
float outputLevel = apvts.getRawParameterValue("Output")->load();  // -20 to +20 dB
g3 = juce::Decibels::decibelsToGain(outputLevel);
```

We can let JUCE handle the math, and we get to work with parameters that have a more natural range than 0 - 1.

However, sometimes it's a little more tricky. In the MDA Delay plug-in, the delay time in samples was calculated using the formula:

```c++
ldel = (VstInt32)(size * fParam0 * fParam0);
```

Here, `size` is the maximum length of the delay buffer in samples, and `fParam0` is a value between 0 and 1 as usual. Notice that `fParam0` gets squared, I'll explain why shortly.

The `getParameterDisplay()` for this parameter was defined as:

```c++
int2strng((VstInt32)(ldel * 1000.0f / getSampleRate()), text);
```

Rather than deriving this from `fParam0` directly, it takes the computed delay in samples `ldel`, and converts it to a time in milliseconds that is shown to the user.

In the JUCE version, I replaced this by a parameter that lets you directly choose the delay time in milliseconds, which seemed like a simpler approach. Instead of going from 0 - 1, the parameter goes to 0 to 500 ms. Makes sense, right?

However, recall that the audio processing logic squares the parameter value in the formula `size * fParam0 * fParam0`. Since that parameter goes from 0 - 1, this creates a nice little x^2 curve. This kind of thing is usually done to make it easier to pick smaller delays. For example, at a sample rate of 44100, the maximum delay length is 22050 samples. With the parameter set to 0.5, the delay is not 11025 (= half) but 5512 samples (= half squared). It makes the slider non-linear, which is what you want for things like times and frequencies.

But we are not working with a normalized value from 0 - 1 anymore, our parameter is in milliseconds already. Squaring that number doesn't make any sense.

Remember that squaring was only done to make the slider work more logarithmically, giving more room to smaller delays than to larger delays. In JUCE, we can achieve this by giving the slider a skew factor. Now our slider is non-linear too, just like in the original plug-in. But we don't have to do any of the math for it ourselves, JUCE takes care of this for us.

We can simply calculate the delay time linearly, without squaring the parameter:

```c++
const float samplesPerMsec = float(getSampleRate()) / 1000.0f;
float ldelParam = apvts.getRawParameterValue("L Delay")->load();  // 0 - 500 ms
ldel = int(ldelParam * samplesPerMsec);
```

### Denormals

Many plug-ins have code to flush denormals, for example like the following. This sets the variable to zero if the number becomes too small:

```c++
if (std::abs(f) > 1.0e-10f) {
    _filter = f;
} else {
    _filter = 0.0f;
}
```

I left this code in the plug-in even though it is not strictly necessary, as we use `juce::ScopedNoDenormals` to automatically flush denormals to zero.
