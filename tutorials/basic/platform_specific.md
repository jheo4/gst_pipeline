# Platform-specific elements

## Cross Platform
  - glimagesink: a video sink available to Android and iOS (based on OpenGL). it can be decomposed into
                 `glupload ! glcolorconvert ! glimagesinkelement`


## Linux
  - ximagesink: a RGB only X-based video sink
  - xvimagesink: a X-based video sink using X Video Extension; it should be supported by GPUs and drivers.
  - alsasink: outputs to the sound card via ALSA (Advanced Linux Sound Architecture).
  - pulsesink: plays audio to a PulseAudio server.


## Mac OS X
  - osxvideosink: video sink available to OS X
  - osxaudiosink: only audio sink available to OS X


## Windows
  - directdrawsink: a DirectDraw-based video sink supporting rescaling and filtering
  - dshowvideosink: a video sink based on DirectDraw allowing different rendering back-ends
  - d3dvideosink: a video sink based on Direct3D supporting rescaling and filtering
  - directsoundsink: the default audio sink based on Direct Sound
  - dshowdecwrapper: a multimedia framework like GStreamer wrapping multiple decoders; it can be embedded in a
                     GStreamer pipeline.


## Android
  - openslessink: the only audio sink on Android based on OpenSL ES
  - openslessrc: the only audio source on Android based on OpenSL ES
  - androidmedia: accesses the codecs available on the Android device including hardware codecs; On Android, HW codecs
                  with glimagesink can attain a high performance & zero-copy decodebin pipeline.


## iOS
  - osxaudiosink: the only audio sink available on iOS
  - iosassetsrc: reads iOS assets.
  - iosavassetsrc: reads and decodes iOS audiovisual assets; dedicated hardware can be used for decoding.

