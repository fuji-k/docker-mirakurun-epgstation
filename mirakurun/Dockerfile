#FROM wiz98/ubu_tz:latest
FROM node:18-buster-slim
EXPOSE 40772
ENV DOCKER="YES"
ENV DEV="libpcsclite-dev build-essential curl libssl-dev yasm libtool autoconf pkg-config gcc git g++ make automake"

RUN apt-get update && \
    apt-get -y install pcscd libpcsclite1 libccid pcsc-tools libboost-all-dev libboost-thread-dev libboost-filesystem-dev libglib2.0-dev libusb-dev psmisc dvb-tools && \
    apt-get -y install $DEV && \
\
# libaribb25
\
    git clone https://github.com/epgdatacapbon/libaribb25 /usr/local/libaribb25 && cp -rp /usr/local/libaribb25 /tmp/ && \
    cd /tmp/libaribb25 && \
    make clean && \
    make && \
    make install && \
    ldconfig && \
\
# arib-b25-stream.sh
    echo '#!/bin/sh\n\
sh -c cat | b25 -v 0 /dev/stdin /dev/stdout | cat' > /usr/local/bin/arib-b25-stream.sh && \
    chmod +x /usr/local/bin/arib-b25-stream.sh && \
\
# recpt1
\
    git clone https://github.com/stz2012/recpt1 /tmp/recpt1 && \
    cd /tmp/recpt1/recpt1 && \
    sed -i -e 's/arib25/aribb25/g' configure.ac && \
    sed -i -e 's/arib25/aribb25/g' -e 's/HAVE_LIBARIB25/HAVE_LIBARIBB25/g' -e '/#include <aribb25\/arib_std_b25.h>/a#include <aribb25\/arib_std_b25_error_code.h>' decoder.h && \
    sed -i -e 's/HAVE_LIBARIB25/HAVE_LIBARIBB25/g' decoder.c && \
    sed -i -e 's/HAVE_LIBARIB25/HAVE_LIBARIBB25/g' recpt1.c && \
    ./autogen.sh && \
    ./configure --enable-b25 && \
    make && \
    make install && \
\
# pm2
\
    npm install pm2 -g && \
\
# arib-b25-stream-test
\
    npm install arib-b25-stream-test -g --unsafe && \
\
# install Mirakurun
\
    cd /usr/local/ && \
    git clone https://github.com/Chinachu/Mirakurun && \
    cd Mirakurun && \
    npm install && \
    npm run build && \
    npm install -g --unsafe-perm --production && \
\
# 不要なパッケージを削除
\
    apt-get -y remove $DEV && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf /tmp/libaribb25 && \
    rm -rf /tmp/recpt1

WORKDIR /usr/local/lib/node_modules/mirakurun

ENTRYPOINT /etc/init.d/pcscd start && \
    npm start
