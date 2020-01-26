#include <grpc_client.hpp>

GrpcClient::GrpcClient(shared_ptr<Channel> server_channel) :
  binder_stub_(Binder::NewStub(server_channel)) { }


void GrpcClient::bind_request(string addr)
{
  ClientContext context;
  IP client_addr;
  client_addr.set_ip(addr);
  DEBUG("send bind request!!!");
  Status s = binder_stub_->bind(&context, client_addr, &bind_info);
  if(s.ok())
    DEBUG("bound server : %s:%d", bind_info.server_ip().c_str(), bind_info.port());
  else
    DEBUG("bind failed...");
}
