#include <server/pipeline.hpp>

/* UserBin & SinkBin Wrapper */
UsrBin_t* make_usrbin(int id, string codec, string width, string height)
{
  /*  UserBin
   *    b[g_sink - queue - scaler - capsfilter - encoder - g_src] - tee
   */
  UsrBin_t* usrbin = g_new0(UsrBin_t, 1);
  usrbin->bin = gst_bin_new(NULL);
  usrbin->id = id;

  GstElement *queue, *scaler, *caps_filter, *encoder, *tee;
  string caps_str = "video/x-raw,width=" + width + ",height=" + height;
  GstCaps *scale_caps = gst_caps_from_string(caps_str.c_str());

  /* GstBin */
  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&scaler, "videoscale", NULL);
  gst_element_factory_make_wrapper(&caps_filter, "capsfilter", NULL);
  gst_element_factory_make_wrapper(&encoder, codec.c_str(), NULL);
  g_object_set(G_OBJECT(caps_filter), "caps", scale_caps, NULL);
  gst_caps_unref(scale_caps);

  gst_bin_add_many(GST_BIN(usrbin->bin), queue, scaler, caps_filter,
                   encoder, NULL);
  gst_element_link_many(queue, scaler, caps_filter, encoder, NULL);
  setup_ghost_sink(queue, GST_BIN(usrbin->bin));
  setup_ghost_src(encoder, GST_BIN(usrbin->bin));

  /* tee */
  usrbin->tee = tee;

  return usrbin;
}


SinkBin_t* make_sinkbin(int id, string codec, string recv_addr, int port_base)
{
  SinkBin_t* sinkbin = g_new0(SinkBin_t, 1);
  sinkbin->rtp_bin = gst_bin_new(NULL);
  sinkbin->rtcp_bin = gst_bin_new(NULL);
  sinkbin->v_caps = gst_caps_new_simple("video/x-raw",
                                        "framerate", GST_TYPE_FRACTION, 15, 1,
                                        NULL);
  sinkbin->id = id;

  GstElement *rtp_bin, *queue, *payloader, *rtp_sink;
  GstElement *rtcp_src, *rtcp_sink;

  /* RTP elements */
  gst_element_factory_make_wrapper(&rtp_bin, "rtpbin", NULL);
  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&payloader, codec.c_str(), NULL);
  gst_element_factory_make_wrapper(&rtp_sink, "udpsink", NULL);

  g_object_set(rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  g_signal_connect(rtp_bin, "request_aux_sender",
                   (GCallback)request_aux_sender, id);
  g_object_set(rtp_sink, "port", port_base, "host", recv_addr.c_str(), NULL);

  gst_bin_add_many(GST_BIN(sinkbin->rtp_bin), queue, rtp_bin, payloader, rtp_sink, NULL);

  gchar *pad_name = g_strdup_printf("send_rtp_sink_%u", id);
  gst_element_link_pads()

  /* RTCP elements */
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);

  g_object_set(rtcp_src, "address", recv_addr.c_str(),
               "port", port_base+1, NULL);
  g_object_set(rtcp_sink, "port", port_base+2,
               "host", recv_addr.c_str(), NULL);



  return sinkbin;
}


/*  Private  */
bool Pipeline::link_tee2pad(GstElement *tee, GstElement *bin)
{
  GstPad *tee_pad, *bin_pad;
  tee_pad = gst_element_get_request_pad(tee, "src_u");
  bin_pad = gst_element_get_static_pad(bin, "sink");
  if(gst_pad_link(tee_pad, bin_pad) != GST_PAD_LINK_OK) {
    ERROR("Tee pad link error");
    return FALSE;
  }
  return TRUE;
}


bool Pipeline::set_pipeline_ready()
{
  gst_element_set_state(pipeline, GST_STATE_READY);
  g_main_loop_quit(loop);
  return TRUE;
}


bool Pipeline::set_pipeline_run()
{
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  g_main_loop_run(loop);
  return TRUE;
}


/* Public */
Pipeline::Pipeline(int _id)
{
  id = _id;
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


bool Pipeline::set_source() {
  gst_element_factory_make_wrapper(&src, "videotestsrc", NULL);
  gst_element_factory_make_wrapper(&src_tee, "tee", NULL);
  g_object_set (src, "is-live", TRUE, "horizontal-speed", 1, NULL);
  gst_bin_add_many(GST_BIN(pipeline), src, src_tee, NULL);

  if(!gst_element_link_wrapper(src, src_tee))
    return FALSE;
  else
    return TRUE;
}


bool Pipeline::register_usrbin(UsrBin_t *_usrbin)
{
  gst_bin_add_many(GST_BIN(pipeline), _usrbin->bin, _usrbin->tee, NULL);
  return TRUE;
}


bool Pipeline::register_sinkbin(SinkBin_t *_sinkbin)
{
  gst_bin_add_many(GST_BIN(pipeline), _sinkbin->rtp_bin,
                   _sinkbin->rtcp_bin, NULL);
  return TRUE;
}


bool Pipeline::connect_userbin_to_src(UsrBin_t *_usrbin)
{
  link_tee2pad(src_tee, _usrbin->bin);
  return TRUE;
}


bool Pipeline::connect_sinkbin_to_userbin(SinkBin_t *_sink_bin,
                                          UsrBin_t *_usrbin)
{
  //link_tee2pad(_usrbin->tee, _sink_bin->);
  return TRUE;
}

