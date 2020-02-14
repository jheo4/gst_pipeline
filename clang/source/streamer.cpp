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
