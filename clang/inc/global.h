#include <gst/gst.h>

#ifndef __GLOBAL__
#define __GLOBAL__

typedef struct _CustomData_t
{
  GstPipeline *pipeline;
  GstElement *rtp_bin;
  GMainLoop *loop;
} CustomData_t;

#endif
