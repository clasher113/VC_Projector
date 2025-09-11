## Build project in Linux

### Install libraries
#### Debian based distros
```sh
sudo apt install libsfml-dev libudev-dev libopenal-dev libvorbis-dev libflac-dev libxrandr-dev libxcursor-dev libfreetype6-dev libgif-dev
```

#### RHEL based distros
```sh
sudo dnf install SFML-devel systemd-devel giflib-devel
```

### Building project with CMake
```sh
git clone https://github.com/clasher113/VC_Projector.git -b server-app
cd VC_Projector
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```