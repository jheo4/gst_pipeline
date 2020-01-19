
Audio Server
gst-launch-1.0 -v rtpbin name=rtpbin \
	audiotestsrc ! audioconvert ! audioresample ! avenc_ac3 ! rtpac3pay ! rtpbin.send_rtp_sink_0  \
	rtpbin.send_rtp_src_0 ! udpsink port=5002 host=127.0.0.1                      \
	rtpbin.send_rtcp_src_0 ! udpsink port=5003 host=127.0.0.1 sync=false async=false \
	udpsrc port=5007 ! rtpbin.recv_rtcp_sink_0


Audio Client
gst-launch-1.0 -v rtpbin name=rtpbin \
	udpsrc caps="application/x-rtp" port=5002 ! rtpbin.recv_rtp_sink_0 \
	rtpbin. ! rtpac3depay ! a52dec ! audioconvert ! audioresample ! autoaudiosink \
	udpsrc port=5003 ! rtpbin.recv_rtcp_sink_0 \
	rtpbin.send_rtcp_src_0 ! udpsink port=5007 host=127.0.0.1 sync=false async=false



Video Server
gst-launch-1.0 -v rtpbin name=rtpbin \
	videotestsrc ! videoconvert ! x264enc ! rtph264pay ! rtpbin.send_rtp_sink_0  \
	rtpbin.send_rtp_src_0 ! udpsink port=5002 host=127.0.0.1                      \
	rtpbin.send_rtcp_src_0 ! udpsink port=5003 host=127.0.0.1 sync=false async=false \
	udpsrc port=5007 ! rtpbin.recv_rtcp_sink_0


Video Client
gst-launch-1.0 -v rtpbin name=rtpbin \
	udpsrc caps="application/x-rtp" port=5002 ! rtpbin.recv_rtp_sink_0 \
	rtpbin. ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink \
	udpsrc port=5003 ! rtpbin.recv_rtcp_sink_0 \
	rtpbin.send_rtcp_src_0 ! udpsink port=5007 host=127.0.0.1 sync=false async=false

