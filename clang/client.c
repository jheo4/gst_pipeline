#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <aux.h>
#include <cb_basic.h>
#include <debug.h>
#include <ghost.h>
#include <gst_common.h>
#include <gst_wrapper.h>
#include <stream_session.h>
#include <client.h>


int main(int argc, char *argv[])
{
  // read video & audio from the rtpbins & decoding it & play it
  DEBUG("Start");
  // argv: ipaddr & port of a server
  gst_init(&argc, &argv);

  DEBUG("initialize data");
  ClientData_t client_data;
  init_client_data(&client_data);

  DEBUG("set bus");
  GstBus* bus = gst_element_get_bus(GST_ELEMENT(client_data.common_data->pipeline));
  connect_basic_signals(bus, client_data.common_data);
  gst_object_unref(bus);

  DEBUG("make audio/video sessions");
  client_data.v_session = make_video_session(0, &client_data);
  client_data.a_session = make_audio_session(1, &client_data);

  DEBUG("setup the connnection between udpsink/src and rtpbin");
  setup_rtp_delivery_with_stream_session(client_data.common_data, client_data.a_session);
  setup_rtp_delivery_with_stream_session(client_data.common_data, client_data.v_session);

  DEBUG("run pipeline");
  gst_element_set_state(GST_ELEMENT(client_data.common_data->pipeline), GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)client_data.common_data->pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "client-pipeline");

  g_main_loop_run(client_data.common_data->loop);

  DEBUG("pipeline done");
  gst_element_set_state(GST_ELEMENT(client_data.common_data->pipeline), GST_STATE_NULL);

  gst_object_unref(client_data.common_data->pipeline);
  g_main_loop_unref(client_data.common_data->loop);

  DEBUG("End");
  return 0;
}

