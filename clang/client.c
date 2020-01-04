/*
 * RTP receiver with RFC4588 retransmission handling enabled
 *
 *  In this example we have two RTP sessions, one for video and one for audio.
 *  Video is received on port 5000, with its RTCP stream received on port 5001
 *  and sent on port 5005. Audio is received on port 5005, with its RTCP stream
 *  received on port 5006 and sent on port 5011.
 *
 *  In both sessions, we set "rtprtxreceive" as the session's "aux" element
 *  in rtpbin, which enables RFC4588 retransmission handling for that session.
 *
 *             .-------.      .----------.        .-----------.   .---------.   .-------------.
 *  RTP        |udpsrc |      | rtpbin   |        |theoradepay|   |theoradec|   |autovideosink|
 *  port=5000  |      src->recv_rtp_0 recv_rtp_0->sink       src->sink     src->sink          |
 *             '-------'      |          |        '-----------'   '---------'   '-------------'
 *                            |          |
 *                            |          |     .-------.
 *                            |          |     |udpsink|  RTCP
 *                            |  send_rtcp_0->sink     | port=5005
 *             .-------.      |          |     '-------' sync=false
 *  RTCP       |udpsrc |      |          |               async=false
 *  port=5001  |     src->recv_rtcp_0    |
 *             '-------'      |          |
 *                            |          |
 *             .-------.      |          |        .---------.   .-------.   .-------------.
 *  RTP        |udpsrc |      |          |        |pcmadepay|   |alawdec|   |autoaudiosink|
 *  port=5006  |      src->recv_rtp_1 recv_rtp_1->sink     src->sink   src->sink          |
 *             '-------'      |          |        '---------'   '-------'   '-------------'
 *                            |          |
 *                            |          |     .-------.
 *                            |          |     |udpsink|  RTCP
 *                            |  send_rtcp_1->sink     | port=5011
 *             .-------.      |          |     '-------' sync=false
 *  RTCP       |udpsrc |      |          |               async=false
 *  port=5007  |     src->recv_rtcp_1    |
 *             '-------'      '----------'
 *
 */

#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <global.h>
#include <cb_basic.h>
#include <session.h>
#include <ghost.h>
#include <aux.h>


static SessionData_t* make_audio_session(guint session_id);
static SessionData_t* make_video_session(guint session_id);
static void handle_new_stream(GstElement *element, GstPad *new_pad, gpointer data);
static void join_session(CustomData_t* data, SessionData_t *session);


int main(int argc, char *argv[])
{
  CustomData_t data;
  memset(&data, 0, sizeof(data));
  GstBus* bus;
  SessionData_t* video_session;
  SessionData_t* audio_session;

  gst_init(&argc, &argv);

  data.loop = g_main_loop_new(NULL, FALSE);
  data.pipeline = GST_PIPELINE(gst_pipeline_new(NULL));
  bus = gst_element_get_bus(GST_ELEMENT(data.pipeline));

  // bus messages are handled by callback functions
  g_signal_connect(bus, "message::error", G_CALLBACK(cb_error), &data);
  g_signal_connect(bus, "message::warning", G_CALLBACK(cb_warning), &data);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(cb_state), &data);
  g_signal_connect(bus, "message::eos", G_CALLBACK(cb_eos), &data);
  gst_bus_add_signal_watch(bus);
  gst_object_unref(bus);

  // RTP bin combines the functions of rtpsession, rtpssrcdemux, rtpjitterbuffer, and rtpptdemux in one element.
  // It allows for multiple RTP sessions that will be synchronized together using RTCP SR packets.
  data.rtp_bin = gst_element_factory_make("rtpbin", NULL);
  gst_bin_add(GST_BIN(data.pipeline), data.rtp_bin);
  g_object_set(data.rtp_bin, "latency", 200, "do-retransmission", TRUE, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  video_session = make_video_session(0);
  audio_session = make_audio_session(1);

  join_session(&data, video_session);
  join_session(&data, audio_session);

  g_print("Starting client pipeline \n");
  gst_element_set_state(GST_ELEMENT(data.pipeline), GST_STATE_PLAYING);

  // export the pipeline graph
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)data.pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "client-pipeline");

  g_main_loop_run(data.loop);

  g_print("Stopping client pipeline \n");
  gst_element_set_state(GST_ELEMENT(data.pipeline), GST_STATE_NULL);

  gst_object_unref(data.pipeline);
  g_main_loop_unref(data.loop);

  return 0;
}


static SessionData_t* make_audio_session(guint session_id)
{
  SessionData_t *ret = make_session(session_id);
  GstBin *bin = GST_BIN(gst_bin_new("audio"));

  GstElement *queue, *depayloader, *decoder, *audio_convert, *audio_resample, *sink;
  queue = gst_element_factory_make("queue", NULL);
  depayloader = gst_element_factory_make("rtppcmadepay", NULL);
  decoder = gst_element_factory_make("alawdec", NULL);
  audio_convert = gst_element_factory_make("audioconvert", NULL);
  audio_resample = gst_element_factory_make("audioresample", NULL);
  sink = gst_element_factory_make("autoaudiosink", NULL);

  gst_bin_add_many(bin, queue, depayloader, decoder, audio_convert, audio_resample, sink, NULL);
  gst_element_link_many(queue, depayloader, decoder, audio_convert, audio_resample, sink, NULL);
  setup_ghost_sink(queue, bin);

  ret->bin = GST_ELEMENT(bin);
  ret->caps = gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "audio",
                                  "clock-rate", G_TYPE_INT, 8000, "encoding-time", G_TYPE_STRING, "PCMA", NULL);
  return ret;
}


