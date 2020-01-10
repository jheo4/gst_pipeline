#include <gst/gst.h>
#include <stream_session.h>
#include <gst_common.h>

#ifndef __RTP_RTCP__
#define __RTP_RTCP__

static GstElement* request_aux_sender(GstElement*, guint, StreamSession_t*);
static GstElement* request_aux_receiver(GstElement*, guint, StreamSession_t*);

/*
 * when the request_aux_sender signal is received, request_aux_sender returns the auxiliary stream sender
 *   auxiliary sender: new bin of rtprtxsend with ghost sink & source
 */
static GstElement* request_aux_sender(GstElement* rtp_bin, guint sess_id, StreamSession_t* session)
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

  pt_map = gst_structure_new("application/x-rtp-pt-map", "8", G_TYPE_UINT, 98, "96", G_TYPE_UINT, 99, NULL);
  g_object_set(rtx, "payload-type-map", pt_map, NULL); // https://en.wikipedia.org/wiki/RTP_payload_formats
  gst_structure_free(pt_map);

  gst_bin_add(GST_BIN(bin), rtx);

  pad = gst_element_get_static_pad(rtx, "src");
  name = g_strdup_printf("src_%u", sess_id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  pad = gst_element_get_static_pad(rtx, "sink");
  name = g_strdup_printf("sink_%u", sess_id);
  gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
  g_free(name);
  gst_object_unref(pad);

  return bin;
}


static GstElement* request_aux_receiver(GstElement* rtp_bin, guint session_id, StreamSession_t* session)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO("Creating AUX receiver");

  bin = gst_bin_new(NULL);
  rtx = gst_element_factory_make("rtprtxreceive", NULL);
  // currently pt_map is hardcoded
  pt_map = gst_structure_new("application/x-rtp-pt-map", "8", G_TYPE_UINT, 98, "96", G_TYPE_UINT, 99, NULL);
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


/*
 *  request the payload-type mapping
 */
static GstCaps* request_pt_map(GstElement* source, guint session_id, guint pt, StreamSession_t* session)
{
  gchar *caps_str;
  g_print("Looking for caps for pt %u in session %u, have %u \n", pt, session_id, session->id);

  if(session_id == session->id) {
    caps_str = gst_caps_to_string(session->caps);
    g_print("Returning %s \n", caps_str);
    g_free(caps_str);
    return gst_caps_ref(session->caps);
  }

  return NULL;
}

#endif

