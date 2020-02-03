#include <grpc_server.hpp>
using namespace std;

ServerBinder::ServerBinder()
{
  timestamp = time(0);
  port_base = 3000;
  server_ip = "0.0.0.0"; // TODO
}


ServerBinder::ServerBinder(string ip, int port)
{
  timestamp = time(0);
  port_base = port;
  server_ip = ip;
}


bool ServerBinder::check_node_failure()
{
  lock_guard<mutex> lock(service_table_mutex);
  for(const ServiceInfo& t : service_table) {
    int timediff = timestamp - t.last_seen();
    cout << "\t" << t.port() << ": was last seen " << timediff << " seconds ago" << endl;
    if(timediff > FAILURE_TIMELIMIT) {
      cout << "Node Failure!!!" << endl;
      // TODO
      // remove the stream..
      return true;
    }
  }
  return false;
}


Status ServerBinder::bind(ServerContext* context,
                          const RequestInfo* request_info, BindInfo* bind_info)
{
  // new bind...
  DEBUG("new bind request to server(%s:%d) from client(%s)", server_ip.c_str(),
        port_base, request_info->client_ip());
  bind_info->set_server_ip(server_ip);
  bind_info->set_client_ip(request_info->client_ip());
  bind_info->set_port(port_base);

  ServiceInfo new_service;

  new_service.set_server_ip(server_ip);
  new_service.set_client_ip(request_info->client_ip());
  new_service.set_port(port_base);
  new_service.set_last_seen(timestamp);
  new_service.set_status(NON_ACTIVATED);

  service_table_mutex.lock();
  service_table.push_back(new_service);
  service_table_mutex.unlock();

  port_base += 3;

  return Status::OK;
}


Status ServerBinder::send_heartbeat(ServerContext* context,
                                    const BindInfo* bind_info, Empty* empty)
{
  lock_guard<mutex> lock(service_table_mutex);
  for(const ServiceInfo& t : service_table) {
    if(t.client_ip() == bind_info->client_ip()) {
      t.set_last_seen(timestamp);
      break;
    }
  }

  return Status::OK;
}
