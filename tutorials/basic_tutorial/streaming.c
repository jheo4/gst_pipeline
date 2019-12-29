/*
 * Streaming: playing media straight from the internet without storing it locally (URIs starting with http://)
 * Buffering in GStreamer: queues building the buffer and bus messages about the buffer level (the state of the queue)
 * Sync among multiple sinks: all the sinks must be synchronized by using a global clock. when the clock is lost, the
 *                            application selects a new one and set the pipeline to PAUSED and thne to PLAYING again.
 *
 * Preroll: a sink element only complete the state change to PAUSED after a buffer has been queued on the input pad(s).
 *          prerolling is needed to fill the pipeline with buffers so that the transition to PLAYING goes AFAP. preroll
 *          is crucial in maintaining sync among streams and ensuring that no buffers are dropped in the sinks.
 *          at GST_STATE_PAUSED, the pipeline is prerolled and ready to render data immediately.
 */

#include <gst/gst.h>
#include <string.h>

typedef struct _CustomData_t
{
  gboolean is_live;
  GstElement *pipeline;
  GMainLoop *loop;
} CustomData_t;


static void cb_msg(GstBus *bus, GstMessage *msg, CustomData_t *data)
{
  switch(GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debug;

      gst_message_parse_error(msg, &err, &debug);
      g_print("Error: %s \n", err->message);
      g_error_free(err);
      g_free(debug);

      gst_element_set_state(data->pipeline, GST_STATE_READY);
      g_main_loop_quit(data->loop);
      break;
    }
    case GST_MESSAGE_EOS:
      gst_element_set_state(data->pipeline, GST_STATE_READY);
      g_main_loop_quit(data->loop);
      break;
    case GST_MESSAGE_BUFFERING: {
      gint percent = 0;
      if(data->is_live) break;

      gst_message_parse_buffering(msg, &percent);
      g_print("Buffering (%3d%%) \r", percent);
      if(percent < 100)
        gst_element_set_state(data->pipeline, GST_STATE_PAUSED); // prerolled
      else
        gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
      break;
    }
    case GST_MESSAGE_CLOCK_LOST:
      gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
      gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
      break;
    default:
      break;
  }
}


int main(int argc, char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstStateChangeReturn ret;
  GMainLoop *main_loop;
  CustomData_t data;

  gst_init(&argc, &argv);
  memset(&data, 0, sizeof(data));

  // build the pipeline
  pipeline = gst_parse_launch(
                "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
                NULL);
  bus = gst_element_get_bus(pipeline);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state. \n");
    gst_object_unref(pipeline);
  }
  else if(ret == GST_STATE_CHANGE_NO_PREROLL) { // for the live streming, GST_STATE_CHANGE_NO_PREROLL is returned
                                                // instead of GST_STATE_CHANGE_SUCCESS.
    data.is_live = TRUE;
  }

  main_loop = g_main_loop_new(NULL, FALSE);
  data.loop = main_loop;
  data.pipeline = pipeline;

  gst_bus_add_signal_watch(bus);
  g_signal_connect(bus, "message", G_CALLBACK(cb_msg), &data);

  g_main_loop_run(main_loop);

  g_main_loop_unref(main_loop);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
}
