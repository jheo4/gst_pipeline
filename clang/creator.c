#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst_wrapper.h>
#include <gst_common.h>
#include <creator.h>
#include <cb_basic.h>
#include <aux.h>
#include <ghost.h>
#include <debug.h>


// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! videoconvert !
//                x264enc ! avdec_h264 ! autovideosink
// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! audioconvert !
//                avenc_ac3 ! avdec_ac3 ! autoaudiosink
int main(int argc, char **argv)
{
  DEBUG("Start");
  gst_init(&argc, &argv);

  CreatorData_t creator_data;
  init_creator_data(&creator_data);

  GstBus *bus = gst_element_get_bus(
                              GST_ELEMENT(creator_data.common_data->pipeline));
  connect_basic_signals(bus, creator_data.common_data);
  g_object_unref(bus);

  DEBUG("create sessions");
  creator_data.v_session = make_video_session(0, &creator_data);

  DEBUG("setup rtp with session");
  setup_rtp_transmission_with_stream_session(creator_data.common_data,
                                             creator_data.v_session);

  //DEBUG("add pad-added handler");
  //g_signal_connect(creator_data.decodebin, "pad-added",
  //                 G_CALLBACK(file_pad_added_handler), &creator_data);

  DEBUG("Run pipeline");
  gst_element_set_state(GST_ELEMENT(creator_data.common_data->pipeline),
                        GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)creator_data.common_data->pipeline,
                            GST_DEBUG_GRAPH_SHOW_ALL, "creator-pipeline");

  g_main_loop_run(creator_data.common_data->loop);

  gst_element_set_state(GST_ELEMENT(creator_data.common_data->pipeline),
                        GST_STATE_NULL);
  gst_object_unref(creator_data.common_data->pipeline);
  g_main_loop_unref(creator_data.common_data->loop);

  DEBUG("End");
  return 0;
}

