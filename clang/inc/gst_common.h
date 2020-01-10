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

  // GstRTPProfile:
  //   GST_RTP_PROFILE_UNKNOWN: invalid profile
  //   GST_RTP_PROFILE_AVP: the Audio/Visual profile (RFC 3551)
  //   GST_RTP_PROFILE_SAVP: the secure Audio/Visual profile (RFC 3711)
  //   GST_RTP_PROFILE_AVPF: the Audio/Visual profile with feedback (RFC 4585)
  //   GST_RTP_PROFILE_SAVPF: the secure Audio/Visual profile with feedback (RFC 5124)
  DEBUG("set common data");
  g_object_set(data->rtp_bin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  gst_bin_add(GST_BIN(data->pipeline), data->rtp_bin);

  DEBUG("End");
  return data;
}
#endif

