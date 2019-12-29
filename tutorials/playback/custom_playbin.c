/*
 * A single file can contain multiple streams; a container can multiplex multiple streams. To select the proper stream,
 * the metadata info is needed. In this example, streams in a file and their metadata are recovered. Switching streams
 * is allowed via a customized playbin.
 */

#include <gst/gst.h>
#include <stdio.h>

typedef struct _CustomData_t
{
  GstElement *playbin;

  gint n_video;
  gint n_audio;
  gint n_text;

  gint cur_video;
  gint cur_audio;
  gint cur_text;

  GMainLoop *main_loop;
} CustomData_t;


// since playbin is a plug-in and not a part of the GStreamer core. So, GstPlayFlags is defined in the codes.
typedef enum
{
  GST_PLAY_FLAG_VIDEO = (1 << 0),    // video output (1)
  GST_PLAY_FLAG_AUDIO = (1 << 1),    // audio output (2)
  GST_PLAY_FLAG_TEXT = (1 << 2)      // text output (4)
} GstPlayFlags;


static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData_t *data);
static gboolean handle_keyboard(GIOChannel *src, GIOCondition cond, CustomData_t *data);
static void analyze_streams(CustomData_t *data);

int main(int argc, char *argv[])
{
  CustomData_t data;
  GstBus *bus;
  GstStateChangeReturn ret;
  gint flags;
  GIOChannel *io_stdin;

  gst_init(&argc, &argv);

  data.playbin = gst_element_factory_make("playbin", "custom_playbin");
  if(!data.playbin) {
    g_printerr("Not all elements could be created. \n");
    return -1;
  }

  g_object_set(data.playbin, "uri",
               "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_cropped_multilingual.webm", NULL);
  g_object_get(data.playbin, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO; // 011 010 001
  flags &= ~GST_PLAY_FLAG_TEXT;
  g_object_set(data.playbin, "flags", flags, NULL);
  g_object_set(data.playbin, "connection-speed", 56, NULL);

  bus = gst_element_get_bus(data.playbin);
  gst_bus_add_watch(bus, (GstBusFunc)handle_message, &data);

  io_stdin = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

  ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state. \n");
    gst_object_unref(data.playbin);
    return -1;
  }

  data.main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.main_loop);

  g_main_loop_unref(data.main_loop);
  g_io_channel_unref(io_stdin);
  gst_object_unref(bus);
  gst_element_set_state(data.playbin, GST_STATE_NULL);
  gst_object_unref(data.playbin);

  return 0;
}


static void analyze_streams(CustomData_t *data)
{
  gint i;
  GstTagList *tags;
  gchar *str;
  guint rate;

  g_object_get(data->playbin, "n-video", &data->n_video, NULL);
  g_object_get(data->playbin, "n-audio", &data->n_audio, NULL);
  g_object_get(data->playbin, "n-text", &data->n_text, NULL);

  g_print("%d video streams, %d audio streams, %d text streams \n\n", data->n_video, data->n_audio, data->n_text);

  for(int i = 0; i < data->n_video; i++) {
    tags = NULL;
    g_signal_emit_by_name(data->playbin, "get-video-tags", i, &tags);
    if(tags) {
      gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &str);

      g_print("video stream %d: \n", i);
      g_print("\tcodec: %s\n", str ? str : "unknown");
      g_free(str);
      gst_tag_list_free(tags);
    }
  }
  g_print("\n");

  for(int i = 0; i < data->n_audio; i++) {
    tags = NULL;
    g_signal_emit_by_name(data->playbin, "get-audio-tags", i, &tags);
    if(tags) {
      g_print("audio stream %d: \n", i);
      if (gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &str)) {
        g_print("\tcodec: %s\n", str ? str : "unknown");
        g_free(str);
      }
      if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
        g_print("\tlanguage: %s\n", str ? str : "unknown");
        g_free(str);
      }
      if (gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &rate)) {
        g_print("\tbitrate: %d\n", rate);
      }
      gst_tag_list_free(tags);
    }
  }
  g_print("\n");

  for(int i = 0; i < data->n_text; i++) {
    tags = NULL;
    g_signal_emit_by_name(data->playbin, "get-text-tags", i, &tags);
    if(tags) {
      g_print("subtitle stream %d: \n", i);

      if(gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
        g_print("\tlanguage: %s\n", str);
        g_free(str);
      }
      gst_tag_list_free(tags);
    }
  }
  g_print("\n");

  g_object_get(data->playbin, "current-video", &data->cur_video, NULL);
  g_object_get(data->playbin, "current-audio", &data->cur_audio, NULL);
  g_object_get(data->playbin, "current-text", &data->cur_text, NULL);

  g_print("Currently playing video stream %d, audio stream %d, and text stream %d \n",
          data->cur_video, data->cur_audio, data->cur_text);
  g_print("Type any number and hit ENTER to select a different audio stream \n");
}


static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData_t *data)
{
  GError *err;
  gchar *debug_info;
  switch(GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      g_main_loop_quit (data->main_loop);
      break;
    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream reached.\n");
      g_main_loop_quit (data->main_loop);
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
      if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin))
        if(new_state == GST_STATE_PLAYING)
          analyze_streams(data);
    } break;
  }

  return TRUE;
}


static gboolean handle_keyboard(GIOChannel *src, GIOCondition cond, CustomData_t *data)
{
	gchar *str = NULL;
	if(g_io_channel_read_line(src, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
    int index = g_ascii_strtoull(str, NULL, 0);
    if(index < 0 || index >= data->n_audio) g_printerr("Index out of bounds \n");
    else {
      g_print("Setting current audio stream to %d \n", index);
      g_object_set(data->playbin, "current-audio", index, NULL);
    }
  }
  g_free(str);
  return TRUE;
}

