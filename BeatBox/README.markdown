# BeatBox

Drum replacer / enhancer

Contains three samples (kick, snare and hat) designed to be triggered by incoming audio in three frequency ranges. The plug-in has built-in drum sounds based on the *Roland CR-78*.

| Parameter | Description |
| --------- | ----------- |
| Hat Thr | Trigger threshold level |
| Hat Rate | Maximum trigger rate |
| Hat Mix | Sample playback level |
| Kik Thr | |
| Kik Trig | Trigger filter frequency - switches to "key listen" mode while adjusted |
| Kik Mix | |
| Snr Thr |  |
| Snr Trig | Trigger filter frequency - increase if snare sample is triggered by kick drum |
| Snr Mix | |
| Dynamics | Apply input signal level variations to output |
| Thru Mix | Allow some of the input signal to be mixed with the output |

The original plug-in also had a recording mode parameter that let the (advanced) user replace the hi-hat, kick, and snare samples. I did not include this in the JUCE version as it wasn't very convenient to use.
