#include <server/pipeline.hpp>

/* UserBin & SinkBin Wrapper */
UsrBin_t* make_usrbin(int id, string codec, string resolution)
{
  UsrBin_t* usrbin = g_new0(UsrBin_t, 1);
  usrbin->id = id;
  return usrbin;
}


SinkBin_t* make_sinkbin(int id, string recv_addr, int port_base)
{
  SinkBin_t* sinkbin = g_new0(SinkBin_t, 1);
  sinkbin->id = id;
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
  return TRUE;
}


bool Pipeline::set_pipeline_run()
{
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

