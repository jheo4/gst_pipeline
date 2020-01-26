#include <iostream>

#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <binder.grpc.pb.h>
#include <debug.h>

#ifndef __GRPC_CLIENT__
#define __GRPC_CLIENT__
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using binder::Binder;
using binder::IP;
using binder::BindInfo;

using namespace std;

class GrpcClient
{
private:
  unique_ptr<Binder::Stub> binder_stub_;

public:
  BindInfo bind_info;
  GrpcClient(shared_ptr<Channel> server_channel);
  void bind_request(string addr);
};
#endif
