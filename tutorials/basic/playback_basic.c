/*
 * 2 Mechanisms to change the playback rate
 *   - Step Events
 *     - allow skipping a givem amount of media and change the playback rate only to positive values
 *     - only affect the sink; only work if the rest of the pipeline can support going at a different speed.
 *   - Seek Events
 *     - allow jumping to any position in the stream and set positive & negative playback rates
 *     - go all the way through the pipeline; every element can react to them
 *
 * These events are created and passed onto the pipeline until they reach an element that can handle them.
 */

#include <string.h>
#include <stdio.h>
#include <gst/gst.h>

typedef struct _CustomData_t
{
  GstElement *pipeline;
  GstElement *video_sink;
  GMainLoop *loop;

  gboolean playing;
  gdouble rate;
} CustomData_t;


static void send_seek_event(CustomData_t *data)
{
  gint64 position;
  GstEvent *seek_event;

  if(!gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &position)) {
    g_printerr("Unable to retrieve current position. \n");
    return;
  }

  if(data->rate > 0)
    seek_event = gst_event_new_seek(data->rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                    GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_END, 0);
  else
    seek_event = gst_event_new_seek(data->rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                    GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);

  if(data->video_sink == NULL)
    g_object_get(data->pipeline, "video-sink", &data->video_sink, NULL);

  gst_element_send_event(data->video_sink, seek_event);
  gst_print("Current rate: %g\n", data->rate);
}


static gboolean handle_keyboard(GIOChannel *src, GIOCondition cond, CustomData_t *data)
{
  gchar *str = NULL;
  if(g_io_channel_read_line(src, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) return TRUE;

  switch(g_ascii_tolower(str[0])) {
    case 'p':
      data->playing = !data->playing;
      gst_element_set_state(data->pipeline, data->playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
      g_print("Setting state to %s \n", data->playing ? "PLAYING" : "PAUSE");
      break;

    case 's':
      if(g_ascii_isupper(str[0])) data->rate *= 2.0;
      else data->rate /= 2.0;

      send_seek_event(data);
      break;

    case 'd':
      data->rate *= -1.0;
      send_seek_event(data);
      break;

    case 'n':
      if(data->video_sink == NULL) g_object_get(data->pipeline, "video-sink", &data->video_sink, NULL);
      gst_element_send_event(data->video_sink, gst_event_new_step(GST_FORMAT_BUFFERS, 1, ABS(data->rate), TRUE,
                             FALSE));
      g_print("Stepping one frame \n");
      break;

    case 'q':
      g_main_loop_quit(data->loop);
      break;

    default:
      break;
  }
  g_free(str);
  return TRUE;
}


int main(int argc, char *argv[])
{
  CustomData_t data;
  GstStateChangeReturn ret;
  GIOChannel *io_stdin;

  gst_init(&argc, &argv);
  memset(&data, 0, sizeof(data));

  g_print ("USAGE: Choose one of the following options, then press enter:\n"
           " 'P' to toggle between PAUSE and PLAY\n"
           " 'S' to increase playback speed, 's' to decrease playback speed\n"
           " 'D' to toggle playback direction\n"
           " 'N' to move to next frame (in the current direction, better in PAUSE)\n"
           " 'Q' to quit\n");

  data.pipeline = gst_parse_launch(
                "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
                NULL);
  io_stdin = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state. \n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  data.playing = TRUE;
  data.rate = 1.0;
  data.loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.loop);

  g_main_loop_unref(data.loop);
  g_io_channel_unref(io_stdin);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  if(data.video_sink != NULL) gst_object_unref(data.video_sink);
  gst_object_unref(data.pipeline);

  return 0;
}
