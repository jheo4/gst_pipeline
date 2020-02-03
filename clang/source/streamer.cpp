#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <aux.h>
#include <cb_basic.h>
#include <debug.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <streamer.hpp>
#include <codec_pay.hpp>


Streamer::Streamer()
{
  common_data = make_common_data();
  gst_element_factory_make_wrapper(&test_src, "videotestsrc", NULL);
  g_object_set (test_src, "is-live", TRUE, "horizontal-speed", 1, NULL);
  gst_bin_add_many(GST_BIN(common_data->pipeline), test_src, NULL);

  GstBus *bus = gst_element_get_bus(GST_ELEMENT(common_data->pipeline));
  connect_basic_signals(bus, common_data);
  g_object_unref(bus);
}


Streamer::~Streamer()
{
  gst_element_set_state(GST_ELEMENT(common_data->pipeline), GST_STATE_NULL);
  gst_object_unref(common_data->pipeline);
  g_main_loop_unref(common_data->loop);
}


void Streamer::run()
{
  gst_element_set_state(GST_ELEMENT(common_data->pipeline), GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)common_data->pipeline,
                                GST_DEBUG_GRAPH_SHOW_ALL, "streamer-pipeline");
  g_main_loop_run(common_data->loop);
}


SourceBin_t* Streamer::setup_source(guint id)
{
  // setup source bin: [source]--[tee]
  GstBin* src_bin = GST_BIN(gst_bin_new(NULL));
  GstElement *test_src, *tee;

  gst_element_factory_make_wrapper(&test_src, "videotestsrc", "test_src");
  gst_element_factory_make_wrapper(&tee, "tee", "src_tee");

  g_object_set (test_src, "is-live", TRUE, "horizontal-speed", 1, NULL);

  gst_bin_add_many(src_bin, test_src, tee, NULL);
  gst_element_link_wrapper(test_src, tee);

  setup_ghost_tee_src(tee, src_bin);

  // SourceBin_t setting...
  SourceBin_t* new_src_bin = g_new0(SourceBin_t, 1);
  new_src_bin->bin = GST_ELEMENT(src_bin);
  new_src_bin->id = id;

  return new_src_bin;
}


GstBin* Streamer::setup_transcoder(string codec, string resolution)
{
  // setup transcoder bin: [queue]--[videoscale]--[capsfilter]--[encoder]--[payloader]
  GstBin* trans_bin = GST_BIN(gst_bin_new(NULL));
  GstElement *in_queue, *scaler, *caps_filter, *encoder, *payloader, *out_tee;
  GstCaps* scale_caps;

  string width = resolution.substr(0, resolution.find("x"));
  string height = \
              resolution.substr(resolution.find("x")+1, resolution.find('\0'));
  string caps_str = "video/x-raw,width=" + width + ",height=" + height;
  string pay_type = get_pay_type(codec);

  gst_element_factory_make_wrapper(&in_queue, "queue", "in_queue");
  gst_element_factory_make_wrapper(&scaler, "videoscale", "scaler");
  gst_element_factory_make_wrapper(&caps_filter, "capsfilter", "caps_filter");
  gst_element_factory_make_wrapper(&encoder, codec.c_str(), "encoder");
  gst_element_factory_make_wrapper(&payloader, pay_type.c_str(), "payloader");

  scale_caps = gst_caps_from_string(caps_str.c_str());
  g_object_set(G_OBJECT(caps_filter), "caps", scale_caps, NULL);
  gst_caps_unref(scale_caps);

  gst_bin_add_many(trans_bin, in_queue, scaler,
                   caps_filter, encoder, payloader, out_tee, NULL);

  gst_element_link_wrapper(in_queue, scaler);
  gst_element_link_wrapper(scaler, caps_filter);
  gst_element_link_wrapper(caps_filter, encoder);
  gst_element_link_wrapper(encoder, payloader);
  gst_element_link_wrapper(payloader, out_tee);

  setup_ghost_sink(in_queue, trans_bin);
  setup_ghost_tee_src(out_tee, trans_bin);

  return trans_bin;
}


