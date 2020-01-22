#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.h>

#ifndef __SERVER__
#define __SERVER__

typedef struct _ServerData_t
{
  GstCommonData_t common;
  GstElement *filesrc, *decodebin,
             *video_encoder, *video_convert, *video_sink,
             *audio_convert, *audio_sink;
} CreatorData_t;


static void pad_added_handler(GstElement*, GstPad*, CreatorData_t*);
static StreamSession_t* make_video_stream_session(guint);
static StreamSession_t* make_audio_stream_session(guint);


static void pad_added_handler(GstElement* src, GstPad* new_pad, CreatorData_t* data)
{
  GstCaps* new_pad_caps = gst_pad_get_current_caps(new_pad);
  GstStructure* new_caps_struct = gst_caps_get_structure(new_pad_caps, 0);
  const gchar* new_caps_type = gst_structure_get_name(new_caps_struct);

  GstPadLinkReturn ret;
  GstPad *audio_sink_pad = NULL, *video_sink_pad = NULL;

  if(g_str_has_prefix(new_caps_type, "audio/x-raw")) {
    audio_sink_pad = gst_element_get_static_pad(data->audio_convert, "sink");
    if(gst_pad_is_linked(audio_sink_pad)) {
      g_print("The audio convert sink is already linked. \n");
      goto exit;
    }

    ret = gst_pad_link(new_pad, audio_sink_pad);
  }

  if(g_str_has_prefix(new_caps_type, "video/x-raw")) {
    video_sink_pad = gst_element_get_static_pad(data->video_convert, "sink");
    if(gst_pad_is_linked(video_sink_pad)) {
      g_print("The video convert sink is already linked. \n");
      goto exit;
    }

    ret = gst_pad_link(new_pad, video_sink_pad);
  }

  if(GST_PAD_LINK_FAILED(ret)) g_print("Type %s: link failed \n", new_caps_type);
  else g_print("Type %s: link success \n", new_caps_type);

  exit:
  if(new_pad_caps != NULL) gst_caps_unref(new_pad_caps);
  if(audio_sink_pad != NULL) gst_object_unref(audio_sink_pad);
  if(video_sink_pad != NULL) gst_object_unref(video_sink_pad);
}

#endif

