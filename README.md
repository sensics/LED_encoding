# LED_encoding
Program to compute the maximum instantaneous power draw for different strides and bit depths

This program computes LED bright/dark patterns for the OSVR HDK and shows how
much instantaneous overlap there is among bright LEDs in the different patterns.
You can select the number of LEDs, the number of bits to encode them in, and
the stride to pick for time offsets among the LEDs.

The LED mapping is as described in the
[OSVR-Core video-based-tracker Developing document](https://github.com/OSVR/OSVR-Core/blob/master/plugins/videobasedtracker/doc/Developing.md).

