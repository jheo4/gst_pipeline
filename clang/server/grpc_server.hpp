#include <iostream>
#include <ctime>
#include <string>
#include <vector>

#include <debug.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <binder.grpc.pb.h>

#ifndef __GRPC_SERVER__
#define __GRPC_SERVER__
#define FAILURE_TIMELIMIT 10
#define NON_ACTIVATED 0
#define TO_ACTIVATE 1
#define ACTIVATED 2

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using binder::Binder;
using binder::IP;
using binder::BindInfo;
using binder::ServiceInfo;
using binder::Empty;

using namespace std;

class ServerBinder final: public Binder::Service
{
private:
  int timestamp;
  int port_base;

public:
  string server_ip;
  std::mutex service_table_mutex;
  vector<ServiceInfo> service_table;

  ServerBinder();
  ServerBinder(string ip, int port);
  void update_timestamp(){ timestamp = time(0); }
  int get_timestamp() { return timestamp; }
  bool check_node_failure();

  // grpc override func
  Status bind(ServerContext* context,
              const IP* in_addr,
              BindInfo* bind_info) override;
  Status send_heartbeat(ServerContext* context,
                        const BindInfo* bind_info,
                        Empty* empty) override;
};

#endif

