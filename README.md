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