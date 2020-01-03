#include <gst/gst.h>

#ifndef __GHOST__
#define __GHOST__

// Create a new ghost pad to the element of the bin & add the ghost pad to the bin
static void setup_ghost_src(GstElement *src, GstBin *bin)
{
  GstPad *src_pad = gst_element_get_static_pad(src, "src");
  GstPad *bin_pad = gst_ghost_pad_new("src", src_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
}


static void setup_ghost_sink(GstElement *sink, GstBin *bin)
{
  GstPad *sink_pad = gst_element_get_static_pad(sink, "sink");
  GstPad *bin_pad = gst_ghost_pad_new("sink", sink_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
}

#endif

