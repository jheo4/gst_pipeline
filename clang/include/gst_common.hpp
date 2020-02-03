#include <gst/gst.h>
#include <gst/rtp/rtp.h>
#include <debug.h>

#ifndef __GST_COMMON__
#define __GST_COMMON__

typedef struct _GstCommonData_t
{
  GstPipeline* pipeline;
  GMainLoop* loop;
} GstCommonData_t;


gboolean print_struct_field(GQuark field, const GValue* value,
    gpointer data)
{
  gchar* str = gst_value_serialize(value);
  DEBUG("\t%15s: %s\n", g_quark_to_string(field), str);
  g_free(str);
  return TRUE;
}


class GstCommon
{
public:
  GstCommonData_t* make_common_data()
  {
    GstCommonData_t *data = g_new0(GstCommonData_t, 1);

    DEBUG("create common data");
    data->pipeline = GST_PIPELINE(gst_pipeline_new(NULL));
    data->loop = g_main_loop_new(NULL, FALSE);

    if(!data->pipeline || !data->loop) {
      g_printerr("Failure to initialize gst common data. \n");
      return NULL;
    }

    // RTP reference: https://bit.ly/30bBOzg

    return data;
  }


  void print_element_pad_caps(GstElement* elem, const gchar* pad_name)
  {
    GstPad* elem_pad = gst_element_get_static_pad(elem, pad_name);
    GstCaps* pad_caps = gst_pad_get_current_caps(elem_pad);

    for(guint i = 0; i < gst_caps_get_size(pad_caps); i++) {
      GstStructure* caps_struct = gst_caps_get_structure(pad_caps, i);
      DEBUG("Caps name: %s\n", gst_structure_get_name(caps_struct));
      gst_structure_foreach(caps_struct, print_struct_field, NULL);
    }
  }

};


#endif

