# Hardware-accelerated video decoding
https://bit.ly/369P8GC

If APIs and drivers are properly setup, GStreamer automatically takes advandage of the accelerations.

## Inner workings of HW-accelerated video decoding plugins
When HW acceleration is used, the video frames are moved between GPU memory and system memory. GStreamer needs to keep
track of where the hardware buffers are through, and conventional buffers still travel from element to element. HW
buffers look like regular buffers, but mapping their contents much slower because of memory transfer.

If HW-acceleration APIs are present in the system, the corresponding GStreamer plugin is also available to using
HW acceleration.
