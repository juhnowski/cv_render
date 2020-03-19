ffmpeg \
    -re \
    -i ../film.mp4 \
    -an \
    -c:v copy \
    -f rtp \
    "rtp://127.0.0.1:1234"