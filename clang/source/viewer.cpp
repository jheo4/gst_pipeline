#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <aux.h>
#include <cb_basic.h>
#include <debug.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <viewer.hpp>


Viewer::Viewer()
{
  common_data = make_common_data();
  g_object_set(common_data->rtp_bin, "latency", 200,
               "do-retransmission", TRUE,
               "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  GstBus* bus = gst_element_get_bus(GST_ELEMENT(common_data->pipeline));
  connect_basic_signals(bus, common_data);
  gst_object_unref(bus);
}


Viewer::~Viewer()
{
  gst_element_set_state(GST_ELEMENT(common_data->pipeline), GST_STATE_NULL);
  gst_object_unref(common_data->pipeline);
  g_main_loop_unref(common_data->loop);
}


void Viewer::run()
{
  gst_element_set_state(GST_ELEMENT(common_data->pipeline), GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)common_data->pipeline,
                                  GST_DEBUG_GRAPH_SHOW_ALL, "client-pipeline");
  g_main_loop_run(common_data->loop);
}


StreamSession_t* Viewer::make_video_session(guint id)
{
  GstBin* bin = GST_BIN(gst_bin_new(NULL));
  GstElement *queue, *video_convert, *h264_decoder, *h264_depayloader, *test_sink;
  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&h264_depayloader, "rtph264depay", NULL);
  gst_element_factory_make_wrapper(&h264_decoder, "avdec_h264", NULL);
  gst_element_factory_make_wrapper(&video_convert, "videoconvert", NULL);
  gst_element_factory_make_wrapper(&test_sink, "autovideosink", NULL);

  gst_bin_add_many(bin, queue, h264_depayloader, h264_decoder, video_convert,
                   test_sink, NULL);
  gst_element_link_wrapper(queue, h264_depayloader);
  gst_element_link_wrapper(h264_depayloader, h264_decoder);
  gst_element_link_wrapper(h264_decoder, video_convert);
  gst_element_link_wrapper(video_convert, test_sink);

  setup_ghost_sink(queue, bin);

  StreamSession_t* session = create_stream_session(id);
  session->bin = GST_ELEMENT(bin);
  session->v_caps = gst_caps_new_simple("application/x-rtp",
                                        "media", G_TYPE_STRING, "video",
                                        "clock-rate", G_TYPE_INT, 90000,
                                        "encoding-name", G_TYPE_STRING, "H264",
                                        NULL);
  return session;
}


void Viewer::setup_rtp_receiver_with_stream_session(StreamSession_t* session,
                                  const char* addr, int port_base)
{
  session->rtp_bin = g_object_ref(common_data->rtp_bin);

  GstElement *rtp_src, *rtcp_src, *rtcp_sink;
  gst_element_factory_make_wrapper(&rtp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);

  DEBUG("setup rtp elements");
  g_object_set(rtp_src, "address", addr, "port", port_base,
               "caps", session->v_caps, NULL);
  g_object_set(rtcp_src, "address", addr, "port", port_base+1, NULL);
  g_object_set(rtcp_sink, "port", port_base+2,
               "host", addr, "sync", FALSE, "async", FALSE, NULL);
  DEBUG("RTP src stream (%i) / RTCP sink stream (%i) / RTCP src stream (%i)",
        port_base, port_base+2, port_base+1);

  DEBUG("connect rtp signals to cb handlers");
  gst_bin_add_many(GST_BIN(common_data->pipeline),
                           rtp_src, rtcp_src, rtcp_sink, NULL);

  g_signal_connect(common_data->rtp_bin, "request-aux-receiver",
                   (GCallback)request_aux_receiver, session);

  g_signal_connect_data(common_data->rtp_bin, "pad-added",
                        G_CALLBACK(rtp_pad_added_handler),
                        stream_session_ref(session),
                        (GClosureNotify)stream_session_unref, 0);

  g_signal_connect_data(common_data->rtp_bin, "request-pt-map",
                        G_CALLBACK(request_pt_map),
                        stream_session_ref(session),
                        (GClosureNotify)stream_session_unref, 0);

  DEBUG("link rtp pads between udpsink/src and rtp bin");
  gchar* pad_name;
  pad_name = g_strdup_printf("recv_rtp_sink_%u", session->id);
  gst_element_link_pads(rtp_src, "src", common_data->rtp_bin, pad_name);
  free(pad_name);

  pad_name = g_strdup_printf("recv_rtcp_sink_%u", session->id);
  gst_element_link_pads(rtcp_src, "src", common_data->rtp_bin, pad_name);
  free(pad_name);

  pad_name = g_strdup_printf("send_rtcp_src_%u", session->id);
  gst_element_link_pads(common_data->rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  stream_session_unref(session);
}

