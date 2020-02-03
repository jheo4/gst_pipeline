#include <stdio.h>
#include <gst/gst.h>
#include <gst_common.hpp>

#ifndef __GST_WRAPPER__
#define __GST_WRAPPER__

/*
 * ============================ Wrapper Lists =================================
 * gboolean gst_element_factory_make_wrapper(GstElement** element,
 *                                const gchar *factoryname, const gchar *name)
 * gboolean gst_element_link_wrapper(GstElement *src, GstElement *dest)
 * void setup_ghost_src(GstElement *src, GstBin *bin)
 * void setup_ghost_tee_src(GstElement* tee, GstBin* bin)
 * void setup_ghost_sink(GstElement *sink, GstBin *bin)
 * ============================================================================
 */

gboolean gst_element_factory_make_wrapper(GstElement** element,
                                  const gchar *factoryname, const gchar *name)
{
  *element = gst_element_factory_make(factoryname, name);
  if(!(*element)) {
    g_printerr("The element of %s could not be created. \n", factoryname);
    return FALSE;
  }
  return TRUE;
}


gboolean gst_element_link_wrapper(GstElement* src, GstElement* dest)
{
  if(!gst_element_link(src, dest)) {
    DEBUG("elements (%s-->%s) could not be linked",
          gst_element_get_name(src), gst_element_get_name(dest));
    return FALSE;
  }

  return TRUE;
}


gboolean gst_pad_link_wrapper(GstPad* src_pad, GstPad* dest_pad)
{
  if(gst_pad_link(src_pad, dest_pad) != GST_PAD_LINK_OK) {
    DEBUG("pads (%s-->%s) could not be linked", gst_pad_get_name(src_pad),
          gst_pad_get_name(dest_pad));
    return FALSE;
  }
  return TRUE;
}


// Create a new ghost pad to the element of the bin & add the ghost pad to the bin
void setup_ghost_src(GstElement *src, GstBin *bin)
{
  GstPad *src_pad = gst_element_get_static_pad(src, "src");
  GstPad *bin_pad = gst_ghost_pad_new("src", src_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
}


void setup_ghost_tee_src(GstElement* tee, GstBin* bin)
{
  GstPad* tee_pad = gst_element_get_request_pad(tee, "src_%u");
  GstPad* bin_pad = gst_ghost_pad_new("tee_src", tee_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
}


void setup_ghost_sink(GstElement *sink, GstBin *bin)
{
  GstPad *sink_pad = gst_element_get_static_pad(sink, "sink");
  GstPad *bin_pad = gst_ghost_pad_new("sink", sink_pad);
  gst_element_add_pad(GST_ELEMENT(bin), bin_pad);
}
#endif

