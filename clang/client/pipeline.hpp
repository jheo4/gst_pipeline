#include <string>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <common/debug.h>
#include <common/codecs.hpp>
#include <common/cb_basic.h>
#include <common/gst_wrapper.h>

#ifndef __PIPELINE__
#define __PIPELINE__

typedef struct _UsrSink_t
{
  guint id;
  GstElement *bin;
} UsrSink_t;


typedef struct _SrcBin_t
{
  guint id;
  GstElement *rtp_bin;
  GstElement *usrsink_bin;
  GstCaps *v_caps;
} SrcBin_t;


static void rtp_pad_added_handler(GstElement *element, GstPad *new_pad,
                                  SrcBin_t *arg)
{
  gchar* new_pad_name = gst_pad_get_name(new_pad);
  gchar* target_prefix = g_strdup_printf("recv_rtp_src_%u", arg->id);

  DEBUG("New pad: %s, Target Prefix: %s* \n", new_pad_name, target_prefix);

  if(g_str_has_prefix(new_pad_name, target_prefix)) {
    GstPad* target_sink_pad = \
                          gst_element_get_static_pad(arg->usrsink_bin, "sink");
    DEBUG("Target sink pad: %s", gst_pad_get_name(target_sink_pad));
    if(gst_pad_link(new_pad, target_sink_pad) != GST_PAD_LINK_OK) {
      ERROR("new pad connection error!");
    }
    gst_object_unref(target_sink_pad);
  }

  g_free(target_prefix);
  g_free(new_pad_name);
}

static GstElement* request_aux_receiver(GstElement* rtp_bin, guint session_id)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO("Creating AUX receiver");

  bin = gst_bin_new(NULL);
  rtx = gst_element_factory_make("rtprtxreceive", NULL);
  // currently pt_map is hardcoded
  pt_map = gst_structure_new("application/x-rtp-pt-map", "8", G_TYPE_UINT, 98,
                             "96", G_TYPE_UINT, 99, NULL);
  g_object_set(rtx, "payload-type-map", pt_map, NULL);
  gst_structure_free(pt_map);

  gst_bin_add(GST_BIN(bin), rtx);

  pad = gst_element_get_static_pad(rtx, "src");
  name = g_strdup_printf("src_%u", session_id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  pad = gst_element_get_static_pad(rtx, "sink");
  name = g_strdup_printf("sink_%u", session_id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  return bin;
}

static GstCaps* request_pt_map(GstElement* source, guint id,
                               guint pt, SrcBin_t* srcbin)
{
  gchar* caps_str;
  if(id == srcbin->id) {
    caps_str = gst_caps_to_string(srcbin->v_caps);
    g_print("Returning %s \n", caps_str);
    g_free(caps_str);
    return gst_caps_ref(srcbin->v_caps);
  }

  return NULL;
}



class Pipeline
{
public:
  GstElement *pipeline;
  GMainLoop *loop;
  guint id;

  SrcBin_t srcbin;
  UsrSink_t usrsink;

  Pipeline(int _id);
  ~Pipeline();

  bool set_pipeline_ready();
  bool set_pipeline_run();

  bool set_usrsink(std::string codec);
  bool set_source_with_usrsink(std::string codec, std::string server_addr,
                               int port_base);


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

