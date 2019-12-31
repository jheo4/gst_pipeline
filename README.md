# gstreamer

## Install Meson
Install Dependencies & Meson
```
# Dependencies
sudo apt-get install python3 python3-pip python3-setuptools \
                       python3-wheel ninja-build

# Meson
sudo apt install meson
```

## Install GStreamer
Install Dependencies
```
sudo apt-get install -y \
  build-essential dpkg-dev flex bison autotools-dev automake liborc-dev \
  autopoint libtool gtk-doc-tools libgstreamer1.0-dev libxv-dev libasound2-dev \
  libtheora-dev libogg-dev libvorbis-dev libbz2-dev libv4l-dev libvpx-dev \
  libjack-dev libsoup2.4-dev libpulse-dev faad libfaad-dev libfaac-dev \
  libgl1-mesa-dev libgles2-mesa-dev libopencv-dev libx264-dev libmad0-dev \
  libxv-dev libgtk-3-dev yasm nasm
```

Install GStreamer Ubuntu/Debian
```
sudo apt-get install -y \
  libgstreamer1.0-0 libgstreamer-plugins-*-dev libgstrtspserver-1.0-dev\
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x  gstreamer1.0-plugins-base-apps \
  gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```
