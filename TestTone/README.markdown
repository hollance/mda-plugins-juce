# TestTone

Signal generator with pink and white noise, impulses and sweeps.

Note: the AU version of this plug-in has a bunch of extra features and improvements that I didn't convert yet.

| Parameter | Description |
| --------- | ----------- |
| Mode | see below |
| Level | Peak output level |
| Channel | Generate signals on left or right channel only |
| F1 | Base frequency (not applicable to pink and white noise or impulses) |
| F2 | Fine frequency control, or end frequency for sweep modes |
| Sweep | Sweep duration for sweep modes (2 seconds silence is also added between sweeps). Sets repetition rate in inpulse mode |
| Thru | Allow the input signal to pass through the plug-in |
| 0 dB = | Calibrate output so indicated level is relative to, for example -0.01 dB FS or -18 dB FS |

Possible modes:

- MIDI #: sine waves at musical pitches (A3 = 69 = 440 Hz)
- IMPULSE: single-sample impulse
- WHITE: white noise
- PINK: pink noise
- SINE: ISO 1/3-octave frequencies
- LOG SWP: logarithmic frequency sweep
- LOG STEP: 1/3-octave steps
- LIN SWP: linear frequency sweep
