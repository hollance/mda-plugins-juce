# MDA plug-ins in JUCE

This repo contains the [MDA freeware plug-ins](http://mda.smartelectronix.com) implemented in [JUCE](https://juce.com). 

The [original plug-in source code](https://sourceforge.net/projects/mda-vst/) is for VST2, which is no longer supported. For my own elucidation and enjoyment, I decided to rewrite these plug-ins in JUCE.

The MDA plug-ins were originally written by Paul Kellett, about two decades ago. I also consulted the AU versions by [Sophia Poirier](http://destroyfx.org).

These source examples are provided mainly for **learning purposes**! As Paul said of his original source code release:

> This code is definitely not an example of how to write plug-ins! It's obvious that I didn't know much C++ when I started, and some of the optimizations might have worked on a 486 processor but are not relevant today.  The code is very raw with no niceties like parameter de-zipping, but maybe you'll find some useful stuff in there.  

I've tried to fix some of these issues but did not add any new features such as the parameter dezipping. The code here is 20 years old, so it may no longer be the most optimal way to implement these algorithms. 

That said, it's still **a good place to get started** if you're learning about audio DSP, which is why I added lots of comments to describe what's going on. If you have a [basic understanding of JUCE](https://www.youtube.com/c/TheAudioProgrammer), you should be able to follow along. (See below for tips on getting started.)

Some notes:

- The code has been cleaned up a bit and documented, and occasionally bug fixed.
- These plug-ins have no UI and use the default generic JUCE UI.
- The MDA plug-ins were VST2, meaning that the parameters are always 0 - 1. Since JUCE allows us to have parameters in any range, they were changed to whatever felt more more natural (in dB, Hz, etc).
- I'm not using the JUCE coding style because it's ugly. ;-)
- The code has only been tested with Xcode on a Mac using JUCE 6.1, but should work on Windows too.

This source code is licensed under the [MIT license](LICENSE.txt) but keep in mind that JUCE has its own licensing terms. JUCE conversion done by Matthijs Hollemans.

## Included plug-ins

You can find the [original documentation here](http://mda.smartelectronix.com/vst/help/mdaplugs.htm) — if you're not sure how to use a particular plug-in, check here first.

The plug-ins are ordered from simple to complicated. If you're new to audio programming, start at the top and work your way down the list.

### RingMod

This was the first "mda" effect, made way back in 1998.  It is a simple ring modulator, multiplying the input signal by a sine wave, the frequency of which is set using the Frequency and Fine Tune controls.  As if ring modulation wasn't harsh enough already, the Feedback parameter can add even more edge to the sound!

### Overdrive

Soft distortion

### Degrade

This plug-in reduces the bit-depth and sample rate of the input audio (using the Quantize and Sample Rate parameters) and has some other features for attaining the sound of vintage digital hardware.  The headroom control is a peak clipper, and the Non-Linearity controls add some harmonic distortion to thicken the sound.  Post Filter is a low-pass filter to remove some of the grit introduced by the other controls.

## Plug-ins that have not been converted yet

- Bandisto - Multi-band distortion
- BeatBox - Drum replacer 
- Combo - Amp & speaker simulator
- De-ess - High frequency dynamics processor
- Delay - Simple stereo delay with feedback tone control
- Detune - Simple up/down pitch shifting thickener
- Dither - Range of dither types including noise shaping 
- DubDelay - Delay with feedback saturation and time/pitch modulation
- Dynamics - Compressor / Limiter / Gate
- Envelope - Envelope follower / VCA
- Image - Stereo image adjustment and M-S matrix
- Leslie - Rotary speaker simulator
- Limiter - Opto-electronic style limiter
- Loudness - Equal loudness contours for bass EQ and mix correction 
- Multiband - Multi-band compressor with M-S processing modes 
- Re-Psycho! - Drum loop pitch changer
- RezFilter - Resonant filter with LFO and envelope follower
- Round Panner - 3D panner
- Shepard - Continuously rising/falling tone generator
- Splitter - Frequency / level crossover for setting up dynamic processing
- Stereo Simulator - Haas delay and comb filtering
- Sub-Bass Synthesizer - Several low frequency enhancement methods
- Talkbox - High resolution vocoder
- TestTone - Signal generator with pink and white noise, impulses and sweeps
- Thru-Zero Flanger - Classic tape-flanging simulation
- Tracker - Pitch tracking oscillator, or pitch tracking EQ
- Vocoder - Switchable 8 or 16 band vocoder
- VocInput - Pitch tracking oscillator for generating vocoder carrier input

## How the code is structured

Read this section if you're new to JUCE or audio programming! Or if you're wondering why the code is structured the way it is...

Each plug-in is in its own folder. Go into the folder, open the **.jucer** file in Projucer and then launch your IDE from the Projucer project. Build the VST3 target and open it in JUCE's AudioPluginHost for testing.

Since there is no UI for these plug-ins, the only source files are **PluginProcessor.h** and **.cpp**.

The main functions in PluginProcessor are:

- `createParameterLayout()`: this is where the parameters are defined
- `prepareToPlay()`: initializes the plug-in
- `resetState()`: sets things back to zero, clears out any delay lines, and so on. This method is called when the plug-in is initialized in `prepareToPlay` but also when the plug-in is `reset`.
- `update()`: takes the current parameter values and recomputes whatever depends on these parameters. This method is called from `processBlock` but only when parameters have actually changed. 
- `processBlock()`: does the actual audio processing

The PluginProcessor has a number of member variables. Their names begin with an underscore, such as `_level`. There are basically two types of member variables: 

1. "cooked" parameters 
2. rendering state

The cooked parameters are filled in by the `update()` method. This reads the current parameter values from the APVTS and then puts the cooked version into the member variable. For example:

```c++
void MDARingModAudioProcessor::update()
{
  // Convert from decibels into a linear gain value.
  float level = apvts.getRawParameterValue("Level")->load();
  _level = juce::Decibels::decibelsToGain(level);
}
```

The `update()` method is called by the audio thread at the start of `processBlock()`. To be a bit more efficient, this only happens when any of the parameters have actually been changed by the user or by the host through automation. After all, if the parameters didn't change then calling `update()` again will not change any of these member variables either. 

This is managed by an `atomic<bool>` variable. The PluginProcessor is a `ValueTree::Listener` that listens to changes in the `AudioProcessorValueTreeState` that holds all the parameters. When a parameter gets a new value, this listener sets `_parametersChanged` to true. The audio thread will call `update()` only if it sees this boolean is true. This approach is probably overkill for most of these plug-ins but it's how I generally structure this.

The second type of variable keeps track of rendering state. This is something like the current phase of an oscillator or the delay unit of a filter. These variables are given their initial value by `resetState()` and will be changed by `processBlock`.

Inside `processBlock()` we first read the member variables into local variables (for state) or constants (for parameters). Then the processing loop uses these local variables instead of the member variables. If the audio processing loop updates any of the state (which it usually does), the latest values get copied back into the corresponding member variables after the loop. This is how the original plug-ins did it, and I kept the same approach.
