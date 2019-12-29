# GStreamer Tools

## gst-launch-1.0
  - accepts a textual description of a pipeline & instantiates it & sets it to the PLAYING state.
  - for quickly checking if a given simple pipeline works before going through the actual implementation using
    GStreamer API calls
  - primarily a debugging tool; another alternative is to use gst_parse_launch().

  examples
		- gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
		- gst-launch-1.0 videotestsrc pattern=11 ! videoconvert ! autovideosink
				Properties may be appended to elements, in the form *property=value; multiple properties can be specified,
		  	separated by spaces.
		- gst-launch-1.0 videotestsrc ! videoconvert ! tee name=t ! queue ! autovideosink t. ! queue ! autovideosink
				Names allow linking to elements created previously in the description, and are indispensable to use elements with
				multiple output pads, like demuxers or tees; Named elements are referred to using their name followed by a dot.
		- gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm \
					! matroskademux name=d d.video_00 ! matroskamux ! filesink location=sintel_video.mkv
				The pads can be directly linked by adding a dot plus the Pad name after the name of the element.
		- gst-launch-1.0 souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm \
					! matroskademux ! video/x-vp8 ! matroskamux ! filesink location=sintel_video.mkv
				If there are more than one compatible Pad, the ambiguity can be removed by using Caps Filters
		- gst-launch-1.0 \
				  uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm name=d \
				  ! queue ! theoraenc ! oggmux name=m ! filesink location=sintel.ogg d. ! queue ! audioconvert ! audioresample   \
					! flacenc ! m.
				Transcoding example
		- gst-launch-1.0 uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm \
				  ! queue ! videoscale ! video/x-raw-yuv,width=320,height=200 ! videoconvert ! autovideosin
				Rescaling example


## gst-launch-1.0
	- 3 operation modes
		- without arguments, it lists all available elements types.
		- with a file name as an argument, it treats the file as a GStreamer plugin, tries to open it, and lists all the
			elements described inside.
		- with a GStreamer element name as an argument, it lists all information regarding that element.


## gst-discoverer-1.0
	- a wrapper around the GstDiscoverer object
	- accepts a URI from the commnad line and prints all info about the media GStreamer can extract


## Debugging tools
	- ENV GST_DEBUG
		-	the debug output can be controlled with this env variable (https://bit.ly/2ZB4pOx).
		- a comma-separted list of category:level pairs with an optional level at the beginning (GST_DEBUG=2,audio*:6).
	- your own debug info
		- use the GST_ERROR(), GST_WARNING(), GST_INFO(), GST_LOG() and GST_DEBUG() macros; they use default category in
			the output log.
		- To change the category, write these codes & the last line after gst_init()
			```GST_DEBUG_CATEGORY_STATIC (my_category);
			#define GST_CAT_DEFAULT my_category
			GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");```


## Getting pipeline graphs
  - GStreamer can export pipeline graphs into output .dot files which describe the topology of a pipeline along with
    the negotiated in each link.
  - ENV GST_DEBUG_DUMP_DOT_DIR
    - points to the directory where you want the graphs to be placed
  - within an application, you can use the GST_DEBUG_BIN_TO_DOT_FILE() and GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS() macros
    to generate .dot files.

