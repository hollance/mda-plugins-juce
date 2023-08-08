# Envelope

Envelope follower and VCA

| Parameter | Description |
| --------- | ----------- |
| Output | see below |
| Attack | |
| Release | |
| Gain | Level trim |

The Output parameter can have the following values:

- **L x |R|**: multiplies left channel signal by right channel level (VCA mode)
- **IN/ENV**: left channel is not processed, right output is a DC signal derived from the input signal level
- **FLAT/ENV**: as IN/ENV but the left channel is processed to remove level variations

Normally two Envelope plug-ins will be used with the first set to **FLAT/ENV** to separate the signal waveform and envelope and the second set to **L x |R|** to recombine them. Effects applied to either the left or right channel between these plug-ins can be used to process the waveform and envelope separately, for example delaying the envelope relative to the waveform, or applying waveshaping with a constant "drive" level.

Note that in **L x |R|** mode with a mono input the plug-in acts as a 2:1 expander.

> **WARNING!** This plug is intended for advanced users and is best suited to modular environents such as AudioMulch. It can output high levels of DC which may damage speakers and other equipment.
