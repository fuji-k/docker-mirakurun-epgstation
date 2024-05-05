FROM node:18-buster-slim
EXPOSE 8888
ENV DOCKER="YES"
ENV DEV="autoconf automake build-essential cmake git pkg-config texinfo curl wget python python3 libtool nasm yasm gzip gperf uuid-dev gettext autopoint zip ninja-build unzip"
ENV V_ALSA="1.1.9"

#COPY ./logo.zip /tmp
COPY ./cutass.cpp /tmp

RUN apt-get update && \
    apt-get -y install $DEV && \
    apt-get -y install openssh-client rsync nkf sqlite3 \
    libvorbis-dev v4l-utils libv4l-dev apt-transport-https \
    libmp3lame-dev libopus-dev libvorbis-dev libvpx-dev libx264-dev \
    libass-dev libfreetype6-dev python-pip python3-pip python3-setuptools \
    locales vim p7zip-full mono-complete && \
    echo "ja_JP UTF-8" > /etc/locale.gen && \
    locale-gen ja_JP.UTF-8

ENV TZ=Asia/Tokyo
ENV LANG=ja_JP.UTF-8
ENV LANGUAGE=ja_JP:ja
ENV LC_ALL=ja_JP.UTF-8

RUN mkdir -p ~/ffmpeg_sources && \
    echo "/opt/vc/lib" > /etc/ld.so.conf.d/ffmpeg.conf && \
    echo "/usr/local/lib" >> /etc/ld.so.conf.d/ffmpeg.conf && \
    ldconfig && \
\
# meson and sphinx install
\
    python3 -m pip install meson && \
    python3 -m pip install sphinx && \
    cd /usr/local/src && \
    pip install -e git+https://github.com/johnoneil/arib#egg=arib && \
\
# fdk-aac
\
    cd ~/ffmpeg_sources && \
    git -C fdk-aac pull 2> /dev/null || git clone --depth 1 https://github.com/mstorsjo/fdk-aac && \
    cd fdk-aac && \
    autoreconf -fiv && \
    ./configure \
    --disable-shared && \
    make -j"$(nproc)" && \
    make install && \
\
# libaribb24
\
    cd ~/ffmpeg_sources && \
    git -C aribb24 pull 2> /dev/null || git clone https://github.com/nkoriyama/aribb24 && \
    cd aribb24 && \
    autoreconf -fiv && \
    ./configure \
    --enable-static && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig && \
\
# mmal
\
    cd ~/ffmpeg_sources && \
    git -C userland pull 2> /dev/null || git clone --depth 1 https://github.com/raspberrypi/userland.git && \
    cd userland && \
    sed -i -e "s/sudo make/make/g" buildme && \
    ./buildme && \
\
# ALSA
\
    cd ~/ffmpeg_sources && \
    wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-$V_ALSA.tar.bz2 && \
    tar xjvf alsa-lib-$V_ALSA.tar.bz2 && \
    cd alsa-lib-$V_ALSA && \
    ./configure && \
    make -j"$(nproc)" && \
    make install && \
\
# l-smash 18a9ed25c7ff79a7f4f4bf850c345c72179b8998
\
    cd ~/ffmpeg_sources && \
    git -C l-smash pull 2> /dev/null || git clone https://github.com/l-smash/l-smash.git && \
    cd l-smash && \
    git checkout 18a9ed25c7ff79a7f4f4bf850c345c72179b8998 && \
    ./configure \
    --enable-shared && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig && \
\
# AviSynthPlus 8e7729e22398367c43e865e1ad463ea43eab47b1
\
    cd ~/ffmpeg_sources && \
    git -C AviSynthPlus pull 2> /dev/null || git clone --recursive https://github.com/AviSynth/AviSynthPlus && \
    cd AviSynthPlus && \
    git checkout 8e7729e22398367c43e865e1ad463ea43eab47b1 && \
    cd filesystem && \
    git checkout 3f1c185ab414e764c694b8171d1c4d8c5c437517 && \
    cd ../ && \
    mkdir avisynth-build && \
    cd avisynth-build && \
    cmake -DCMAKE_CXX_FLAGS=-latomic ../ -G Ninja && \
    ninja -j"$(nproc)" && \
    ninja install && \
    ldconfig && \
\
# ffmpeg 04e06beb0ab98a6eb85df32f7809b1143e4bebe7
\
    cd ~/ffmpeg_sources && \
    git -C ffmpeg pull 2> /dev/null || git clone https://github.com/FFmpeg/FFmpeg ffmpeg && \
    cd ffmpeg && \
    git checkout 04e06beb0ab98a6eb85df32f7809b1143e4bebe7 && \
    ./configure \
    --extra-ldflags="-latomic" \
    --extra-cflags="-I/usr/local/include -I/opt/vc/include -I/opt/vc/include/IL" \
    --extra-ldflags="-L/usr/local/lib -L/opt/vc/lib" \
    --arch=armel \
    --target-os=linux \
    --enable-gpl \
    --enable-version3 \
    --disable-doc \
    --disable-debug \
    --enable-pic \
    --enable-avisynth \
    --enable-libaribb24 \
    --enable-libass \
    --enable-libfreetype \
    --enable-libx264 \
    --enable-libfdk-aac \
    --enable-libfreetype \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-nonfree \
    --enable-mmal \
    --enable-omx-rpi \
    --enable-omx \
    --extra-libs=-ldl && \
    make -j"$(nproc)" && \
    make install && \
    hash -r && \
