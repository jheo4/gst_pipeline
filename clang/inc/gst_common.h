#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <debug.h>

#ifndef __GST_COMMON__
#define __GST_COMMON__

typedef struct _GstCommonData_t
{
  GstPipeline* pipeline;
  GMainLoop* loop;
  GstElement* rtp_bin;
} GstCommonData_t;


static GstCommonData_t* make_common_data()
{
  DEBUG("Start");
  GstCommonData_t *data = g_new0(GstCommonData_t, 1);

  DEBUG("create common data");
  data->pipeline = GST_PIPELINE(gst_pipeline_new(NULL));
  data->loop = g_main_loop_new(NULL, FALSE);
  data->rtp_bin = gst_element_factory_make("rtpbin", NULL);

  if(!data->pipeline || !data->loop || !data->rtp_bin) {
    g_printerr("Failure to initialize gst common data. \n");
    return NULL;
  }

  // RTP reference: https://bit.ly/30bBOzg
  DEBUG("set common data");
  g_object_set(data->rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  gst_bin_add(GST_BIN(data->pipeline), data->rtp_bin);

  DEBUG("End");
  return data;
}


static gboolean print_struct_field(GQuark field, const GValue* value,
                                   gpointer data)
{
  gchar* str = gst_value_serialize(value);
  DEBUG("\t%15s: %s\n", g_quark_to_string(field), str);
  g_free(str);
  return TRUE;
}


static void print_element_pad_caps(GstElement* elem, const gchar* pad_name)
{
  GstPad* elem_pad = gst_element_get_static_pad(elem, pad_name);
  GstCaps* pad_caps = gst_pad_get_current_caps(elem_pad);

  for(guint i = 0; i < gst_caps_get_size(pad_caps); i++) {
    GstStructure* caps_struct = gst_caps_get_structure(pad_caps, i);
    DEBUG("Caps name: %s\n", gst_structure_get_name(caps_struct));
    gst_structure_foreach(caps_struct, print_struct_field, NULL);
  }
}

#endif

