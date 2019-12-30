#include <gst/gst.h>
#include <string.h>
#include <stdio.h>

#define GRAPH_LENGTH 78

typedef enum
{
  GST_PLAY_FLAG_DOWNLOAD = (1 << 7),
} GstPlayFlags;

typedef struct _CustomData_t
{
  gboolean is_live;
  GstElement *pipeline;
  GMainLoop *loop;
  gint buf_lv;
} CustomData_t;

static void got_location(GstObject *obj, GstObject *prop_obj, GParamSpec *prop, gpointer data);
static void cb_msg(GstBus *bus, GstMessage *msg, CustomData_t *data);
static gboolean refresh_ui(CustomData_t *data);

int main(int argc, char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstStateChangeReturn ret;
  GMainLoop *main_loop;
  CustomData_t data;
  guint flags;

  gst_init(&argc, &argv);

  memset(&data, 0, sizeof(data));
  data.buf_lv = 100;

  pipeline =  gst_parse_launch(
                  "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
                  NULL);
  bus = gst_element_get_bus(pipeline);

  g_object_get(pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_DOWNLOAD;
  g_object_set(pipeline, "flags", flags, NULL);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  else if(ret == GST_STATE_CHANGE_NO_PREROLL) data.is_live = TRUE;

  main_loop = g_main_loop_new(NULL, FALSE);
  data.loop = main_loop;
  data.pipeline = pipeline;

  gst_bus_add_signal_watch(bus);
  g_signal_connect(bus, "message", G_CALLBACK(cb_msg), &data);

  // deep-notify: emitted when the properties of any of children elements change
  g_signal_connect(pipeline, "deep-notify::temp-location", G_CALLBACK(got_location), NULL);
  g_timeout_add_seconds(1, (GSourceFunc)refresh_ui, &data);
  g_main_loop_run(main_loop);

  g_main_loop_unref(main_loop);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_print ("\n");
  return 0;
}

static void got_location(GstObject *obj, GstObject *prop_obj, GParamSpec *prop, gpointer data)
{
  gchar *location;
  g_object_get(G_OBJECT(prop_obj), "temp-location", &location, NULL);
  g_print("Temporary file: %s \n", location);
  g_free(location);
  //g_object_set (G_OBJECT (prop_object), "temp-remove", FALSE, NULL); // keep temp files after this program exits
}


static void cb_msg(GstBus *bus, GstMessage *msg, CustomData_t *data)
{
  switch(GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error(msg, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      gst_element_set_state (data->pipeline, GST_STATE_READY);
      g_main_loop_quit (data->loop);
      break;
    }
    case GST_MESSAGE_EOS:
      gst_element_set_state (data->pipeline, GST_STATE_READY);
      g_main_loop_quit (data->loop);
      break;
    case GST_MESSAGE_BUFFERING:
      if(data->is_live) break;

      gst_message_parse_buffering(msg, &data->buf_lv);
      if(data->buf_lv < 100)
        gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
      else
        gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
      break;
    case GST_MESSAGE_CLOCK_LOST:
      gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
      gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
      break;
    default:
      break;
  }
}


static gboolean refresh_ui(CustomData_t *data)
{
  GstQuery *query;
  gboolean res;

  query = gst_query_new_buffering(GST_FORMAT_PERCENT);
  res = gst_element_query(data->pipeline, query);

  if(res) {
    gint n_range, range, i;;
    gchar graph[GRAPH_LENGTH + 1];
    gint64 position = 0, duration = 0;

    memset(graph, ' ', GRAPH_LENGTH);
    graph[GRAPH_LENGTH] = '\0';

    n_range = gst_query_get_n_buffering_ranges(query);
    for(range = 0; range < n_range; range++) {
      gint64 start, stop;
      gst_query_parse_nth_buffering_range(query, range, &start, &stop);
      start = start * GRAPH_LENGTH / (stop - start);
      stop = stop * GRAPH_LENGTH / (stop - start);
      for(i = (gint)start; i < stop; i++)
        graph[i] = '-';
    }

    if(gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &position) && GST_CLOCK_TIME_IS_VALID(position) &&
       gst_element_query_duration(data->pipeline, GST_FORMAT_TIME, &duration) && GST_CLOCK_TIME_IS_VALID(duration)) {
      i = (gint)(GRAPH_LENGTH * (double)position / (double)duration + 1);
      graph[i] = data->buf_lv < 100 ? 'X' : '>';
    }
    g_print("[%s]", graph);

    if(data->buf_lv < 100) g_print (" Buffering: %3d%%", data->buf_lv);
		else g_print ("                ");
    g_print ("\r");
  }
  return TRUE;
}