\
# L-SMASH-Works 77595d21d77e31557f159747832b07016e71ec59
\
    cd ~/ffmpeg_sources && \
    git -C L-SMASH-Works pull 2> /dev/null || git clone https://github.com/HolyWu/L-SMASH-Works.git && \
    git -C chapter_exe pull 2> /dev/null || git clone https://github.com/tobitti0/chapter_exe.git -b arm-test && \
    cd L-SMASH-Works && \
    git checkout 77595d21d77e31557f159747832b07016e71ec59 && \
    cd ~/ffmpeg_sources && \
    cp chapter_exe/src/sse2neon.h L-SMASH-Works/AviSynth/emmintrin.h && \
    cd L-SMASH-Works/AviSynth && \
    sed -i.bk -e '42,43d' -e "72aif host_machine.cpu_family().startswith('arm')\n add_project_arguments('-mfpu=neon', language : ['c', 'cpp'])\nendif\n" meson.build && \
    sed -i.bk '52d' video_output.cpp && \
    CC=gcc CXX=gcc LD=gcc LDFLAGS="-Wl,-Bsymbolic,-L/opt/vc/lib" \
    meson build && \
    cd build && \
    ninja -v -j"$(nproc)" && \
    ninja install && \
    ldconfig && \
\
# JoinLogoScpTrialSetLinux c3a454899a656117819f71264ca31067c023b1cb
# chapter_exe d734bf1285671baa311da77bf59cfa9e3a4e127a
# join_logo_scp 466d367468f41d80f6967a0036f05c7e966c426c
# join_logo_scp_trial 88232b3639d22584adc2bfe41cf75b1d91c3bf33
# logoframe ba0d3a245547c8932f0d0e4005852cc24bf78a6e
\
    cd && \
    git -C JoinLogoScpTrialSetLinux pull 2> /dev/null || git clone --recursive https://github.com/tobitti0/JoinLogoScpTrialSetLinux.git && \
    cd JoinLogoScpTrialSetLinux && \
    git checkout c3a454899a656117819f71264ca31067c023b1cb && \
    cd modules/chapter_exe && \
    git checkout d734bf1285671baa311da77bf59cfa9e3a4e127a && \
    cd ../join_logo_scp && \
    git checkout 466d367468f41d80f6967a0036f05c7e966c426c && \
    cd ../join_logo_scp_trial && \
    git checkout 88232b3639d22584adc2bfe41cf75b1d91c3bf33 && \
    cd ../logoframe && \
    git checkout ba0d3a245547c8932f0d0e4005852cc24bf78a6e && \
    cd src && \
    make -j"$(nproc)" && \
    cd && cp JoinLogoScpTrialSetLinux/modules/logoframe/src/logoframe JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/logoframe && \
    cd JoinLogoScpTrialSetLinux/modules/join_logo_scp/src && \
    make -j"$(nproc)" && \
    cd && cp JoinLogoScpTrialSetLinux/modules/join_logo_scp/src/join_logo_scp JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/join_logo_scp && \
    cd JoinLogoScpTrialSetLinux/modules/chapter_exe/src && \
    make -j"$(nproc)" && \
    cd && cp JoinLogoScpTrialSetLinux/modules/chapter_exe/src/chapter_exe JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/chapter_exe && \
    cd JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial && \
    npm install && \
    npm link && \
\
# delogo-AviSynthPlus-Linux e55cfa2067f82ce8755b85d672eeba9c6dde9977
\
    cd ~/ffmpeg_sources && \
    git -C delogo-AviSynthPlus-Linux pull 2> /dev/null || git clone https://github.com/tobitti0/delogo-AviSynthPlus-Linux.git && \
    cd delogo-AviSynthPlus-Linux && \
    git checkout e55cfa2067f82ce8755b85d672eeba9c6dde9977 && \
    cd src && \
    make -j"$(nproc)" && \
    make install && \
    ldconfig && \
\
# install EPGStation
\
    cd /usr/local/ && \
    git -C EPGStation pull 2> /dev/null || git clone https://github.com/l3tnun/EPGStation.git && \
    cd /usr/local/EPGStation && \
    npm run all-install && \
    npm run build && \
\
# cutass build
\
    cd /tmp && \
    g++ -o cutass cutass.cpp && \
    cp cutass /usr/local/bin && \
    chmod a+x /usr/local/bin/cutass

WORKDIR /usr/local/EPGStation

ENTRYPOINT npm start
