#include <unistd.h>
#include <string>
#include <debug.h>
#include <viewer.hpp>
#include <gst/gst.h>
#include <grpc_client.hpp>

using namespace std;

int main(int argc, char **argv)
{
  if(argc < 7) {
    cout << "Usage: " << argv[0] << " -i <IP_Addr>"
      " -s <STREAM_ID>" << " -c <CODEC>" << " -r <RESOLUTION>" << endl;
    return -1;
  }
  string server_ip = argv[2];
  int stream_id = stoi(argv[4]);
  string codec = argv[6];
  string resolution = argv[8];
  DEBUG("ip(%s) stream_id(%d), codec(%s), resolution(%s)",
        server_ip.c_str(), stream_id, codec.c_str(), resolution.c_str());
  DEBUG("Start");
  gst_init(&argc, &argv);

  auto server_channel = grpc::CreateChannel(server_ip + ":50051",
      grpc::InsecureChannelCredentials());
  GrpcClient grpc_client(server_channel);

  // currently, f
  grpc_client.bind_request(stream_id, codec, resolution);
  DEBUG("bind test %s:%d", grpc_client.bind_info.server_ip().c_str(), grpc_client.bind_info.port());

  sleep(2); // no response from server.. just wait temporarily
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

