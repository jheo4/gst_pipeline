#include <unistd.h>
#include <string>
#include <gst/gst.h>

#include <common/debug.h>
#include <client/pipeline.hpp>

using namespace std;

int main(int argc, char **argv)
{
  gst_init(&argc, &argv);

  int pipe_id = 0;
  Pipeline pipeline(pipe_id);

  pipeline.set_usrsink("avdec_h264");
  pipeline.set_source_with_usrsink("avdec_h264", "0.0.0.0", 1234);
  pipeline.export_diagram();
  pipeline.set_pipeline_run();

  return 0;
}

