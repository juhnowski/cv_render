Before build:
install opencv, ffmpeg, qt5
sudo apt-get install qtbase5-dev
sudo apt-get install qtdeclarative5-dev

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
1. Start test stream:
```
scripts/local_rtp.sh
```
1. Run program with parameters:
```
./simple_opencv_streaming 1234 1235 rtmp://gpu3.view.me/live/test890
```