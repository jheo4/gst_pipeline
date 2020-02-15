#include <string>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <common/debug.h>
#include <common/aux.h>
#include <common/codecs.hpp>
#include <common/cb_basic.h>
#include <common/gst_wrapper.h>

#ifndef __PIPELINE__
#define __PIPELINE__

typedef struct _UsrBin_t
{
  guint id;
  GstElement *bin;
} UsrBin_t;


typedef struct _SrcBin_t
{
  guint id;
  GstElement *rtp_bin;
} SrcBin_t;


UsrBin_t* make_usrbin(int id, std::string codec, std::string width, std::string height);
SrcBin_t* make_srcbin(int id, std::string codec, std::string server_addr, int port_base);


class Pipeline
{
public:
  GstElement *pipeline;
  GMainLoop *loop;
  guint id;
  GstElement *converter, *sink;

  Pipeline(int _id);
  ~Pipeline();

  bool set_sink();
  bool set_pipeline_ready();
  bool set_pipeline_run();

  bool register_usrbin(UsrBin_t *_usrbin);
  bool register_srcbin(SrcBin_t *_srcbin);
  /* TODO
   * bool unregister_userbin(UsrBin_t *_usrbin);
   * bool unregister_sinkbin(SrcBin_t *_srcbin);
   */
  bool connect_userbin_to_sink(UsrBin_t *_usrbin);
  bool connect_srcbin_to_userbin(SrcBin_t *_sink_bin, UsrBin_t *_usrbin);
  /* TODO
   * bool disconnect_userbin_to_src(UsrBin_t *_usrbin);
   * bool disconnect_sinkbin_to_userbin(SinkBin_t *_sink_bin, UsrBin_t *_usrbin);
   */
  void export_diagram() {
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                              "client-pipeline");
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

