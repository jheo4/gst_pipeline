#include <client/pipeline.hpp>

using namespace std;

/* Public */
Pipeline::Pipeline(int _id)
{
  id = _id;
  usrsink.bin = NULL;
  srcbin.rtp_bin = NULL;
  pipeline = gst_pipeline_new(NULL);
  loop = g_main_loop_new(NULL, FALSE);
  if(!pipeline || !loop) ERROR("pipeline creation error...");
  else {
    GstBus *bus = gst_element_get_bus(GST_ELEMENT(pipeline));
    connect_basic_signals(bus, pipeline, loop);
    g_object_unref(bus);
  }
}


Pipeline::~Pipeline()
{
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  g_main_loop_unref(loop);
}


bool Pipeline::set_pipeline_ready()
{
  DEBUG("ready");
  gst_element_set_state(pipeline, GST_STATE_READY);
  //g_main_loop_quit(loop);
  return TRUE;
}


bool Pipeline::set_pipeline_run()
{
  DEBUG("run");
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  //g_main_loop_run(loop);
  return TRUE;
}


bool Pipeline::set_usrsink(std::string codec)
{
  /*  UserSink
   *    b[ g_sink - queue - depayloader - decoder - g_src ] - converter - sink
   */
  usrsink.bin = gst_bin_new(NULL);
  usrsink.id = id;

  /* UsrBin Part */
  string depay_type = get_depay_type(codec);
  GstElement *queue, *depayloader, *decoder;

  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&depayloader, depay_type.c_str(), NULL);
  gst_element_factory_make_wrapper(&decoder, codec.c_str(), NULL);

  gst_bin_add_many(GST_BIN(usrsink.bin), queue, depayloader, decoder, NULL);
  gst_element_link_many(queue, depayloader, decoder, NULL);

  setup_ghost_sink(queue, GST_BIN(usrsink.bin));
  setup_ghost_src(decoder, GST_BIN(usrsink.bin));

  gst_bin_add(GST_BIN(pipeline), usrsink.bin);

  /* SinkBin Part */
  GstElement *converter, *sink;
  gst_element_factory_make_wrapper(&converter, "videoconvert", NULL);

  string sink_name = "fakesink" + to_string(id);
  gst_element_factory_make_wrapper(&sink, "fakesink", sink_name.c_str());
  g_object_set(sink, "sync", TRUE, NULL);

  //string sink_name = "videosink" + to_string(id);
  //gst_element_factory_make_wrapper(&sink, "autovideosink", sink_name.c_str());
  //g_object_set(sink, "sync", TRUE, NULL);


  gst_bin_add_many(GST_BIN(pipeline), converter, sink, NULL);

  /* Link UsrBin - SinkBin */
  gst_element_link_many(usrsink.bin, converter, sink, NULL);

  return TRUE;
}


bool Pipeline::set_source_with_usrsink(string codec, string server_addr,
                                       int port_base)
{
  if(usrsink.bin == NULL) {
    DEBUG("Plz set UsrSink first...");
    return FALSE;
  }

  /* Source
   *    rtp_src - rtp_bin - (pad-added) - b[ g_sink - usrsink_bin - g_src] - sink
   *   rtcp_src /         \ rtcp_sink
   */
  string caps_type = get_caps_type(codec);
  srcbin.id = id;
  srcbin.usrsink_bin = usrsink.bin;
  srcbin.v_caps = gst_caps_new_simple("application/x-rtp",
                            "media", G_TYPE_STRING, "video",
                            "clock-rate", G_TYPE_INT, 90000,
                            "encoding-name", G_TYPE_STRING, caps_type.c_str(),
                            NULL);

  gst_element_factory_make_wrapper(&srcbin.rtp_bin, "rtpbin", NULL);
  gst_bin_add(GST_BIN(pipeline), srcbin.rtp_bin);
  g_object_set(srcbin.rtp_bin, "latency", 16, "do-retransmission", TRUE,
               "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  g_signal_connect(srcbin.rtp_bin, "request_aux_receiver",
                   (GCallback)request_aux_receiver, id);

  g_signal_connect_data(srcbin.rtp_bin, "pad-added",
                        G_CALLBACK(rtp_pad_added_handler), &srcbin, NULL, 0);

  g_signal_connect_data(srcbin.rtp_bin, "request-pt-map",
                        G_CALLBACK(request_pt_map), &srcbin, NULL, 0);

  /* RTP & RTCP */
  GstElement *rtp_src;
  GstElement *rtcp_src, *rtcp_sink;

  /* RTP Setting */
  gst_element_factory_make_wrapper(&rtp_src, "udpsrc", NULL);
  gst_bin_add(GST_BIN(pipeline), rtp_src);

  g_object_set(rtp_src, "address", server_addr.c_str(), "port", port_base,
               "caps", srcbin.v_caps, NULL);

  gchar* rtp_pad_name = g_strdup_printf("recv_rtp_sink_%u", id);
  gst_element_link_pads(rtp_src, "src", srcbin.rtp_bin, rtp_pad_name);
  free(rtp_pad_name);

  /* RTCP Setting */
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);
  gst_bin_add_many(GST_BIN(pipeline), rtcp_src, rtcp_sink, NULL);

  g_object_set(rtcp_src, "address", server_addr.c_str(), "port", port_base+2,
               NULL);
  g_object_set(rtcp_sink, "port", port_base+1, "host", server_addr.c_str(),
               "sync", FALSE, "async", FALSE, NULL);

  gchar *rtcp_pad_name = g_strdup_printf ("recv_rtcp_sink_%u", id);
  gst_element_link_pads (rtcp_src, "src", srcbin.rtp_bin, rtcp_pad_name);
  rtcp_pad_name = g_strdup_printf ("send_rtcp_src_%u", id);
  gst_element_link_pads (srcbin.rtp_bin, rtcp_pad_name, rtcp_sink, "sink");
  g_free (rtcp_pad_name);

  return TRUE;
}

