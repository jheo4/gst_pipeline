#include <stdio.h>
#include <gst/gst.h>
#include <gst_common.h>

#ifndef __GST_WRAPPER__
#define __GST_WRAPPER__


static gboolean gst_element_factory_make_wrapper(GstElement** element, const gchar *factoryname, const gchar *name)
{
  *element = gst_element_factory_make(factoryname, name);
  if(!(*element)) {
    g_printerr("The element of %s could not be created. \n", factoryname);
    return FALSE;
  }
  return TRUE;
}


static gboolean gst_element_link_wrapper(GstElement *src, GstElement *dest)
{
  if(!gst_element_link(src, dest)) {
    g_printerr("elements (%s-->%s) could not be linked \n",
               gst_element_get_name(src), gst_element_get_name(dest));

    print_element_pad_caps(src, "src");
    print_element_pad_caps(dest, "sink");
    return FALSE;
  }

  return TRUE;
}
#endif

