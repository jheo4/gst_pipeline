# Handy Elements

## Bins
  Bin elements: a single element instantiating all the necessary internal pipeline
  - playbin: manages all aspects of media playback.
  - uridecodebin: decodes data from a URI to raw media by seleting a suitable source element and connecting the source
                  to a decodebin element.
      gst-launch-1.0 uridecodebin
          uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! videoconvert !
          autovideosink
      gst-launch-1.0 uridecodebin
          uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! audioconvert !
          autoaudiosink
  - decodebin: constructs a decoding pipeline via auto-plugging.
      gst-launch-1.0 souphttpsrc
          location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! decodebin !
          autovideosink


## File I/O
  - filesrc: reads a local file & produces media with ANY Caps.
      gst-launch-1.0 filesrc location=f:\\media\\sintel\\sintel_trailer-480p.webm ! decodebin ! autovideosink
  - filesink: writes to a file all the received media.
			gst-launch-1.0 audiotestsrc ! vorbisenc ! oggmux ! filesink location=test.ogg


## Network
	- souphttpsrc: receives data as a client over the network via HTTP using the libsoup library.


## Test Media Generation
	very useful to check if other parts of the pipeline are working by replacing the source.
	- videotestsrc
			gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
	- audiotestsrc
			gst-launch-1.0 audiotestsrc ! audioconvert ! autoaudiosink


## Video Adapters
	- videoconvert: converts from one color space to another one (e.g. RGB to YUV). it is normally the first choice when
									solving negotiation problems; use videoconvert whenever you use elemtns whose Caps are unknown at
									design time like decoding a user-provided file.
			gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
	- videorate: produces a stream matching the source pad's frame rate from an incoming stream of time-stamped video
							 frames by dropping and duplicating frames. useful to link elements requiring different frame rates;
							 always use it whenever the actual frame rate is unknown at design time.
			gst-launch-1.0 videotestsrc ! video/x-raw,framerate=30/1 ! videorate ! video/x-raw,framerate=1/1 ! videoconvert !
					autovideosink
	- videoscale: resizes video frames.
			gst-launch-1.0 uridecodebin
					uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! videoscale !
				  video/x-raw,width=178,height=100 ! videoconvert ! autovideosink


## Audio Adapters
	- audioconvert: converts raw audio buffers between possible formats. use this to solve negotiation problems with
								  audio.
			gst-launch-1.0 audiotestsrc ! audioconvert ! autoaudiosink
	- audioresample: resamples raw audio buffers to different sampling rates using a configurable windowing function.
			gst-launch-1.0
					uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm !
				  audioresample ! audio/x-raw-float,rate=4000 ! audioconvert ! autoaudiosink


## Multithreading
	- queue: performs two tasks below and triggers signals when it is about to be empty or full. it can be instructed to
					 drop buffers instead of blocking when it is full.
			- blocking the pushing threads attempting to push more buffers into the queue blocks
			- creating a new thread on the source Pad to decouple the sink processing and source Pads
	- queue2: has the same design goals with queue but follows a different implementation approach. So, prefer queue2
						over queue whenever network buffering is a concern.
			- with the two tasks above, storing the received data on a disk file
			- uses the buffereing message via bus instead of the signals
	- multiqueue: provides queues for multiple streams simultaneousl and sync the different streams.
	- tee: splits data to multiple pads (multiple flows). For separate threads for each branch, use separate queue
				 elements in each branch; otherwise a blocked dataflow in one branch will stall the other branches.
			gst-launch-1.0 audiotestsrc ! tee name=t ! queue ! audioconvert ! autoaudiosink t. ! queue ! wavescope !
					videoconvert ! autovideosink

## Capabilities
	- capsfilter: does not modify data but enforces limitations on the data format
			gst-launch-1.0 videotestsrc ! video/x-raw, format=GRAY8 ! videoconvert ! autovideosink
	- typefind: determines the type of media a stream contains. once the type has been detected, sets source Pad Caps &
						  emits have-type signal.

## Debugging
	- fakesink: swallows any data fed to it.
			gst-launch-1.0 audiotestsrc num-buffers=1000 ! fakesink sync=false
	- identity: a dummy element passing incoming data to diagnose offset, timestamp, or buffer dropping.
			gst-launch-1.0 audiotestsrc ! identity drop-probability=0.1 ! audioconvert ! autoaudiosink

