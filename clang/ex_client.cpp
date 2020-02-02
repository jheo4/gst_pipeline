#include <unistd.h>
#include <debug.h>
#include <viewer.hpp>
#include <gst/gst.h>
#include <grpc_client.hpp>

int main(int argc, char *argv[])
{
  DEBUG("Start");
  gst_init(&argc, &argv);

  // assume known server info...
  auto server_channel = grpc::CreateChannel("0.0.0.0:50051",
      grpc::InsecureChannelCredentials());
  GrpcClient grpc_client(server_channel);

  grpc_client.bind_request("0.0.0.0");
  DEBUG("bind test %s:%d", grpc_client.bind_info.server_ip().c_str(), grpc_client.bind_info.port());

  sleep(2);
  Viewer viewer;
  DEBUG("Set Video Session");
  viewer.v_session = viewer.make_video_session(0);

  DEBUG("Set RTP with Session");
  viewer.setup_rtp_receiver_with_stream_session(viewer.v_session,
                                                grpc_client.bind_info.server_ip().c_str(),
                                                grpc_client.bind_info.port());
  DEBUG("Run");
  viewer.run();

  DEBUG("End");
  return 0;
}

