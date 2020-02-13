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

#include <aux.h>
#include <cb_basic.h>
#include <debug.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <streamer.hpp>
#include <grpc_server.hpp>

#define MAX_PIPELINE_THREAD 10
#define EXPERIMENT_OPTION 0
/*
 * 0: naive option, every time just create a new streamer
 * 1: source share option, just add a new user-specific bin to the common src
 * 2: user-specific share option: this should be very smart...
 */

// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! videoconvert !
//                x264enc ! avdec_h264 ! autovideosink
// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! audioconvert !
//                avenc_ac3 ! avdec_ac3 ! autoaudiosink

using namespace std;

typedef struct _ServiceStreamer_t
{
  Streamer* streamer;
  ServiceInfo* info;
} ServiceStreamer_t;


void create_streamer(ServiceStreamer_t* service_streamer)
{
  DEBUG("info client %s", service_streamer->info->client_ip().c_str());
  DEBUG("info server %s", service_streamer->info->server_ip().c_str());
  DEBUG("info port %d", service_streamer->info->port());

  // check table
  Streamer streamer;
  service_streamer->streamer = &streamer;
  DEBUG("Set Video Session * Connect it to video src");
  streamer.v_session = streamer.make_video_session(0);

  gst_element_link_filtered(streamer.test_src,
                            streamer.v_session->bin,
                            streamer.v_session->v_caps);

  DEBUG("Set RTP with Session");
  streamer.setup_rtp_sender_with_stream_session(streamer.v_session,
                                 service_streamer->info->client_ip().c_str(),
                                 (int)service_streamer->info->port());

  DEBUG("Run");
  streamer.run();

  DEBUG("End");
}


void run_streamer(ServerBinder* binder)
{
  ServiceStreamer_t streamer;
  while(1) {
    sleep(1);
    for(int i = 0; i < binder->service_table.size(); i++) {
      if(binder->service_table[i].status() == NON_ACTIVATED) {
        DEBUG("try to get the lock, there is a non-activated bind request");
        binder->service_table_mutex.lock();

        if(binder->service_table[i].status() == NON_ACTIVATED) {
          DEBUG("still non-actiavte after getting the lock... let's activate");
          binder->service_table[i].set_status(ACTIVATED);
          binder->service_table_mutex.unlock();
          DEBUG("new bound!!! and let's activate...");
          // streamer_info vector
          streamer.info = &binder->service_table[i];
          create_streamer(&streamer);
        }

        binder->service_table_mutex.unlock();
      }
    }
  }
}


void update_timestamp(ServerBinder* binder)
{
  while(1) {
    sleep(1);
    binder->update_timestamp();
  }
}



int main(int argc, char **argv)
{
  DEBUG("Start");

  // gst init...
  gst_init(&argc, &argv);

  // protobuf init...
  string addr("0.0.0.0");
  int grpc_port = 50051;
  int stream_port = 3000;

  ServerBinder server_binder(addr, stream_port);
  ServerBuilder builder;
  builder.AddListeningPort(addr + ":" + to_string(grpc_port),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&server_binder);
  unique_ptr<Server> server = builder.BuildAndStart();

  thread timestamp_thread(update_timestamp, &server_binder);
  thread pipeline_threads[MAX_PIPELINE_THREAD];
  for(int i = 0; i < MAX_PIPELINE_THREAD; i++){
    pipeline_threads[i] = thread(run_streamer, &server_binder);
  }

  server->Wait();

  timestamp_thread.join();
  for(int i = 0 ; i < MAX_PIPELINE_THREAD; i++)
    pipeline_threads[i].join();

  return 0;
}

