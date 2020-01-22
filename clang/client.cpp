#include <gst/gst.h>
#include <debug.h>
#include <viewer.hpp>

int main(int argc, char *argv[])
{
  DEBUG("Start");
  gst_init(&argc, &argv);

  Viewer viewer;
  DEBUG("Set Video Session");
  viewer.v_session = viewer.make_video_session(0);

  DEBUG("Set RTP with Session");
  viewer.setup_rtp_receiver_with_stream_session(viewer.v_session, "127.0.0.1",
                                                3000);
  DEBUG("Run");
  viewer.run();

  DEBUG("End");
  return 0;
}

