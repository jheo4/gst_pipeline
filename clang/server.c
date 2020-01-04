/*
 * An RTP server
 *  creates two sessions and streams audio on one, video on the other, with RTCP
 *  on both sessions. The destination is 127.0.0.1.
 *
 *  In both sessions, we set "rtprtxsend" as the session's "aux" element
 *  in rtpbin, which enables RFC4588 retransmission for that session.
 *
 *  .-------.    .-------.    .-------.      .------------.       .-------.
 *  |audiots|    |alawenc|    |pcmapay|      | rtpbin     |       |udpsink|
 *  |      src->sink    src->sink    src->send_rtp_0 send_rtp_0->sink     |
 *  '-------'    '-------'    '-------'      |            |       '-------'
 *                                           |            |
 *  .-------.    .---------.    .---------.  |            |       .-------.
 *  |audiots|    |theoraenc|    |theorapay|  |            |       |udpsink|
 *  |      src->sink      src->sink  src->send_rtp_1 send_rtp_1->sink     |
 *  '-------'    '---------'    '---------'  |            |       '-------'
 *                                           |            |
 *                               .------.    |            |
 *                               |udpsrc|    |            |       .-------.
 *                               |     src->recv_rtcp_0   |       |udpsink|
 *                               '------'    |       send_rtcp_0->sink    |
 *                                           |            |       '-------'
 *                               .------.    |            |
 *                               |udpsrc|    |            |       .-------.
 *                               |     src->recv_rtcp_1   |       |udpsink|
 *                               '------'    |       send_rtcp_1->sink    |
 *                                           '------------'       '-------'
 *
 * To keep the set of ports consistent across both this server and the
 * corresponding client, a SessionData struct maps a rtpbin session number to
 * a GstBin and is used to create the corresponding udp sinks with correct
 * ports.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <global.h>
#include <session.h>
#include <cb_basic.h>
#include <aux.h>
#include <ghost.h>

static SessionData_t* make_audio_session(guint session_num);
static SessionData_t* make_video_session(guint session_num);
static void add_stream(CustomData_t* data, SessionData_t* session);


int main(int argc, char *argv[])
{
  CustomData_t data;
  memset(&data, 0, sizeof(data));

  GstBus *bus;
  SessionData_t *video_session, *audio_session;

  gst_init(&argc, &argv);

  data.loop = g_main_loop_new(NULL, FALSE);

  // Create a new pipeline & watch signals from the pipeline's bus
  data.pipeline = GST_PIPELINE(gst_pipeline_new(NULL));
  bus = gst_element_get_bus(GST_ELEMENT(data.pipeline));
  g_signal_connect(bus, "message::error", G_CALLBACK(cb_error), &data);
  g_signal_connect(bus, "message::warning", G_CALLBACK(cb_warning), &data);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(cb_state), &data);
  g_signal_connect(bus, "message::eos", G_CALLBACK(cb_eos), &data);
  gst_bus_add_signal_watch(bus);
  gst_object_unref(bus);

  // RTP bin combines the functions of rtpsession, rtpssrcdemux, rtpjitterbuffer and rtpptdemux in one element.
  // It allows for multiple RTP sessions that will be synchronized together using RTCP SR packets.
  data.rtp_bin = gst_element_factory_make("rtpbin", NULL);

  // GstRTPProfile:
  //   GST_RTP_PROFILE_UNKNOWN: invalid profile
  //   GST_RTP_PROFILE_AVP: the Audio/Visual profile (RFC 3551)
  //   GST_RTP_PROFILE_SAVP: the secure Audio/Visual profile (RFC 3711)
  //   GST_RTP_PROFILE_AVPF: the Audio/Visual profile with feedback (RFC 4585)
  //   GST_RTP_PROFILE_SAVPF: the secure Audio/Visual profile with feedback (RFC 5124)
  g_object_set(data.rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  // add rtp_bin to the empty pipeline
  gst_bin_add(GST_BIN(data.pipeline), data.rtp_bin);

  // make video & audio RTP sessions
  video_session = make_video_session(0);
  audio_session = make_audio_session(1);

  add_stream(&data, video_session);
  add_stream(&data, audio_session);

  g_print("starting server pipeline \n");
  gst_element_set_state(GST_ELEMENT(data.pipeline), GST_STATE_PLAYING);

  // export the pipeline graph
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)data.pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "server-pipeline");

  g_main_loop_run(data.loop);

  g_print("stopping server pipeline \n");
  gst_element_set_state(GST_ELEMENT(data.pipeline), GST_STATE_NULL);

  gst_object_unref(data.pipeline);
  g_main_loop_unref(data.loop);

  return 0;
}


/*
 * Session: bin, # of bin references, session number (id)
 * audio bin: (audio_src - encoder - payloader) -- ghost_pad -->
 */
