/usr/bin/ffmpeg \
    -protocol_whitelist file,udp,rtp,crypto \
    -re \
    -i test_2.sdp \
    -vcodec copy -acodec copy \
    -f flv 'rtmp://gpu3.view.me/live/test890';