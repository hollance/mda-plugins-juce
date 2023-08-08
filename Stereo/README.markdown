# Stereo

Stereo Simulator: Add artificial width to a mono signal. Haas delay and comb filtering.

This plug converts a mono signal to a 'wide' signal using two techniques: Comb filtering where the signal is panned left and right alternately with increasing frequency, and Haas panning where a time delayed signal sounds appears to be at the same level as a quieter non-delayed version.

Comb mode is mono compatible. Haas mode is not, but sounds fine on some signals such as backing vocals. This plug-in must be used in a stereo channel or bus!

| Parameter | Description |
| --------- | ----------- |
| Width | Stereo width (turn to right for comb filter mode, left for Haas panning mode) |
| Delay | Delay time - use higher values for more filter "combs" across the frequency spectrum |
| Balance | Balance correction for Haas mode |
| Mod | Amount of delay modulation (defaults to OFF to reduce processor usage) |
| Rate | Modulation rate (note that modulation completely disappears in mono) |
