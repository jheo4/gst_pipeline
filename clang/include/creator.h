#include <stdio.h>
#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.h>
#include <gst_wrapper.h>
#include <aux.h>
#include <ghost.h>

#ifndef __CREATOR__
#define __CREATOR__
#define CREATOR_PORT_BASE 3000


typedef struct _CreatorData_t
{
  GstCommonData_t *common_data;
  GstElement *file_src, *decodebin;
  GstElement *test_src;
  StreamSession_t *v_session, *a_session;
} CreatorData_t;


static gboolean init_creator_data(CreatorData_t* data)
{
  data->common_data = NULL;
  data->test_src = NULL, data->file_src = NULL, data->decodebin = NULL;
  data->v_session = NULL, data->a_session = NULL;

  data->common_data = make_common_data();

  DEBUG("make creator data & setting");
  gst_element_factory_make_wrapper(&data->test_src, "videotestsrc", NULL);
  g_object_set (data->test_src, "is-live", TRUE, "horizontal-speed", 1, NULL);
  gst_bin_add_many(GST_BIN(data->common_data->pipeline), data->test_src, NULL);

  //gst_element_factory_make_wrapper(&data->filesrc, "filesrc", "filesrc");
  //gst_element_factory_make_wrapper(&data->decodebin, "decodebin", "decodebin");
  //g_object_set(data->filesrc, "location", "/home/jin/github/4k.mp4", NULL);
  //gst_element_link_wrapper(data->filesrc, data->decodebin);

  return TRUE;
}


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
}


static StreamSession_t* make_video_session(guint session_id,
                                           CreatorData_t* data)
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

  StreamSession_t* session = create_stream_session(session_id);
  session->bin = GST_ELEMENT(bin);
  gst_bin_add(GST_BIN(data->common_data->pipeline), session->bin);
  session->v_caps = gst_caps_new_simple("video/x-raw",
                                        "framerate", GST_TYPE_FRACTION, 15, 1,
                                        NULL);

  return session;
}


static StreamSession_t* make_audio_session(guint session_id,
                                           CreatorData_t* data)
{
  GstBin* bin = GST_BIN(gst_bin_new(NULL));
  GstElement *audio_convert, *ac_decoder, *alaw_encoder, *pcma_payloader;

  gst_element_factory_make_wrapper(&audio_convert, "audioconvert", NULL);
  //avenc_ac3 alawenc
  gst_element_factory_make_wrapper(&ac_decoder, "a52dec", NULL);
  gst_element_factory_make_wrapper(&alaw_encoder, "alawenc", NULL);
  gst_element_factory_make_wrapper(&pcma_payloader, "rtppcmapay", NULL);

  gst_bin_add_many(bin, audio_convert, ac_decoder, alaw_encoder, pcma_payloader, NULL);
  gst_element_link_wrapper(audio_convert, ac_decoder);
  gst_element_link_wrapper(ac_decoder, alaw_encoder);
  gst_element_link_wrapper(alaw_encoder, pcma_payloader);

  setup_ghost_sink(audio_convert, bin);
  setup_ghost_src(pcma_payloader, bin);

  StreamSession_t* session = create_stream_session(session_id);
  session->bin = GST_ELEMENT(bin);

  return session;
}


static gboolean setup_rtp_transmission_with_stream_session(
                 GstCommonData_t* common_data, StreamSession_t* stream_session)
{
  GstElement *rtp_sink, *rtcp_sink, *rtcp_src;
  gst_element_factory_make_wrapper(&rtp_sink, "udpsink", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);

  int session_port_base = CREATOR_PORT_BASE + (stream_session->id * 3);
  g_object_set(rtp_sink, "port", session_port_base, "host", "127.0.0.1", NULL);
  g_object_set(rtcp_sink, "port", session_port_base+1, "host",
               "127.0.0.1", "sync", FALSE, "async", FALSE, NULL);
  g_object_set(rtcp_src, "port", session_port_base+2, NULL);


  gst_bin_add_many(GST_BIN(common_data->pipeline),
                   rtp_sink, rtcp_sink, rtcp_src, NULL);

  g_signal_connect(common_data->rtp_bin, "request_aux_sender",
                   (GCallback)request_aux_sender, stream_session);

  gchar* pad_name;
  pad_name = g_strdup_printf("send_rtp_sink_%u", stream_session->id);
  gst_element_link_pads(stream_session->bin, "src",
                        common_data->rtp_bin, pad_name);
  gst_element_link_wrapper(common_data->rtp_bin, rtp_sink);
  g_free(pad_name);

  pad_name = g_strdup_printf("send_rtcp_src_%u", stream_session->id);
  gst_element_link_pads(common_data->rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  pad_name = g_strdup_printf("recv_rtcp_sink_%u", stream_session->id);
  gst_element_link_pads(rtcp_src, "src", common_data->rtp_bin, pad_name);
  g_free(pad_name);

  DEBUG("RTP sink stream (%i) / RTCP sink stream (%i) / RTCP src stream (%i)",
        session_port_base, session_port_base+1, session_port_base+2);

  stream_session_unref(stream_session);
  return TRUE;
}

#endif

