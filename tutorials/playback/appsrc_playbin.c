#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1024
#define SAMPLE_RATE 44100

typedef struct _CustomData_t
{
  GstElement *pipline;
  GstElement *app_source;
  GMainLoop *main_loop;

  guint64 n_samples;
  gfloat a, b, c, d;
  guint src_id;
} CustomData_t;


static gboolean push_data(CustomData_t *data);
static void start_feed(GstElement *src, guint size, CustomData_t *data);
static void stop_feed(GstElement *src, CustomData_t *data);
static void error_cb(GstBus *bus, GstMessage *msg, CustomData_t *data);
static void src_setup(GstElement *pipeline, GstElement *src, CustomData_t *data);


int main(int argc, char *argv[])
{
  CustomData_t data;
  GstBus *bus;

  memset(&data, 0, sizeof(data));
  data.b = 1;
  data.d = 1;

  gst_init(&argc, &argv);
  data.pipline = gst_parse_launch("playbin uri=appsrc://", NULL);
  g_signal_connect(data.pipline, "source-setup", G_CALLBACK(src_setup), &data);

  bus = gst_element_get_bus(data.pipline);
  gst_bus_add_signal_watch(bus);
  g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, &data);
  gst_object_unref(bus);

  gst_element_set_state(data.pipline, GST_STATE_PLAYING);

  data.main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.main_loop);

  gst_element_set_state(data.pipline, GST_STATE_NULL);
  gst_object_unref(data.pipline);

  return 0;
}


static gboolean push_data(CustomData_t *data)
{
  GstBuffer *buf;
  GstFlowReturn ret;
  int i;
  GstMapInfo map;
  gint16 *raw;
  gint n_samples = BUF_SIZE / 2;  // each sample is 16 bits
  gfloat freq;

  buf = gst_buffer_new_and_alloc(BUF_SIZE);
  GST_BUFFER_TIMESTAMP(buf) = gst_util_uint64_scale(data->n_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION(buf) = gst_util_uint64_scale(n_samples, GST_SECOND, SAMPLE_RATE);
  gst_buffer_map(buf, &map, GST_MAP_WRITE);

  raw = (gint16 *)map.data;
  data->c = data->d;
   data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for (i = 0; i < n_samples; i++) {
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(500 * data->a);
  }

  gst_buffer_unmap(buf, &map);
  data->n_samples += n_samples;

  g_signal_emit_by_name(data->app_source, "push-buffer", buf, &ret);
  gst_buffer_unref(buf);

  if(ret != GST_FLOW_OK) return FALSE;
  return TRUE;
}


static void start_feed(GstElement *src, guint size, CustomData_t *data)
{
  if(data->src_id == 0) {
    g_print("Start feeding \n");
    data->src_id = g_idle_add((GSourceFunc) push_data, data);
  }
}


static void stop_feed(GstElement *src, CustomData_t *data)
{
  if(data->src_id != 0) {
    g_print("Stop feeding\n");
    g_source_remove(data->src_id);
    data->src_id = 0;
  }
}


static void error_cb(GstBus *bus, GstMessage *msg, CustomData_t *data)
{
  GError *err;
  gchar *debug_info;

  /* Print error details on the screen */
  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  g_main_loop_quit (data->main_loop);
}


static void src_setup(GstElement *pipeline, GstElement *src, CustomData_t *data)
{
  GstAudioInfo info;
  GstCaps *audio_caps;

  g_print("Source has been created. Configuring. \n");
  data->app_source = src;

  gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
  audio_caps = gst_audio_info_to_caps(&info);
  g_object_set(src, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
  g_signal_connect(src, "need-data", G_CALLBACK(start_feed), data);
  g_signal_connect(src, "enough-data", G_CALLBACK(stop_feed), data);
  gst_caps_unref(audio_caps);
}

