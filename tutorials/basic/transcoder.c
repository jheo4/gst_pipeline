#include <gst/gst.h>

// a structure for all the pipeline elements
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *videoConvert;
  GstElement *audioConvert;
  GstElement *scale;
  GstElement *filter;
  GstCaps *scale_caps;
  GstElement *v_encoder, *v_decoder;
  GstElement *audioSink;
  GstElement *videoSink;
} CustomData_t;


// handler for the pad-added signal
//  Pad: the ports through which elements communicate with each other
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData_t *data);


int main(int argc, char *argv[])
{
  CustomData_t data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean isTerminated = FALSE;

  gst_init(&argc, &argv);

  // uridecodebin: turn URI into raw audio/video streams
  data.source = gst_element_factory_make("uridecodebin", "source");
  data.audioConvert = gst_element_factory_make("audioconvert", "audioconvert");
  data.videoConvert = gst_element_factory_make("videoconvert", "videoconvert");
  data.scale = gst_element_factory_make("videoscale", "scaler");
  data.filter = gst_element_factory_make("capsfilter", "filter");
  data.v_encoder = gst_element_factory_make("x264enc", "videoencoder");
  data.v_decoder = gst_element_factory_make("avdec_h264", "videodecoder");
  data.audioSink = gst_element_factory_make("autoaudiosink", "audiosink");
  data.videoSink = gst_element_factory_make("autovideosink", "videosink");
  data.pipeline = gst_pipeline_new("test_pipeline");

  if(!data.pipeline || !data.audioConvert || !data.audioSink || !data.videoSink || !data.source || !data.v_encoder ||
     !data.v_decoder || !data.scale || !data.filter) {
    g_printerr("Not all elements could be created. \n");
    return -1;
  }

  // link the converter to the sink; not with the source; the source contains no source pads
  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.audioConvert,
                   data.audioSink, data.scale, data.filter, data.v_encoder, data.v_decoder,
                   data.videoConvert, data.videoSink, NULL);

  if(!gst_element_link(data.audioConvert, data.audioSink)) {
    g_printerr("Elements could not be linked. \n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  // scale - encoder - decoder - videoConvert - videoSink
  if(!gst_element_link(data.scale, data.filter) ||
     !gst_element_link(data.filter, data.v_encoder) ||
     !gst_element_link(data.v_encoder, data.v_decoder) ||
     !gst_element_link(data.v_decoder, data.videoConvert) ||
     !gst_element_link(data.videoConvert, data.videoSink)) {
    g_printerr("Elements could not be linked. \n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  g_object_set(data.source, "uri",
               "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
  g_object_set(data.v_encoder, "pass", 5, "quantizer", 25, "speed-preset", 6,  NULL);

  // gst-launch-1.0 uridecodebin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
  //   ! videoscale ! video/x-raw,format=I420,width=178,height=100,framerate=0/1 ! videoconvert ! autovideosink
  /*data.scale_caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "width", G_TYPE_INT, 178,
                                        "height", G_TYPE_INT, 100,
                                        "framerate", GST_TYPE_FRACTION, 0, 1,
                                        NULL);*/
  data.scale_caps = gst_caps_from_string("video/x-raw,format=I420,width=178,height=100,framerate=0/1");

  g_object_set(G_OBJECT(data.filter), "caps", data.scale_caps, NULL);
  gst_caps_unref(data.scale_caps);

  // GSignals: allow you to be notified via callbacks when the signal arrives.
  // The source element will trigger the pad-added signal when it has enough info to start producing data
  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)data.pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "test");

  if(ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state. \n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  bus = gst_element_get_bus(data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if(msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch(GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          g_printerr("Error from element %s: %s \n", GST_OBJECT_NAME(msg->src), err->message);
          g_printerr("Debugging info: %s \n", debug_info ? debug_info : "none");
          g_clear_error(&err);
          g_free(debug_info);
          isTerminated = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print("End-of-Stream reached. \n");
          isTerminated = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          // GStreamer States: NULL-->READY-->PAUSED-->PLAYING (only through intermediate states)
          // filtering the bus messages regarding state changes
          if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
            GstState oldState, newState, pendingState;
            gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
            g_print("Pipeline state changed from %s to %s \n",
                    gst_element_state_get_name(oldState),
                    gst_element_state_get_name(newState));
          }
          break;
        default:
          g_printerr("Unexpected message received. \n");
          break;
      }
      gst_message_unref(msg);
    }
  } while(!isTerminated);

  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
  return 0;
}


// src (the source of the signal), newPad (the added pad by src)
static void pad_added_handler(GstElement *src, GstPad *newPad, CustomData_t *data)
{
  // pad can have many capabilities (queried by gst_pad_query_caps); GstCaps can contain many GstStructure.
  GstCaps *newPadCaps = gst_pad_get_current_caps(newPad); // current capabilities of the pad
  GstStructure *newPadStruct = gst_caps_get_structure(newPadCaps, 0);
  const gchar *newPadType = gst_structure_get_name(newPadStruct);

  GstPadLinkReturn ret;
  GstPad *audioSinkPad = NULL;
  GstPad *videoSinkPad = NULL;

  g_print("Received new pad '%s' from '%s': \n",
          GST_PAD_NAME(newPad), GST_ELEMENT_NAME(src));

  // for the source, the convert is the sink. (src --> convert --> sink)
  if(g_str_has_prefix(newPadType, "audio/x-raw")) {
    GstPad *audioSinkPad = gst_element_get_static_pad(data->audioConvert, "sink");
    if(gst_pad_is_linked(audioSinkPad)) {
      g_print("We are already linked. Ignoring. \n");
      goto exit;
    }

    ret = gst_pad_link(newPad, audioSinkPad);
  }
  else if(g_str_has_prefix(newPadType, "video/x-raw")) {
    GstPad *videoSinkPad = gst_element_get_static_pad(data->scale, "sink");
    if(gst_pad_is_linked(videoSinkPad)) {
      g_print("We are already linked. Ignoring. \n");
      goto exit;
    }

    ret = gst_pad_link(newPad, videoSinkPad);
  }
  else {
    g_print("It has type %s which is not raw audio or raw video. Ignoring. \n", newPadType);
    goto exit;
  }

  if(GST_PAD_LINK_FAILED(ret))
    g_print("Type is %s but link failed. \n", newPadType);
  else
    g_print("Link succeeded (type: %s). \n", newPadType);

exit:
  if(newPadCaps != NULL) gst_caps_unref(newPadCaps);
  if(audioSinkPad != NULL) gst_object_unref(audioSinkPad);
  if(videoSinkPad != NULL) gst_object_unref(videoSinkPad);
}

