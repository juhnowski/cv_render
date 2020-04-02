Before build:
install opencv, ffmpeg

To build Release:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

To build Debug:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

Run
1. Download some *.mp4 file to scripts
1. Start test stream:
    ```
    scripts/local_rtp.sh
    ```
1. Run program with parameters:
    ```
    ./simple_opencv_streaming 50051 50041 rtmp://gpu3.view.me/live/test890
    ```
   where:
    - 50051 - audio port
    - 50041 - video port