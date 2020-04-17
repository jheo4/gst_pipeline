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

#define EXPERIMENT_OPTION 0

// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! videoconvert !
//                x264enc ! avdec_h264 ! autovideosink
// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! audioconvert !
//                avenc_ac3 ! avdec_ac3 ! autoaudiosink

using namespace std;

#define NUM_OF_USERS 5

void run_pipeline(Pipeline *pipeline) {
  pipeline->set_pipeline_run();
  sleep(10);
}

int main(int argc, char **argv)
{
  // gst init...
  gst_init(&argc, &argv);

  int id = 0;
  int port_base = 1234;

  /*
   *  Case 1: 1 User & 1 Pipeline
   *  Case 2: N User & 1 Pipeline (Source sharing)
   *  Case 3: N User & 1 Pipeline (User-specific sharing)
   */
  Pipeline pipelines[NUM_OF_USERS];
  thread pipeline_threads[NUM_OF_USERS];
  UsrBin_t *usrbin[NUM_OF_USERS];
  SinkBin_t *sinkbin[NUM_OF_USERS];
/*
  pipelines[id].id = id;
  pipelines[id].set_source();
  usrbin[id] = make_usrbin(id, "x264enc", "1920", "1080");
  pipelines[id].register_usrbin(usrbin[id]);
  pipelines[id].connect_userbin_to_src(usrbin[id]);
*/
  for(id = 0; id < NUM_OF_USERS; id++) {
    pipelines[id].id = id;
    pipelines[id].set_source();

    usrbin[id] = make_usrbin(id, "x264enc", "1920", "1080");
    pipelines[id].register_usrbin(usrbin[id]);
    pipelines[id].connect_userbin_to_src(usrbin[id]);

    sinkbin[id] = make_sinkbin(id, "x264enc", "0.0.0.0", port_base);
    pipelines[id].register_sinkbin(sinkbin[id]);
    pipelines[id].connect_sinkbin_to_userbin(sinkbin[id], usrbin[id]);

    DEBUG("id/port : %d/%d", id, port_base);
    port_base+=3;

  }
  for(int i = 0; i < NUM_OF_USERS; i++)
    pipeline_threads[i] = thread(run_pipeline, &pipelines[i]);

  pipelines[0].export_diagram();

  for(int i = 0; i < NUM_OF_USERS; i++)
    pipeline_threads[i].join();

  return 0;
}

