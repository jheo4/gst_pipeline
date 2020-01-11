#include <stdio.h>
#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.h>
#include <gst_wrapper.h>
#include <aux.h>
#include <ghost.h>

#ifndef __CLIENT__
#define __CLIENT__


typedef struct _ClientData_t
{
  GstCommonData_t* common_data;
  GstElement *video_sink, *audio_sink;
  StreamSession_t *v_session, *a_session;
} ClientData_t;


static gboolean init_client_data(ClientData_t* data)
{
  DEBUG("Start");
  data->common_data = NULL;

  DEBUG("set common data");
  data->common_data = make_common_data();

  DEBUG("make creator data & setting");
  gst_element_factory_make_wrapper(&data->audio_sink, "autoaudiosink", "autoaudiosink");
  gst_element_factory_make_wrapper(&data->video_sink, "autovideosink", "autovideosink");

  gst_bin_add_many(GST_BIN(data->common_data->pipeline), data->audio_sink, data->video_sink, NULL);

  DEBUG("End");
  return TRUE;
}


static void rtp_pad_added_handler(GstElement* src, GstPad* new_pad, StreamSession_t* session)
{
  DEBUG("Start");
  gchar* new_pad_name = gst_pad_get_name(new_pad);
  gchar* target_prefix = g_strdup_printf("recv_rtp_src_%u", session->id);

  DEBUG("New pad: %s, Target Prefix: %s* \n", new_pad_name, target_prefix);

  if(g_str_has_prefix(new_pad_name, target_prefix)) {
    DEBUG("new pad has target prefix");

    GstElement* parent = GST_ELEMENT(gst_element_get_parent(session->rtp_bin));
    DEBUG("rtp_bin parent name: %s", gst_element_get_name(parent));

    if(parent) {
      DEBUG("connet session bin and rtp bin & sync states between them");
      gst_bin_add(GST_BIN(parent), session->bin);
      gst_element_sync_state_with_parent(session->bin);
      gst_object_unref(parent);

      GstPad *target_sink_pad = gst_element_get_static_pad(session->bin, "sink");
      g_assert_cmpint(gst_pad_link(new_pad, target_sink_pad), ==, GST_PAD_LINK_OK);
      gst_object_unref(target_sink_pad);
    }
  }

  g_free(target_prefix);
  g_free(new_pad_name);
  DEBUG("Ends");
}


static StreamSession_t* make_video_session(guint session_id, ClientData_t *data)
{
  GstBin* bin = GST_BIN(gst_bin_new(NULL));
  GstElement *video_convert, *h264_decoder, *h264_depayloader;

  gst_element_factory_make_wrapper(&h264_depayloader, "rtph264depay", "rtph264depay");
  gst_element_factory_make_wrapper(&h264_decoder, "avdec_h264", "avdec_h264");
  gst_element_factory_make_wrapper(&video_convert, "videoconvert", "videoconvert");

  gst_bin_add_many(bin, h264_depayloader, h264_decoder, video_convert, NULL);
  gst_element_link_wrapper(h264_depayloader, h264_decoder);
  gst_element_link_wrapper(h264_decoder, video_convert);

  setup_ghost_sink(h264_depayloader, bin);
  setup_ghost_src(video_convert, bin);

  StreamSession_t* session = create_stream_session(session_id);
  session->bin = GST_ELEMENT(bin);

  return session;
}


static StreamSession_t* make_audio_session(guint session_id, ClientData_t *data)
{
  GstBin* bin = GST_BIN(gst_bin_new(NULL));
  GstElement *audio_convert, *a52_decoder, *ac3_depayloader;

  gst_element_factory_make_wrapper(&ac3_depayloader, "rtpac3depay", "rtpac3depay");
  gst_element_factory_make_wrapper(&audio_convert, "audioconvert", "audioconvert");
  gst_element_factory_make_wrapper(&a52_decoder, "a52dec", "a52dec");

  gst_bin_add_many(bin, audio_convert, a52_decoder, ac3_depayloader, NULL);
  gst_element_link_wrapper(ac3_depayloader, a52_decoder);
  gst_element_link_wrapper(a52_decoder, audio_convert);

  setup_ghost_sink(ac3_depayloader, bin);
  setup_ghost_src(audio_convert, bin);

  StreamSession_t* session = create_stream_session(session_id);
  session->bin = GST_ELEMENT(bin);

  return session;
}


static gboolean setup_rtp_delivery_with_stream_session(GstCommonData_t* common_data, StreamSession_t* session)
{
  DEBUG("Start");
  DEBUG("create rtp elements");
  GstElement *rtp_src, *rtcp_src, *rtcp_sink;
  gst_element_factory_make_wrapper(&rtp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_src, "udpsrc", NULL);
  gst_element_factory_make_wrapper(&rtcp_sink, "udpsink", NULL);

  DEBUG("setup rtp elements");
  int session_port_base = 3000 + (session->id * 3); // for testing, set it as a creator's port number
  g_object_set(rtp_src, "port", session_port_base, NULL);
  g_object_set(rtcp_src, "port", session_port_base+1, NULL);
  g_object_set(rtcp_sink, "port", session_port_base+2, "host", "127.0.0.1", "sync", FALSE, "async", FALSE, NULL);
  DEBUG("rtp src: %d, rtcp src/sink: %d/%d", session_port_base, session_port_base+1, session_port_base+2);

  DEBUG("connect rtp signals to cb handlers");
  gst_bin_add_many(GST_BIN(common_data->pipeline), rtp_src, rtcp_src, rtcp_sink, NULL);
  g_signal_connect_data(common_data->rtp_bin, "pad-added", G_CALLBACK(rtp_pad_added_handler),
                        stream_session_ref(session), (GClosureNotify)stream_session_unref, 0);
  //g_signal_connect(common_data->rtp_bin, "request_aux_receiver", (GCallback)request_aux_receiver, session);
  //g_signal_connect_data(common_data->rtp_bin, "request-pt-map", G_CALLBACK(request_pt_map),
  //                      stream_session_ref(session), (GClosureNotify)stream_session_unref, 0);

  DEBUG("link rtp pads between udpsink/src and rtp bin");
  gst_element_link_pads(rtp_src, "src", common_data->rtp_bin, g_strdup_printf("recv_rtp_sink_%u", session->id));
  gst_element_link_pads(rtcp_src, "src", common_data->rtp_bin, g_strdup_printf("recv_rtcp_sink_%u", session->id));
  gst_element_link_pads(common_data->rtp_bin, g_strdup_printf("send_rtcp_src_%u", session->id), rtcp_sink, "sink");

  stream_session_unref(session);

  DEBUG("End");
  return TRUE;
}

#endif

