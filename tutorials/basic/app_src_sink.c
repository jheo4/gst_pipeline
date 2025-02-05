/* Data can be injected into a pipeline & extracted from it.
 *   appsrc: injects application data into a GStreamer pipeline.
 *     - push mode: can block the app or listen to signals (enough-data / need-data)
 *   appsink: extracts GStreamer data back to the application.
 *   buffers: data travels through a GStreamer pipeline in chuncks (a unit of data).
 *     - source pads produce buffers / sink pads consume buffers.
 *     - GstBuffer can contain multiple GstMemory objects.
 *     - every buffer has time-stamps & duration describing
 *       in which moment the content of the buffer should be decoded & rendered & displayed.
 *       - after demuxing, each buffer will contain a single video frame with raw caps & precise time-stamps.
 */

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1024
#define SAMPLE_RATE 44100

typedef struct _CustomData
{
  GstElement *pipeline, *app_source, *tee;
  GstElement *audio_queue, *audio_convert1, *audio_resample, *audio_sink; // branch1
  GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink; // branch2
  GstElement *app_queue, *app_sink; // branch3

  guint64 num_samples;
  gfloat a, b, c, d;
  guint source_id;

  GMainLoop *main_loop;
} CustomData_t;


static gboolean push_data(CustomData_t *data)
{
  GstBuffer *buffer;
  GstFlowReturn ret;
  GstMapInfo map;
  gint16 *raw;
  gint num_samples = BUF_SIZE / 2;
  gfloat freq;

  buffer = gst_buffer_new_and_alloc(BUF_SIZE);
  GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  raw = (gint16*)map.data;
  data->c += data->d;
  data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for (int i = 0; i < num_samples; i++) {
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(500 * data->a);
  }
  gst_buffer_unmap (buffer, &map);
  data->num_samples += num_samples;

  g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  if(ret != GST_FLOW_OK)
    return FALSE;

  return TRUE;
}


static void start_feed(GstElement *source, guint size, CustomData_t *data)
{
  if(data->source_id == 0) {
    g_print("start feeding. \n");
    // g_idle_add: add an idle function which is called whenever the GLib main loop is idle.
    // buffers don't need to be fed into appsrc from only the GLib main thread with need-data & enough-data.
    data->source_id = g_idle_add((GSourceFunc) push_data, data);
  }
}


static void stop_feed(GstElement *source, CustomData_t *data)
{
  if(data->source_id != 0) {
    g_print("Stop feeding. \n");
    g_source_remove(data->source_id);
    data->source_id = 0;
  }
}


static GstFlowReturn new_sample(GstElement *sink, CustomData_t *data)
{
  GstSample *sample;

  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if(sample) {
    g_print("*");
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}


static void error_cb (GstBus *bus, GstMessage *msg, CustomData_t *data) {
  GError *err;
  gchar *debug_info;

  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  g_main_loop_quit (data->main_loop);
}


int main(int argc, char *argv[])
{
  CustomData_t data;
  GstPad *tee_audio_pad, *tee_video_pad, *tee_app_pad;
  GstPad *queue_audio_pad, *queue_video_pad, *queue_app_pad;
  GstAudioInfo info;
  GstCaps *audio_caps;
  GstBus *bus;

  memset(&data, 0, sizeof(data));
  data.b = 1;
  data.d = 1;

  gst_init(&argc, &argv);

  data.pipeline = gst_pipeline_new("test-pipeline");
  data.app_source = gst_element_factory_make("appsrc", "audio_source");
  data.tee = gst_element_factory_make("tee", "tee");

  data.audio_queue = gst_element_factory_make("queue", "audio_queue");
  data.audio_convert1 = gst_element_factory_make("audioconvert", "audio_convert1");
  data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
  data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");

  data.video_queue = gst_element_factory_make("queue", "video_queue");
  data.audio_convert2 = gst_element_factory_make("audioconvert", "audio_convert2");
  data.visual = gst_element_factory_make("wavescope", "visual");
  data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
  data.video_sink = gst_element_factory_make("autovideosink", "video_sink");

  data.app_queue = gst_element_factory_make("queue", "app_queue");
  data.app_sink = gst_element_factory_make("appsink", "app_sink");

  if(!data.pipeline || !data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 ||
     !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual ||
     !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink) {
    g_printerr("Not all elements could be created. \n");
    return -1;
  }

  // wavescope
  g_object_set(data.visual, "shader", 0, "style", 0, NULL);

  // appsrc
  gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
  audio_caps = gst_audio_info_to_caps(&info);
  g_object_set(data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
  g_signal_connect(data.app_source, "need-data", G_CALLBACK(start_feed), &data);
  g_signal_connect(data.app_source, "enough-data", G_CALLBACK(stop_feed), &data);

  // appsink
  g_object_set(data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL); // enable emit-signals
  g_signal_connect(data.app_sink, "new-sample", G_CALLBACK(new_sample), &data);
  gst_caps_unref(audio_caps);

  // pipeline configuration
  gst_bin_add_many(GST_BIN(data.pipeline), data.app_source, data.tee, data.audio_queue, data.audio_convert1,
                   data.audio_resample, data.audio_sink, data.video_queue, data.audio_convert2, data.visual,
                   data.video_convert, data.video_sink, data.app_queue, data.app_sink, data.app_queue, data.app_sink,
                   NULL);
  if(!gst_element_link_many(data.app_source, data.tee, NULL) ||
     !gst_element_link_many(data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL) ||
     !gst_element_link_many(data.video_queue, data.audio_convert2, data.visual, data.video_convert,
                            data.video_sink, NULL) ||
     !gst_element_link_many(data.app_queue, data.app_sink, NULL)) {
    g_printerr("Elements could not be linked. \n");
    return -1;
  }

  tee_audio_pad = gst_element_get_request_pad(data.tee, "src_%u");
  tee_video_pad = gst_element_get_request_pad(data.tee, "src_%u");
  tee_app_pad = gst_element_get_request_pad(data.tee, "src_%u");
  queue_audio_pad = gst_element_get_static_pad(data.audio_queue, "sink");
  queue_video_pad = gst_element_get_static_pad(data.video_queue, "sink");
  queue_app_pad = gst_element_get_static_pad(data.app_queue, "sink");

  g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
  g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
  g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));

  if(gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
     gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
     gst_pad_link(tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK) {
    g_printerr("Tee could not be linked to queues.\n");
    g_object_unref(data.pipeline);
    return -1;
  }
  gst_object_unref(queue_audio_pad);
  gst_object_unref(queue_video_pad);
  gst_object_unref(queue_app_pad);

  bus = gst_element_get_bus(data.pipeline);
  gst_bus_add_signal_watch(bus);
  g_signal_connect(GST_OBJECT(bus), "message::error", (GCallback)error_cb, &data);
  gst_object_unref(bus);

  gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  data.main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.main_loop);

  gst_element_release_request_pad(data.tee, tee_audio_pad);
  gst_element_release_request_pad(data.tee, tee_video_pad);
  gst_element_release_request_pad(data.tee, tee_app_pad);
  gst_object_unref(tee_audio_pad);
  gst_object_unref(tee_video_pad);
  gst_object_unref(tee_app_pad);

  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
}

