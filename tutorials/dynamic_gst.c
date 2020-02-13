#include <gst/gst.h>
#include <unistd.h>

void setup_ghost_src(GstElement *src, GstElement *bin)
{
  GstPad* src_pad = gst_element_get_static_pad(src, "src");
  GstPad* bin_pad = gst_ghost_pad_new("src", src_pad);
  gst_element_add_pad(bin, bin_pad);
  gst_object_unref (src_pad);
}


void setup_ghost_tee_src(GstElement* tee, GstElement* bin)
{
  GstPad* tee_pad = gst_element_get_request_pad(tee, "src_%u");
  GstPad* bin_pad = gst_ghost_pad_new("tee_src", tee_pad);
  gst_pad_set_active (bin_pad, TRUE);
  gst_element_add_pad(bin, bin_pad);
  gst_object_unref (tee_pad);
}


void setup_ghost_sink(GstElement *sink, GstElement *bin)
{
  GstPad* sink_pad = gst_element_get_static_pad(sink, "sink");
  GstPad* bin_pad = gst_ghost_pad_new("sink", sink_pad);
  gst_element_add_pad(bin, bin_pad);
  gst_object_unref (sink_pad);
}


int main(int argc, char *argv[]) {
  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  // base pipeline
  GstElement *pipeline, *test_src, *src_tee, *user_queue, *video_convert,
             *user_tee, *sink_queue, *video_sink;
  GstElement *user_bin;

  GstBus *bus;
  GstMessage *msg;
  GstPad *source_tee_pad, *user_tee_pad, *sink_queue_pad;
  GstPad *user_bin_sink_pad;

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new ("test-pipeline");

  /* Create the elements */
  test_src = gst_element_factory_make("videotestsrc", NULL);
  src_tee = gst_element_factory_make ("tee", NULL);

  user_queue = gst_element_factory_make ("queue", "video_queue");
  video_convert = gst_element_factory_make ("videoconvert", NULL);
  user_tee = gst_element_factory_make("tee", NULL);

  sink_queue =  gst_element_factory_make ("queue", "video_queue");
  video_sink = gst_element_factory_make ("autovideosink", "video_sink");

  /* Set bins */
  user_bin = gst_bin_new(NULL);
  gst_bin_add_many(GST_BIN(user_bin), user_queue, video_convert, NULL);
  gst_element_link_many(user_queue, video_convert, NULL);
  setup_ghost_sink(user_queue, user_bin);
  setup_ghost_src(video_convert, user_bin);

  /* Add all the things the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), test_src, src_tee, user_bin, user_tee, sink_queue, video_sink, NULL);
  gst_element_link_many(test_src, src_tee, NULL);
  gst_element_link_many(sink_queue, video_sink, NULL);

  /* Manually link the Tee, which has "Request" pads */
  source_tee_pad = gst_element_get_request_pad(src_tee, "src_%u");
  user_bin_sink_pad = gst_element_get_static_pad(user_bin, "sink");
  gst_element_link(user_bin, user_tee);

  user_tee_pad = gst_element_get_request_pad(user_tee, "src_%u");
  sink_queue_pad = gst_element_get_static_pad(sink_queue, "sink");

  g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(source_tee_pad));
  g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(user_tee_pad));
  g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(user_bin_sink_pad));

  if (gst_pad_link (source_tee_pad, user_bin_sink_pad) != GST_PAD_LINK_OK ||
      gst_pad_link (user_tee_pad, sink_queue_pad) != GST_PAD_LINK_OK) {
    g_printerr ("Tee could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  // gst_object_unref (queue_audio_pad);
  // gst_object_unref (queue_video_pad);
  /* Start playing the pipeline */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  sleep(5);
  GstElement *test_queue, *test_sink;
  GstPad *test_queue_pad;

  test_queue = gst_element_factory_make("queue", NULL);
  test_sink = gst_element_factory_make ("autovideosink", "test_sink");

  gst_element_set_state (pipeline, GST_STATE_READY);

  gst_bin_add_many(GST_BIN(pipeline), test_queue, test_sink, NULL);
  gst_element_link_many(test_queue, test_sink, NULL);
  test_queue_pad = gst_element_get_static_pad(test_queue, "sink");

  user_tee_pad = gst_element_get_request_pad(user_tee, "src_%u");
  gst_pad_link (user_tee_pad, test_queue_pad);

  sleep(1);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  bus = gst_element_get_bus (pipeline);
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad (user_bin, user_bin_sink_pad);
  gst_object_unref (user_bin_sink_pad);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);

  /*
    */

  // user_tee_pad -- test_queue

  /* Wait until error or EOS */
  //bus = gst_element_get_bus (pipeline);
  //msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Release the request pads from the Tee, and unref them */
  //gst_element_release_request_pad (user_bin, user_bin_src_pad);
  //gst_element_release_request_pad (user_bin, user_bin_sink_pad);
  //gst_object_unref (user_bin_sink_pad);

  /* Free resources */
  //if (msg != NULL)
  //  gst_message_unref (msg);
  //gst_object_unref (bus);
  //gst_element_set_state (pipeline, GST_STATE_NULL);

  //gst_object_unref (pipeline);
  return 0;
}

