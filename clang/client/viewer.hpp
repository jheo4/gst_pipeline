#include <stdio.h>
#include <string>
#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <aux.h>

#ifndef __VIEWER__
#define __VIEWER__


class Viewer : GstCommon
{
  public:
    // Attributes
    GstCommonData_t* common_data;
    GstElement *v_sink;
    StreamSession_t *v_session;

    // Methods
    Viewer();
    ~Viewer();
    void setup_rtp_receiver_with_stream_session(StreamSession_t* session,
                                              const char* addr, int port_base);
    StreamSession_t* make_video_session(guint id);
    void run();
};


static void rtp_pad_added_handler(GstElement* src, GstPad* new_pad,
                                  StreamSession_t* session)
{
  gchar* new_pad_name = gst_pad_get_name(new_pad);
  gchar* target_prefix = g_strdup_printf("recv_rtp_src_%u", session->id);

  DEBUG("New pad: %s, Target Prefix: %s* \n", new_pad_name, target_prefix);

  if(g_str_has_prefix(new_pad_name, target_prefix)) {
    DEBUG("new pad has target prefix");

    GstElement* parent = GST_ELEMENT(gst_element_get_parent(session->rtp_bin));
    DEBUG("rtp_bin parent name: %s", gst_element_get_name(parent));

    gst_bin_add(GST_BIN(parent), session->bin);
    gst_element_sync_state_with_parent(session->bin);
    gst_object_unref(parent);

    GstPad* target_sink_pad = gst_element_get_static_pad(session->bin, "sink");
    g_assert_cmpint(gst_pad_link(new_pad, target_sink_pad), ==, GST_PAD_LINK_OK);
    gst_object_unref(target_sink_pad);
  }

  g_free(target_prefix);
  g_free(new_pad_name);
}


#endif

