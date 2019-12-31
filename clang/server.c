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


#include <gst/gst.h>
#include <gst/rtp/rtp.h>


typedef struct _SessionData_t
{
  int ref;
  guint session_num;
  GstElement *input;
} SessionData_t;


static SessionData_t* session_ref(SessionData_t *data);
static void session_unref(SessionData_t *data);
static SessionData_t* session_new(guint session_num);
static void cb_state(GstBus *bus, GstMessage *msg, gpointer data);
static void setup_ghost(GstElement *src, GstBin *bin);

static SessionData_t* make_audio_session(guint session_num);
static SessionData_t* make_video_session(guint session_num);

static GstElement* request_aux_sender(GstElement *rtp_bin, guint sess_id, SessionData_t *session);
static void add_stream(GstPipeline *pipe, GstElement *rtp_bin, SessionData_t *session);


int main(int argc, char *argv[])
{
  GstPipeline *pipeline;
  GstBus *bus;
  SessionData_t *video_session, *audio_session;
  GstElement *rtp_bin;
  GMainLoop *loop;

  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  // Create a new pipeline & watch signals from the pipeline's bus
  pipeline = GST_PIPELINE(gst_pipeline_new(NULL));
  bus = gst_element_get_bus(GST_ELEMENT(pipeline));
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(cb_state), pipeline);
  gst_bus_add_signal_watch(bus);
  gst_object_unref(bus);

  // RTP bin combines the functions of rtpsession, rtpssrcdemux, rtpjitterbuffer and rtpptdemux in one element.
  // It allows for multiple RTP sessions that will be synchronized together using RTCP SR packets.
  rtp_bin = gst_element_factory_make("rtpbin", NULL);

  // GstRTPProfile:
  //   GST_RTP_PROFILE_UNKNOWN: invalid profile
  //   GST_RTP_PROFILE_AVP: the Audio/Visual profile (RFC 3551)
  //   GST_RTP_PROFILE_SAVP: the secure Audio/Visual profile (RFC 3711)
  //   GST_RTP_PROFILE_AVPF: the Audio/Visual profile with feedback (RFC 4585)
  //   GST_RTP_PROFILE_SAVPF: the secure Audio/Visual profile with feedback (RFC 5124)
  g_object_set(rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  // add rtp_bin to the empty pipeline
  gst_bin_add(GST_BIN(pipeline), rtp_bin);

  // make video & audio RTP sessions
  video_session = make_video_session(0);
  audio_session = make_audio_session(1);

  add_stream(pipeline, rtp_bin, video_session);
  add_stream(pipeline, rtp_bin, audio_session);

  g_print("starting server pipeline \n");
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  g_main_loop_run(loop);

  g_print("stopping server pipeline \n");
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);

  gst_object_unref(pipeline);
  g_main_loop_unref(loop);

  return 0;
}


static SessionData_t* session_ref(SessionData_t *data)
{
  g_atomic_int_inc(&data->ref);
  return data;
}


static void session_unref(SessionData_t *data)
{
  if(g_atomic_int_dec_and_test(&data->ref)) g_free((gpointer)data);
}


static SessionData_t* session_new(guint session_num)
{
  SessionData_t *ret = g_new0(SessionData_t, 1);
  ret->session_num = session_num;
  return session_ref(ret);
}


static void cb_state(GstBus *bus, GstMessage *msg, gpointer data)
{
  GstObject *pipeline = GST_OBJECT(data);
  GstState old, new, pending;
  gst_message_parse_state_changed(msg, &old, &new, &pending);

  if(msg->src == pipeline) g_print("pipeline %s changed state from %s to %s \n", GST_OBJECT_NAME(pipeline),
                                   gst_element_state_get_name(old), gst_element_state_get_name(new));
}


static void setup_ghost(GstElement *src, GstBin *bin)
{
  GstPad *src_pad = gst_element_get_static_pad(src, "src");
  GstPad *bin_pad = gst_ghost_pad_new("src", src_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
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

  setup_ghost(payloader, audio_bin);

  session = session_new(session_num);
  session->input = GST_ELEMENT(audio_bin);

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

  setup_ghost(payloader, video_bin);

  session = session_new(session_num);
  session->input = GST_ELEMENT(video_bin);

  return session;
}

/*
 * when the request_aux_sender signal is received, request_aux_sender returns the auxiliary stream sender
 *   auxiliary sender: new bin of rtprtxsend with ghost sink & source
 */
static GstElement* request_aux_sender(GstElement *rtp_bin, guint sess_id, SessionData_t *session)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO("creating AUX sender");

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


/*
 * add udpsinks (streams) to the pipeline & connects them to session & rtp_bin
 * for the complete pipeline, connect request_aux_sender siganl to the callback function; request_aux_sender
 *   RTP: session (src) - rtp_bin - udpsink (rtp_sink)
 *   RTCP: udpsrc (rtcp_src) - rtp_bin udpsink (rtcp_sink)
 */
static void add_stream(GstPipeline *pipeline, GstElement *rtp_bin, SessionData_t *session)
{
  GstElement *rtp_sink, *rtcp_sink, *rtcp_src, *identity;
  int port_base;
  gchar *pad_name;

  rtp_sink = gst_element_factory_make("udpsink", NULL);
  rtcp_sink = gst_element_factory_make("udpsink", NULL);
  rtcp_src = gst_element_factory_make("udpsrc", NULL);
  identity = gst_element_factory_make("identity", NULL);

  port_base = 5000 + (session->session_num * 6);

  // add all elements into the pipeline & connect a signal to a callback function
  gst_bin_add_many(GST_BIN(pipeline), rtp_sink, rtcp_sink, rtcp_src, identity, session->input, NULL);
  g_signal_connect(rtp_bin, "request_aux_sender", (GCallback)request_aux_sender, session);

  g_object_set(rtp_sink, "port", port_base, "host", "127.0.0.1", NULL);
  g_object_set(rtcp_sink, "port", port_base+1,"host", "127.0.0.1", "sync", FALSE, "async", FALSE, NULL);
  g_object_set(rtcp_src, "port", port_base+5, NULL);
  g_object_set(identity, "drop-probability", 0.01, NULL); // to drop some rtp packets at random for demonstrating
                                                          // rtprtxsend works

  // session_bin (audio/video bins) - rtp_bin - identity (pack drop) - rtp_sink: with pack drops
  // session_bin (audio/video bins) - rtp_bin - rtp_sink: without packet drops
  pad_name = g_strdup_printf("send_rtp_sink_%u", session->session_num);
  gst_element_link_pads(session->input, "src", rtp_bin, pad_name);
  g_free(pad_name);
  pad_name = g_strdup_printf("send_rtp_src_%u", session->session_num);
  gst_element_link_pads(rtp_bin, pad_name, identity, "sink");
  gst_element_link(identity, rtp_sink);
  g_free(pad_name);

  // rtcp_src - rtp_bin - rtcp_sink: rtcp is bi-directional between the client and the server
  pad_name = g_strdup_printf("recv_rtcp_sink_%d", session->session_num);
  gst_element_link_pads(rtcp_src, "src", rtp_bin, pad_name);
  g_free(pad_name);
  pad_name = g_strdup_printf("send_rtcp_src_%u", session->session_num);
  gst_element_link_pads(rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  g_print("New RTP stream on %i/%i/%i \n", port_base, port_base+1, port_base+5);

  session_unref(session);
}

