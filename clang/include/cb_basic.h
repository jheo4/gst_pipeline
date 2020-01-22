#include <gst/gst.h>
#include <gst_common.hpp>

#ifndef __CB_BASIC__
#define __CB_BASIC__

static void cb_eos(GstBus *bus, GstMessage *message, GstCommonData_t* data)
{
  g_print("End of Stream \n");
  g_main_loop_quit(data->loop);
}


static void cb_state(GstBus *bus, GstMessage *message, GstCommonData_t* data)
{
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
  if(message->src == (GstObject*)data->pipeline)
    g_print("Pipeline %s changed state from %s to %s \n", GST_OBJECT_NAME(message->src),
            gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
}


static void cb_warning(GstBus *bus, GstMessage *message, GstCommonData_t data)
{
  GError *warning = NULL;

  gst_message_parse_warning(message, &warning, NULL);
  g_printerr("Got warning from %s: %s \n", GST_OBJECT_NAME(message->src), warning->message);
  g_error_free(warning);
}


static void cb_error(GstBus *bus, GstMessage *message, GstCommonData_t* data)
{
  GError *error = NULL;

  gst_message_parse_error(message, &error, NULL);
  g_printerr("Got error from %s: %s \n", GST_OBJECT_NAME(message->src), error->message);
  g_error_free(error);

  g_main_loop_quit(data->loop);
}


static gboolean connect_basic_signals(GstBus *bus, GstCommonData_t* data)
{
  if(!g_signal_connect(bus, "message::error", G_CALLBACK(cb_error), data) ||
     !g_signal_connect(bus, "message::warning", G_CALLBACK(cb_warning), data) ||
     !g_signal_connect(bus, "message::state-changed", G_CALLBACK(cb_state), &data) ||
     !g_signal_connect(bus, "message::eos", G_CALLBACK(cb_eos), &data)) {
    g_printerr("Failure to connect basic signals(error, warning, state-changed, eos) to the bus");
    return FALSE;
  }

  gst_bus_add_signal_watch(bus);
  return TRUE;
}
#endif

