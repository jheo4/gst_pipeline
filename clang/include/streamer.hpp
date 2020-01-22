#include <stdio.h>
#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <aux.h>

#ifndef __STREAMER__
#define __STREAMER__


class Streamer : GstCommon
{
  public:
    // Attributes
    GstCommonData_t* common_data;
    GstElement *file_src, *decodebin;
    GstElement* test_src;
    StreamSession_t* v_session;

    // Methods
    Streamer();
    ~Streamer();
    void setup_rtp_sender_with_stream_session(StreamSession_t* session,
                                              const char* addr,
                                              int port_base);
    StreamSession_t* make_video_session(guint id);
    void run();
};


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

#endif