static SessionData_t* make_video_session(guint session_id)
{
  SessionData_t *ret = make_session(session_id);
  GstBin *bin = GST_BIN(gst_bin_new("video"));

  GstElement *queue, *depayloader, *decoder, *converter, *sink;
  queue = gst_element_factory_make("queue", NULL);
  depayloader = gst_element_factory_make("rtptheoradepay", NULL);
  decoder = gst_element_factory_make("theoradec", NULL);
  converter = gst_element_factory_make("videoconvert", NULL);
  sink = gst_element_factory_make("autovideosink", NULL);

  gst_bin_add_many(bin, depayloader, decoder, converter, queue, sink, NULL);
  gst_element_link_many(queue, depayloader, decoder, converter, sink, NULL);

  setup_ghost_sink(queue, bin);

  ret->bin = GST_ELEMENT(bin);
  ret->caps = gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "video",
                                  "clock-rate", G_TYPE_INT, 90000, "encoding-name", G_TYPE_STRING, "THEORA", NULL);

  return ret;
}


/*
 * with the signal pad-added, compare the pad_name and session's pad prefix
 * if they are matched, add audio/video bins to rtpbin's parent and link the new_pad to audio/video bins' sink
 */
static void handle_new_stream(GstElement *element, GstPad *new_pad, gpointer data)
{
  SessionData_t *session = (SessionData_t*)data;
  gchar *pad_name, *target_prefix;

  pad_name = gst_pad_get_name(new_pad);
  target_prefix = g_strdup_printf("recv_rtp_src_%u", session->id);
  g_print("New pad: %s, looking for %s_* \n", pad_name, target_prefix);

  if(g_str_has_prefix(pad_name, target_prefix)) {
    GstPad *output_sink_pad;
    GstElement *parent;

    parent = GST_ELEMENT(gst_element_get_parent(session->rtp_bin));
    gst_bin_add(GST_BIN(parent), session->bin);
    gst_element_sync_state_with_parent(session->bin);
    gst_object_unref(parent);

    output_sink_pad = gst_element_get_static_pad(session->bin, "sink");
    g_assert_cmpint(gst_pad_link(new_pad, output_sink_pad), ==, GST_PAD_LINK_OK);
    gst_object_unref(output_sink_pad);

    g_print("Linked");
  }

  g_free(target_prefix);
  g_free(pad_name);
}


static void join_session(CustomData_t* data, SessionData_t *session)
{
  GstElement *rtp_src, *rtcp_src, *rtcp_sink;
  gchar *pad_name;
  guint port_base;

  g_print("Joining session %p \n", session);

  session->rtp_bin = g_object_ref(data->rtp_bin);

  port_base = 5000 + (session->id * 6);

  // make udpsrc(rtp_src, rtcp_src) and udpsink(rtcp_sink); rtcp should be bi-directional between server and client
  //   server: rtcp_src(port_base+5), rtcp_sink(port_base+1)
  //   client: rtcp_src(port_base+1), rtcp_sink(port_base+5)
  rtp_src = gst_element_factory_make("udpsrc", NULL);
  rtcp_src = gst_element_factory_make("udpsrc", NULL);
  rtcp_sink = gst_element_factory_make("udpsink", NULL);
  g_object_set(rtp_src, "port", port_base, "caps", session->caps, NULL);
  g_object_set(rtcp_src, "port", port_base + 1, NULL);
  g_object_set(rtcp_sink, "port", port_base + 5, "host", "127.0.0.1", "sync", FALSE, "async", FALSE, NULL);

  g_print("Connecting to %i/%i%i \n", port_base, port_base + 1, port_base + 5);

  gst_bin_add_many(GST_BIN(data->pipeline), rtp_src, rtcp_src, rtcp_sink, NULL);

  // connect signal with user data
  g_signal_connect_data(data->rtp_bin, "pad-added", G_CALLBACK(handle_new_stream),
                        session_ref(session), (GClosureNotify)session_unref, 0);
  // for the retransmission, request-aux-receiver and request-pt-map signals are used
  g_signal_connect(data->rtp_bin, "request-aux-receiver", (GCallback) request_aux_receiver, session);
  g_signal_connect_data(data->rtp_bin, "request-pt-map", G_CALLBACK(request_pt_map),
                        session_ref(session), (GClosureNotify)session_unref, 0);

  // udpsrc(rtp_src) - rtp_bin
  pad_name = g_strdup_printf("recv_rtp_sink_%u", session->id);
  gst_element_link_pads(rtp_src, "src", data->rtp_bin, pad_name);
  g_free(pad_name);

  // udpsrc(rtcp_src) - rtp_bin
  pad_name = g_strdup_printf("recv_rtcp_sink_%u", session->id);
  gst_element_link_pads(rtcp_src, "src", data->rtp_bin, pad_name);
  g_free(pad_name);

  // rtcp bi-drection: from client(rtp_bin - udpsink(rtcp_sink)) - source(rtcp_src - rtp_bin)
  pad_name = g_strdup_printf("send_rtcp_src_%u", session->id);
  gst_element_link_pads(data->rtp_bin, pad_name, rtcp_sink, "sink");
  g_free(pad_name);

  session_unref(session);
}

