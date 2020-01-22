#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <aux.h>
#include <cb_basic.h>
#include <debug.h>
#include <gst_common.hpp>
#include <gst_wrapper.h>
#include <streamer.hpp>


// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! videoconvert !
//                x264enc ! avdec_h264 ! autovideosink
// gst-launch-1.0 filesrc location=4k.mp4 ! decodebin ! audioconvert !
//                avenc_ac3 ! avdec_ac3 ! autoaudiosink
int main(int argc, char **argv)
{
  DEBUG("Start");
  gst_init(&argc, &argv);

  Streamer streamer;

  DEBUG("Set Video Session * Connect it to video src");
  streamer.v_session = streamer.make_video_session(0);
  gst_element_link_filtered(streamer.test_src, streamer.v_session->bin,
                            streamer.v_session->v_caps);

  DEBUG("Set RTP with Session");
  streamer.setup_rtp_sender_with_stream_session(streamer.v_session,
                                                "127.0.0.1", 3000);

  //DEBUG("add pad-added handler");
  //g_signal_connect(creator_data.decodebin, "pad-added",
  //                 G_CALLBACK(file_pad_added_handler), &creator_data);

  DEBUG("Run");
  streamer.run();

  DEBUG("End");
  return 0;
}

