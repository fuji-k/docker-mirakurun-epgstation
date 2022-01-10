FROM node:14-buster-slim
EXPOSE 8888
ENV DOCKER="YES"
ENV DEV="autoconf automake build-essential cmake git-core pkg-config texinfo curl wget python python3 unzip libtool nasm yasm gzip gperf uuid-dev gettext autopoint"
ENV V_ALSA="1.1.9"

RUN apt-get update && \
    apt-get -y install $DEV openssh-client rsync nkf sqlite3 \
    libass-dev libvorbis-dev v4l-utils libv4l-dev libjpeg-dev ffmpeg && \
\
# install hardware acceleration ffmpeg
\
    mkdir -p ~/ffmpeg_sources && \
\
# x264
\
    cd ~/ffmpeg_sources && \
    git -C x264 pull 2> /dev/null || git clone --depth 1 https://code.videolan.org/videolan/x264.git && \
    cd x264 && \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --enable-static \
    --enable-pic && \
    make -j"$(nproc)" && \
    make install && \
\
# vpx
\
    cd ~/ffmpeg_sources && \
    git -C libvpx pull 2> /dev/null || git clone --depth 1 https://chromium.googlesource.com/webm/libvpx.git && \
    cd libvpx && \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --disable-examples \
    --disable-unit-tests \
    --enable-vp9-highbitdepth \
    --as=yasm && \
    make -j"$(nproc)" && \
    make install && \
\
# fdk-aac
\
    cd ~/ffmpeg_sources && \
    git -C fdk-aac pull 2> /dev/null || git clone --depth 1 https://github.com/mstorsjo/fdk-aac && \
    cd fdk-aac && \
    autoreconf -fiv && \
    ./configure --prefix="$HOME/ffmpeg_build" --disable-shared && \
    make -j"$(nproc)" && \
    make install && \
\
# lameMP3
\
    cd ~/ffmpeg_sources && \
    wget -O lame-3.100.tar.gz https://jaist.dl.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz && \
    tar xzvf lame-3.100.tar.gz && \
    cd lame-3.100 && \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --disable-shared \
    --enable-static \
    --enable-nasm && \
    make -j"$(nproc)" && \
    make install && \
\
# opus
\
    cd ~/ffmpeg_sources && \
    git -C opus pull 2> /dev/null || git clone --depth 1 https://github.com/xiph/opus.git && \
    cd opus && \
    ./autogen.sh && \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --disable-shared && \
    make -j"$(nproc)" && \
    make install && \
\
# libaribb24
\
    cd ~/ffmpeg_sources && \
    git clone https://github.com/nkoriyama/aribb24 && \
    cd aribb24 && \
    autoreconf -fiv && \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --enable-static --disable-shared && \
    make -j"$(nproc)" && \
    make install && \
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
    ./configure --prefix="$HOME/ffmpeg_build" && \
    make -j"$(nproc)" && \
    make install && \
\
# ffmpeg
\
    echo "/opt/vc/lib" > /etc/ld.so.conf.d/vc.conf && \
    echo "$HOME/ffmpeg_build/lib" > /etc/ld.so.conf.d/ffmpeg.conf && \
    ldconfig && \
    \
    cd ~/ffmpeg_sources && \
    git -C ffmpeg pull 2> /dev/null || git clone --depth 1 https://github.com/FFmpeg/FFmpeg --branch n4.4 ffmpeg && \
    cd ffmpeg && \
    PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig:/opt/vc/lib/pkgconfig" \
    ./configure \
    --prefix="$HOME/ffmpeg_build" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I$HOME/ffmpeg_build/include -I/opt/vc/include -I/opt/vc/include/IL" \
    --extra-ldflags="-L$HOME/ffmpeg_build/lib -L/opt/vc/lib" \
    --extra-libs="-ldl" \
    --enable-gpl \
    --enable-nonfree \
    --enable-version3 \
    --enable-libv4l2 \
    --enable-libaribb24 \
    --enable-libass \
    --enable-libfdk-aac \
    --enable-libfreetype \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-libx264 \
    --enable-mmal \
    --enable-omx-rpi \
    --enable-omx \
    --disable-doc \
    --disable-debug \
    --disable-shared \
    --enable-static \
    --enable-small && \
    make -j"$(nproc)" && \
    make install && \
    hash -r && \
    cp $HOME/ffmpeg_build/bin/ffmpeg /usr/local/bin && \
    cp $HOME/ffmpeg_build/bin/ffprobe /usr/local/bin && \
\
# install EPGStation
\
    cd /usr/local/ && \
    git -C EPGStation pull 2> /dev/null || git clone https://github.com/l3tnun/EPGStation.git -b v2.6.18 && \
    cd /usr/local/EPGStation && \
    npm run all-install && \
    npm run build && \
\
# SQLite regex
\
    wget -O /tmp/sqlite-amalgamation-3270200.zip https://www.sqlite.org/2019/sqlite-amalgamation-3270200.zip && \
    wget -O /tmp/sqlite-src-3270200.zip https://www.sqlite.org/2019/sqlite-src-3270200.zip && \
    unzip /tmp/sqlite-amalgamation-3270200.zip -d /tmp/ && \
    unzip /tmp/sqlite-src-3270200.zip -d /tmp/ && \
    cp /tmp/sqlite-src-3270200/ext/misc/regexp.c /tmp/sqlite-amalgamation-3270200 && \
    cd /tmp/sqlite-amalgamation-3270200 && \
    gcc -g -fPIC -shared regexp.c -o regexp.so && \
    cp regexp.so /usr/local/EPGStation/ && \
    cd /usr/local/EPGStation && \
\
# remove packages
\
    apt-get -y --allow-remove-essential remove $DEV && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /tmp/sqlite-* && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/EPGStation

ENTRYPOINT npm start
