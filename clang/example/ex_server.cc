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
#include <server/grpc_server.hpp>

#define MAX_PIPELINE 10
#define EXPERIMENT_OPTION 0

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


void create_pipeline(ServiceStreamer_t* service_streamer)
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


void pipeline_serving(ServerBinder* binder)
{
  ServiceStreamer_t streamer;
  while(1) {
    sleep(0.1);
    for(int i = 0; i < binder->service_table.size(); i++) {
      if(binder->service_table[i].status() == NON_ACTIVATED) {
        binder->service_table_mutex.lock();

        if(binder->service_table[i].status() == NON_ACTIVATED) {
          binder->service_table[i].set_status(ACTIVATED);
          binder->service_table_mutex.unlock();

          /* Bind & create streamer... */
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

  /* Setting GRPC */
  string addr("0.0.0.0");
  int grpc_port = 50051;
  int stream_port = 3000;

  ServerBinder server_binder(addr, stream_port);
  ServerBuilder builder;
  builder.AddListeningPort(addr + ":" + to_string(grpc_port),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&server_binder);
  unique_ptr<Server> server = builder.BuildAndStart();

  /* Setting Threads */
  thread timestamp_thread(update_timestamp, &server_binder); // binding thread
  thread pipeline_threads[MAX_PIPELINE];
  for(int i = 0; i < MAX_PIPELINE; i++){
    pipeline_threads[i] = thread(pipeline_serving, &server_binder);
  }

  /* Waiting */
  server->Wait();

  /* Clean up */
  timestamp_thread.join();
  for(int i = 0 ; i < MAX_PIPELINE; i++)
    pipeline_threads[i].join();

  return 0;
}

