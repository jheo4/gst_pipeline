GST_DEBUG=GST_SCHEDULING:6 ./example/client 1234 2>&1 | grep "calling chainfunction" | egrep "\<(element1name|video_sink):sink\>" > log.txt


grep "*" log.txt |sed "s/ .*</ /"|sed "s/:sink.*$//"
