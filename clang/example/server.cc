#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <common/debug.h>
#include <server/pipeline.hpp>
//#include <server/grpc_server.hpp>

#define MAX_PIPELINE 10
#define EXPERIMENT_OPTION 0

// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! videoconvert !
//                x264enc ! avdec_h264 ! autovideosink
// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! audioconvert !
//                avenc_ac3 ! avdec_ac3 ! autoaudiosink

using namespace std;

int main(int argc, char **argv)
{
  // gst init...
  gst_init(&argc, &argv);

  int pipe_id = 0;
  int port_base = 1234;
  DEBUG("Pipeline Setting...");
  Pipeline pipeline(pipe_id);
  pipeline.set_source();

  DEBUG("Userbin, Sinkbin creating...");
  UsrBin_t *usrbin = make_usrbin(pipe_id, "x264enc", "1280", "780");
  SinkBin_t *sinkbin = make_sinkbin(pipe_id, "x264enc", "0.0.0.0", 1234);

  DEBUG("Userbin, Sinkbin registering...");
  pipeline.register_usrbin(usrbin);
  pipeline.register_sinkbin(sinkbin);

  DEBUG("Userbin, Sinkbin connecting...");
  pipeline.connect_userbin_to_src(usrbin);
  pipeline.connect_sinkbin_to_userbin(sinkbin, usrbin);

  pipeline.export_diagram();
  DEBUG("Pipline Run...");
  pipeline.set_pipeline_run();

  return 0;
}

