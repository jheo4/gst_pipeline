#include <string>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <common/debug.h>
#include <common/codecs.hpp>
#include <common/cb_basic.h>
#include <common/gst_wrapper.h>

#ifndef __PIPELINE__
#define __PIPELINE__

typedef struct _UsrBin_t
{
  guint id;
  GstElement *bin, *tee;
} UsrBin_t;


typedef struct _SinkBin_t
{
  guint id;
  GstElement *rtp_bin;
} SinkBin_t;


UsrBin_t* make_usrbin(int id, std::string codec, std::string width, std::string height);
SinkBin_t* make_sinkbin(int id, std::string codec, std::string recv_addr, int port_base);
static GstElement* request_aux_sender(GstElement* rtp_bin, guint id)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO("Creating AUX sender");

  bin = gst_bin_new(NULL);
  // rtprtxsend
  //   - keeps a history of RTP pakcets up to a configurable limit
  //   - listen for upstream custom re-transmission events from downstream
  //   - if retransmission events occur, look up the requested seqnum from the history & resend it as auxiliary stream
  rtx = gst_element_factory_make("rtprtxsend", NULL);

  pt_map = gst_structure_new("application/x-rtp-pt-map", "8", G_TYPE_UINT, 98,
                             "96", G_TYPE_UINT, 99, NULL);
  // https://en.wikipedia.org/wiki/RTP_payload_formats
  g_object_set(rtx, "payload-type-map", pt_map, NULL);
  gst_structure_free(pt_map);

  gst_bin_add(GST_BIN(bin), rtx);

  pad = gst_element_get_static_pad(rtx, "src");
  name = g_strdup_printf("src_%u", id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  pad = gst_element_get_static_pad(rtx, "sink");
  name = g_strdup_printf("sink_%u", id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  return bin;
}



class Pipeline
{
private:
  bool link_tee2pad(GstElement *tee, GstElement *bin);

public:
  GstElement *pipeline;
  GMainLoop *loop;
  guint id;
  GstElement *src, *src_tee;

  Pipeline(int _id = 0);
  ~Pipeline();

  bool set_source();
  bool set_pipeline_ready();
  bool set_pipeline_run();

  bool register_usrbin(UsrBin_t *_usrbin);
  bool register_sinkbin(SinkBin_t *_sinkbin);
  /* TODO
   * bool unregister_userbin(UsrBin_t *_usrbin);
   * bool unregister_sinkbin(SinkBin_t *_sinkbin);
   */
  bool connect_userbin_to_src(UsrBin_t *_usrbin);
  bool connect_sinkbin_to_userbin(SinkBin_t *_sink_bin, UsrBin_t *_usrbin);
  /* TODO
   * bool disconnect_userbin_to_src(UsrBin_t *_usrbin);
   * bool disconnect_sinkbin_to_userbin(SinkBin_t *_sink_bin, UsrBin_t *_usrbin);
   */
  void export_diagram() {
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                              "server-pipeline");
  }
};

#endif


/*
static void file_pad_added_handler(GstElement* src, GstPad* new_pad,
                                   CreatorData_t* data)
{
  GstCaps* new_pad_caps = gst_pad_get_current_caps(new_pad);
  GstStructure* new_caps_struct = gst_caps_get_structure(new_pad_caps, 0);
  const gchar* new_caps_type = gst_structure_get_name(new_caps_struct);

  GstPadLinkReturn ret;
  GstPad *audio_sink_pad = NULL, *video_sink_pad = NULL;

  if(g_str_has_prefix(new_caps_type, "audio/x-raw")) {
    if(!data->a_session->bin) {
      g_print("The audio session bin is not ready. \n");
      goto exit;
    }

    audio_sink_pad = gst_element_get_static_pad(data->a_session->bin, "sink");

    if(gst_pad_is_linked(audio_sink_pad)) {
      g_print("The audio convert sink is already linked. \n");
      goto exit;
    }

    ret = gst_pad_link(new_pad, audio_sink_pad);
  }
  else if(g_str_has_prefix(new_caps_type, "video/x-raw")) {
    if(!data->v_session->bin) {
      g_print("The video Session bin is not ready. \n");
      goto exit;
    }

    video_sink_pad = gst_element_get_static_pad(data->v_session->bin, "sink");

    if(gst_pad_is_linked(video_sink_pad)) {
      g_print("The video convert sink is already linked. \n");
      goto exit;
    }

    ret = gst_pad_link(new_pad, video_sink_pad);
  }

  if(GST_PAD_LINK_FAILED(ret))
    g_print("Type %s: link failed \n", new_caps_type);
  else g_print("Type %s: link success \n", new_caps_type);

  exit:
  if(new_pad_caps != NULL) gst_caps_unref(new_pad_caps);
  if(audio_sink_pad != NULL) gst_object_unref(audio_sink_pad);
  if(video_sink_pad != NULL) gst_object_unref(video_sink_pad);
}*/

