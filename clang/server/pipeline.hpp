#include <string>
#include <gst/gst.h>
#include <common/debug.h>
#include <common/cb_basic.h>
#include <common/gst_wrapper.h>

#ifndef __PIPELINE__
#define __PIPELINE__

using namespace std;

typedef struct _UsrBin_t
{
  guint id;
  GstElement *bin, *tee;
} UsrBin_t;


typedef struct _SinkBin_t
{
  guint id;
  GstElement *rtp_bin, *rtcp_bin;
  GstElement *rtp_sink;
  GstElement *rtcp_src, *rtcp_sink;
} SinkBin_t;


UsrBin_t* make_usrbin(int id, string codec, string resolution);
SinkBin_t* make_sinkbin(int id, string recv_addr, int port_base);


class Pipeline
{
private:
  bool link_tee2pad(GstElement *tee, GstElement *bin);
  bool set_pipeline_ready();
  bool set_pipeline_run();

public:
  GstElement *pipeline;
  GMainLoop *loop;
  guint id;
  GstElement *src, *src_tee;

  Pipeline(int _id);
  ~Pipeline();

  bool set_source();
  bool register_usrbin(UsrBin_t *_usrbin);
  bool register_sinkbin(SinkBin_t *_sinkbin);
  bool connect_userbin_to_src(UsrBin_t *_usrbin);
  bool connect_sinkbin_to_userbin(SinkBin_t *_sink_bin, UsrBin_t *_usrbin);
};

#endif

