## Build project in Linux

### Install libraries
#### Debian based distros
```sh
sudo apt install libxi-dev libxrandr-dev libxcursor-dev libudev-dev libgl1-mesa-dev libfreetype-dev
```

#### RHEL based distros
```sh
sudo dnf install libXi-devel libXrandr-devel libXcursor-devel systemd-devel mesa-libGL-devel freetype-devel
```

### Building project with CMake
```sh
git clone --recurse-submodules https://github.com/clasher113/VC_Projector.git -b server-app
cd VC_Projector
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```