static SessionData_t* make_audio_session(guint session_num)
{
  SessionData_t *session;
  GstBin *audio_bin = GST_BIN(gst_bin_new(NULL));
  GstElement *audio_src, *encoder, *payloader;

  audio_src = gst_element_factory_make("audiotestsrc", NULL); // currently testsrc
  encoder = gst_element_factory_make("alawenc", NULL);
  payloader = gst_element_factory_make("rtppcmapay", NULL);

  g_object_set(audio_src, "is-live", TRUE, NULL);

  gst_bin_add_many(audio_bin, audio_src, encoder, payloader, NULL);
  gst_element_link_many(audio_src, encoder, payloader, NULL);

  setup_ghost_src(payloader, audio_bin);

  session = make_session(session_num);
  session->bin = GST_ELEMENT(audio_bin);

  return session;
}


/*
 * Session: bin, # of bin references, session number (id)
 * video bin: (audio_src -(Caps)- encoder - payloader) -- ghost_pad -->
 */
static SessionData_t* make_video_session(guint session_num)
{
  SessionData_t *session;
  GstBin *video_bin = GST_BIN(gst_bin_new(NULL));
  GstElement *video_src, *encoder, *payloader;
  GstCaps *video_caps;

  video_src = gst_element_factory_make("videotestsrc", NULL); // currently testsrc
  encoder = gst_element_factory_make("theoraenc", NULL);
  payloader = gst_element_factory_make("rtptheorapay", NULL);

  g_object_set(video_src, "is-live", TRUE, "horizontal-speed", 1, NULL);
  g_object_set(payloader, "config-interval", 2, NULL);

  gst_bin_add_many(video_bin, video_src, encoder, payloader, NULL);
  video_caps = gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, 352, "height", G_TYPE_INT, 288,
                                   "framerate", GST_TYPE_FRACTION, 15, 1, NULL);

  gst_element_link_filtered(video_src, encoder, video_caps);
  gst_element_link(encoder, payloader);

  setup_ghost_src(payloader, video_bin);

  session = make_session(session_num);
  session->bin = GST_ELEMENT(video_bin);

  return session;
}


/*
 * add udpsinks (streams) to the pipeline & connects them to session & rtp_bin
 * for the complete pipeline, connect request_aux_sender siganl to the callback function; request_aux_sender
 *   RTP: session (src) - rtp_bin - udpsink (rtp_sink)
 *   RTCP: udpsrc (rtcp_src) - rtp_bin udpsink (rtcp_sink)
 */
static void add_stream(CustomData_t* data, SessionData_t *session)
{
  GstElement *rtp_sink, *rtcp_sink, *rtcp_src, *identity;
  int port_base;
  gchar *pad_name;

  rtp_sink = gst_element_factory_make("udpsink", NULL);
  rtcp_sink = gst_element_factory_make("udpsink", NULL);
  rtcp_src = gst_element_factory_make("udpsrc", NULL);
  identity = gst_element_factory_make("identity", NULL);

  port_base = 5000 + (session->id * 6);

  // add all elements into the pipeline & connect a signal to a callback function
  gst_bin_add_many(GST_BIN(data->pipeline), rtp_sink, rtcp_sink, rtcp_src, identity, session->bin, NULL);
  g_signal_connect(data->rtp_bin, "request_aux_sender", (GCallback)request_aux_sender, session);

  g_object_set(rtp_sink, "port", port_base, "host", "127.0.0.1", NULL);
  g_object_set(rtcp_sink, "port", port_base+1,"host", "127.0.0.1", "sync", FALSE, "async", FALSE, NULL);
  g_object_set(rtcp_src, "port", port_base+5, NULL);
  g_object_set(identity, "drop-probability", 0.01, NULL); // to drop some rtp packets at random for demonstrating
                                                          // rtprtxsend works

  // session_bin (audio/video bins) - rtp_bin - identity (pack drop) - rtp_sink: with pack drops
  // session_bin (audio/video bins) - rtp_bin - rtp_sink: without packet drops
  pad_name = g_strdup_printf("send_rtp_sink_%u", session->id);
  gst_element_link_pads(session->bin, "src", data->rtp_bin, pad_name);
  g_free(pad_name);
  pad_name = g_strdup_printf("send_rtp_src_%u", session->id);
  gst_element_link_pads(data->rtp_bin, pad_name, identity, "sink");
  gst_element_link(identity, rtp_sink);
  g_free(pad_name);

  // rtcp_src - rtp_bin - rtcp_sink: rtcp is bi-directional between the client and the server
  pad_name = g_strdup_printf("recv_rtcp_sink_%d", session->id);
  gst_element_link_pads(rtcp_src, "src", data->rtp_bin, pad_name);
  g_free(pad_name);
  pad_name = g_strdup_printf("send_rtcp_src_%u", session->id);
  gst_element_link_pads(data->rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  g_print("New RTP stream on %i/%i/%i \n", port_base, port_base+1, port_base+5);

  session_unref(session);
}

