#include <gst/gst.h>

int main(int argc, char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  // init gstreamer: internal structures, available plugins, execute commandline options
  //                 commandline params are passed via argc & argv.
  gst_init(&argc, &argv);

  // build pipeline
  //    media travels from the source to the sink, passing through a series of
  //    intermediate elements performing tasks (pipelines).
  //  gst_parse_launch: create a pipeline from a textual representation of a pipeline
  //                    For advanced features, pipelines should be built manually.
  //  playbin: gst_parse_launch build a pipeline composed of a single element, playbin.
  //           playbin acts as a source and a sink (a whole pipeline).
  pipeline = gst_parse_launch(
                "playbin uri=http://www.ithinknext.com/mydata/board/files/F201308021823010.mp4",
                NULL);

	// start the playback by setting the pipeline state as PLAYING.
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  // retrieve the pipeline's bus
  bus = gst_element_get_bus(pipeline);

  // wait until an error or EoS is recieved.
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  if(msg != NULL) gst_message_unref(msg);

  gst_object_unref(msg);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
