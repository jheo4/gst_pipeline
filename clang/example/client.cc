#include <unistd.h>
#include <string>
#include <gst/gst.h>

#include <common/debug.h>
#include <client/pipeline.hpp>

using namespace std;

#define NUM_OF_USERS 5

int main(int argc, char **argv)
{
  gst_init(&argc, &argv);

  int id;
  int port_base = 1234;

  Pipeline pipelines[NUM_OF_USERS];

  for(id = 0; id < NUM_OF_USERS; id++, port_base+=3) {
    pipelines[id].id = id;
    pipelines[id].set_usrsink("avdec_h264");
    pipelines[id].set_source_with_usrsink("avdec_h264", "0.0.0.0", port_base);
    pipelines[id].set_pipeline_run();
  }
  sleep(15);
  return 0;
}

