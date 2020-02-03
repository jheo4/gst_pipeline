#include <grpc_client.hpp>

GrpcClient::GrpcClient(shared_ptr<Channel> server_channel) :
  binder_stub_(Binder::NewStub(server_channel)) {}


void GrpcClient::bind_request(int stream_id, string codec, string resolution)
{
  ClientContext context;
  RequestInfo request_info;
  request_info.set_client_ip("0.0.0.0");
  request_info.set_stream_id(stream_id);
  request_info.set_codec(codec);
  request_info.set_resolution(resolution);

  DEBUG("send bind request!!!");
  Status s = binder_stub_->bind(&context, request_info, &bind_info);
  if(s.ok())
    DEBUG("bound server : %s:%d", bind_info.server_ip().c_str(), bind_info.port());
    /*
     *  if bind_info.server_ip == FAIL: bind fail..
     *    print bind_info.status_message
     */
  else
    DEBUG("No Bind Server...");
}
