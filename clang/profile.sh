GST_DEBUG=GST_SCHEDULING:6 ./example/server 2>&1 | grep "calling chainfunction" | egrep "\<(rtp_sink_0|rtp_sink_1|rtp_sink_2|rtp_sink_3|rtp_sink_4):sink\>" > server.csv
grep "." server.csv |sed "s/ .*</ /"|sed "s/:sink.*$//" > server_pip.csv

GST_DEBUG=GST_SCHEDULING:6 ./example/client 2>&1 | grep "calling chainfunction" | egrep "\<(videosink0|videosink1|videosink2|videosink3|videosink4):sink\>" > client.csv
grep "." client.csv |sed "s/ .*</ /"|sed "s/:sink.*$//" > client_pip.csv


==========================

# Server
GST_DEBUG=GST_SCHEDULING:6 ./example/server 2>&1 | grep "calling chainfunction" > server.csv
cat server.csv | egrep "\<(rtp_sink_0|rtp_sink_1|rtp_sink_2|rtp_sink_3|rtp_sink_4|rtp_sink_5|rtp_sink_6|rtp_sink_7|rtp_sink_8|rtp_sink_9):sink\>" | grep "." | sed "s/ .*</ /" | sed "s/:sink.*$//" > server_pip.csv

=(A2-A1)*1000
=IF(C1>16, C1, "")
=AVERAGE(D:D)








cat server.csv | egrep "\<(rtp_sink_0|rtp_sink_1|rtp_sink_2|rtp_sink_3|rtp_sink_4):sink\>"
grep "." server.csv |sed "s/ .*</ /"|sed "s/:sink.*$//" > server_pip.csv

# Client
GST_DEBUG=GST_SCHEDULING:6 ./example/client 2>&1 | grep "calling chainfunction" > client.csv
cat client.csv | egrep "\<(funnel0|fakesink0):(sink|funnelpad0)\>" | grep "." | sed "s/ .*</ /" | sed "s/:sink.*$//" > client_pip.csv


cat client.csv | egrep "\<(videosink0|videosink1|videosink2|videosink3|videosink4):sink\>"
grep "." client.csv |sed "s/ .*</ /"|sed "s/:sink.*$//" > client_pip.csv

