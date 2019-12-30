/*
 * Two properties of playbin allow selecting the audio & video sinks: audio-sink and video-sink. The application needs
 * to instantiate the proper GstElement and pass it to playbin through these properties.
 *
 * For a more complex pipeline, the pipeline with shink should be wrapped in a Bin. GstBin is a container encapsulating
 * partial pipelines so they can be manabed as a single element. Elements in a Bin connect to external elements through
 * GstGhostPad. GstGhostPad is a Pad on the surface of the Bin forwarding data from an external Pad to a given Pad in
 * the Bin.
 */

#include <gst/gst.h>

int main(int argc, char *argv[])
{
  GstElement *pipeline, *bin, *equalizer, *convert, *sink;
  GstPad *pad, *ghost_pad;
  GstBus *bus;
  GstMessage *msg;

  gst_init(&argc, &argv);

  pipeline = gst_parse_launch(
                  "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
                  NULL);
  equalizer = gst_element_factory_make("equalizer-3bands", "equalizer");
  convert = gst_element_factory_make("audioconvert", "convert");
  sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  if(!pipeline || !equalizer || !convert || !sink) {
    g_printerr("Not all elements could be created. \n");
    return -1;
  }

  bin = gst_bin_new("audio_sink_bin");
  gst_bin_add_many(GST_BIN(bin), equalizer, convert, sink, NULL);
  gst_element_link_many(equalizer, convert, sink, NULL);

  pad = gst_element_get_static_pad(equalizer, "sink");
  ghost_pad = gst_ghost_pad_new("sink", pad);
  gst_pad_set_active(ghost_pad, TRUE);
  gst_element_add_pad(bin, ghost_pad);

  gst_object_unref(pad);

  g_object_set(G_OBJECT(equalizer), "band1", (gdouble)-24.0, NULL);
  g_object_set(G_OBJECT(equalizer), "band2", (gdouble)-24.0, NULL);
  g_object_set(G_OBJECT(pipeline), "audio-sink", bin, NULL);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  if(msg != NULL) gst_message_unref(msg);

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