RTPSession_t* Streamer::setup_rtp(guint connected_id, string recv_addr,
                                  int port_base)
{
  RTPSession_t *session = g_new0(RTPSession_t, 1);
  session->connected_id = connected_id;

  gst_element_factory_make_wrapper(&session->rtp_bin, "rtpbin", "rtp_bin");
  gst_element_factory_make_wrapper(&session->rtp_sink, "udpsink", "rtp_sink");
  gst_element_factory_make_wrapper(&session->rtcp_sink,
                                   "udpsink", "rtcp_sink");
  gst_element_factory_make_wrapper(&session->rtcp_src, "udpsrc","rtcp_src");

  // RTP reference: https://bit.ly/30bBOzg
  g_object_set(session->rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  g_object_set(session->rtp_sink, "port", port_base,
               "host", recv_addr.c_str(), NULL);
  g_object_set(session->rtcp_sink, "port", port_base+1,
               "host", recv_addr.c_str(), "sync", FALSE, "async", FALSE, NULL);
  g_object_set(session->rtcp_src, "address", recv_addr.c_str(),
               "port", port_base+2, NULL);
	g_signal_connect(session->rtp_bin, "request_aux_sender",
                   (GCallback)request_aux_sender, connected_id);

  return session;
}

/*
StreamSession_t* Streamer::make_video_session(guint id)
{
  GstBin* bin = GST_BIN(gst_bin_new(NULL));
  GstElement *video_convert, *h264_encoder, *h264_payloader;
  gst_element_factory_make_wrapper(&h264_encoder, "x264enc", NULL);
  gst_element_factory_make_wrapper(&h264_payloader, "rtph264pay", NULL);

  g_object_set(h264_encoder, "pass", 5, "quantizer", 25,
               "speed-preset", 6,  NULL);
  g_object_set (h264_payloader, "config-interval", 2, NULL);

  gst_bin_add_many(bin, h264_encoder, h264_payloader, NULL);

  gst_element_link_wrapper(h264_encoder, h264_payloader);

  setup_ghost_sink(h264_encoder, bin);
  setup_ghost_src(h264_payloader, bin);

  StreamSession_t* session = create_stream_session(id);
  session->bin = GST_ELEMENT(bin);
  gst_bin_add(GST_BIN(common_data->pipeline), session->bin);
  session->v_caps = gst_caps_new_simple("video/x-raw",
                                        "framerate", GST_TYPE_FRACTION, 15, 1,
                                        NULL);
  return session;
}


void Streamer::setup_rtp_sender_with_stream_session(StreamSession_t* session,
                                                    const char* receiver_addr,
                                                    int port_base)
{
  GstElement *rtp_sink, *rtcp_sink, *rtcp_src;
  gst_element_factory_make_wrapper(&rtp_sink, "udpsink", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);

  g_object_set(rtp_sink, "port", port_base, "host", receiver_addr, NULL);
  g_object_set(rtcp_sink, "port", port_base+1, "host", receiver_addr,
               "sync", FALSE, "async", FALSE, NULL);
  g_object_set(rtcp_src, "address", receiver_addr, "port", port_base+2, NULL);

  gst_bin_add_many(GST_BIN(common_data->pipeline),
                   rtp_sink, rtcp_sink, rtcp_src, NULL);

  g_signal_connect(common_data->rtp_bin, "request_aux_sender",
                   (GCallback)request_aux_sender, session);

  gchar* pad_name;
  pad_name = g_strdup_printf("send_rtp_sink_%u", session->id);
  gst_element_link_pads(session->bin, "src",
                        common_data->rtp_bin, pad_name);
  gst_element_link_wrapper(common_data->rtp_bin, rtp_sink);
  g_free(pad_name);

  pad_name = g_strdup_printf("send_rtcp_src_%u", session->id);
  gst_element_link_pads(common_data->rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  pad_name = g_strdup_printf("recv_rtcp_sink_%u", session->id);
  gst_element_link_pads(rtcp_src, "src", common_data->rtp_bin, pad_name);
  g_free(pad_name);

  DEBUG("RTP sink stream (%i) / RTCP sink stream (%i) / RTCP src stream (%i)",
        port_base, port_base+1, port_base+2);

  stream_session_unref(session);
}
*/
