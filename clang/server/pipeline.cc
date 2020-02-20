#include <server/pipeline.hpp>

using namespace std;

/* UserBin & SinkBin Wrapper */
UsrBin_t* make_usrbin(int id, string codec, string width, string height)
{
  /*  UserBin
   *    b[ g_sink - queue - scaler - capsfilter - encoder - g_src ] - tee
   */
  UsrBin_t* usrbin = g_new0(UsrBin_t, 1);
  usrbin->bin = gst_bin_new(NULL);
  usrbin->id = id;

  GstElement *queue, *scaler, *caps_filter, *encoder;
  GstCaps *v_caps = gst_caps_new_simple("video/x-raw",
                                        "width", G_TYPE_INT, stoi(width),
                                        "height", G_TYPE_INT, stoi(height),
                                        "framerate", GST_TYPE_FRACTION, 60, 1,
                                        NULL);

  /* GstBin */
  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&scaler, "videoscale", NULL);
  gst_element_factory_make_wrapper(&caps_filter, "capsfilter", NULL);
  gst_element_factory_make_wrapper(&encoder, codec.c_str(), NULL);
  g_object_set(G_OBJECT(caps_filter), "caps", v_caps, NULL);
  gst_caps_unref(v_caps);

  gst_bin_add_many(GST_BIN(usrbin->bin), queue, scaler, caps_filter,
                   encoder, NULL);
  gst_element_link_many(queue, scaler, caps_filter, encoder, NULL);
  setup_ghost_sink(queue, GST_BIN(usrbin->bin));
  setup_ghost_src(encoder, GST_BIN(usrbin->bin));

  /* tee */
  gst_element_factory_make_wrapper(&usrbin->tee, "tee", NULL);

  return usrbin;
}


SinkBin_t* make_sinkbin(int id, string codec, string recv_addr, int port_base)
{
  /* SinkBin
   *   b[ g_sink - queue - payloader - rtp_bin - rtp_sink ]
   *    [                   rtcp_src /         \ rtcp_sink]
   */
  SinkBin_t* sinkbin = g_new0(SinkBin_t, 1);
  sinkbin->rtp_bin = gst_bin_new(NULL);
  sinkbin->id = id;
  string pay_type = get_pay_type(codec);

  GstElement *queue, *payloader, *rtp_bin, *rtp_sink;
  GstElement *rtcp_src, *rtcp_sink;

  /* Make RTP Elements */
  gst_element_factory_make_wrapper(&queue, "queue", NULL);
  gst_element_factory_make_wrapper(&payloader, pay_type.c_str(), NULL);
  gst_element_factory_make_wrapper(&rtp_bin, "rtpbin", NULL);


  string sink_name = "rtp_sink_" + to_string(id);
  gst_element_factory_make_wrapper(&rtp_sink, "udpsink", sink_name.c_str());

  gst_bin_add_many(GST_BIN(sinkbin->rtp_bin), queue, payloader,
                   rtp_bin, rtp_sink, NULL);

  /* Set RTP elements */
  g_object_set(rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  g_signal_connect(rtp_bin, "request_aux_sender", (GCallback)request_aux_sender, id);
  g_object_set(rtp_sink, "port", port_base, "host", recv_addr.c_str(), NULL);

  /* Link RTP elements
   *   queue - payloader - rtp_bin - rtp_sink
   */
  gst_element_link_wrapper(queue, payloader);
  gchar *rtp_pad_name = g_strdup_printf("send_rtp_sink_%u", id);
  gst_element_link_pads(payloader, "src", rtp_bin, rtp_pad_name);
  gst_element_link_wrapper(rtp_bin, rtp_sink);
  g_free (rtp_pad_name);


  /* Make RTCP Elements */
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);
  gst_bin_add_many(GST_BIN(sinkbin->rtp_bin), rtcp_src, rtcp_sink, NULL);

  /* Set RTCP Elements */
  g_object_set(rtcp_src, "address", recv_addr.c_str(),
               "port", port_base+1, NULL);
  g_object_set(rtcp_sink, "port", port_base+2,
               "host", recv_addr.c_str(), NULL);

  /* Link RTCP elements
   *   rtcp_src - rtp_bin - rtcp_sink
   */
  gchar *rtcp_pad_name = g_strdup_printf ("recv_rtcp_sink_%u", id);
  gst_element_link_pads (rtcp_src, "src", rtp_bin, rtcp_pad_name);
  rtcp_pad_name = g_strdup_printf ("send_rtcp_src_%u", id);
  gst_element_link_pads (rtp_bin, rtcp_pad_name, rtcp_sink, "sink");
  g_free (rtcp_pad_name);

  /* Set SinkBin GhostPads */
  setup_ghost_sink(queue, GST_BIN(sinkbin->rtp_bin));

  return sinkbin;
}


/*  Private  */
bool Pipeline::link_tee2pad(GstElement *tee, GstElement *bin)
{
  GstPad *tee_pad, *bin_pad;
  tee_pad = gst_element_get_request_pad(tee, "src_%u");
  bin_pad = gst_element_get_static_pad(bin, "sink");
  if(gst_pad_link(tee_pad, bin_pad) != GST_PAD_LINK_OK) {
    ERROR("Tee pad link error");
    return FALSE;
  }
  return TRUE;
}


bool Pipeline::set_pipeline_ready()
{
  DEBUG("ready");
  gst_element_set_state(pipeline, GST_STATE_READY);
  return TRUE;
}


bool Pipeline::set_pipeline_run()
{
  DEBUG("run");
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
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

  gst_element_link_wrapper(src, src_tee);
  return TRUE;
}


bool Pipeline::register_usrbin(UsrBin_t *_usrbin)
{
  gst_bin_add_many(GST_BIN(pipeline), _usrbin->bin, _usrbin->tee, NULL);
  gst_element_link_wrapper(_usrbin->bin, _usrbin->tee);
  return TRUE;
}


bool Pipeline::register_sinkbin(SinkBin_t *_sinkbin)
{
  gst_bin_add_many(GST_BIN(pipeline), _sinkbin->rtp_bin, NULL);
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
  link_tee2pad(_usrbin->tee, _sink_bin->rtp_bin);
  return TRUE;
}

