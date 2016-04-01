#/bin/bash

CWD=`pwd`
mkdir -p /cache

#Update
apt-get update -qq

#Install build tools 
#TODO: install just the required packages
apt-get install -o dir::cache::archives="/cache/apt" -y \
  build-essential \
  git \
  libboost-all-dev && \

#Install FIFu library requirements
apt-get install -o dir::cache::archives="/cache/apt" -y \
  libcrypto++-dev \
  libcurl4-gnutls-dev && \

#Install Click
CLICK_DIR=/cache/click
apt-get install -o dir::cache::archives="/cache/apt" -y \
  tcpdump \
  libpcap-dev \
  time &&\
if [ ! -d "$CLICK_DIR" ]; then
  echo "Cloning click OS repository"
  git clone https://github.com/kohler/click.git $CLICK_DIR && \
  cd $CLICK_DIR && \
  ./configure --disable-linuxmodule --enable-ip6 --enable-json && make && make install #TODO: Check why make check fails
else
  echo "Pulling click OS repository"
  cd $CLICK_DIR && \
  git pull && \
  make && make install
fi
rc=$?
if [ ! $rc -eq 0 ]; then
  exit $rc
fi

#Install Blackadder
BLACKADDER_DIR=/cache/blackadder
apt-get install -o dir::cache::archives="/cache/apt" -y \
  libtool \
  autoconf \
  automake \
  libigraph0 \
  libigraph0-dev \
  libconfig++8 \
  libconfig++8-dev \
  libtclap-dev \
  libboost-graph-dev && \
if [ ! -d "$BLACKADDER_DIR" ]; then
  echo "Cloning blackadder repository"
  git clone https://github.com/fp7-pursuit/blackadder $BLACKADDER_DIR && \
  cd $BLACKADDER_DIR/src && \
  ./configure --disable-linuxmodule && make && make install && \
  cd $BLACKADDER_DIR/lib && \
  #autoreconf –fi && \ #TODO: Check why this fails on the script
  ./configure && make && make install
else
  echo "Pulling blackadder repository"
  cd $BLACKADDER_DIR &&\
  git pull &&\
  cd $BLACKADDER_DIR/src && \
  make && make install && \
  cd $BLACKADDER_DIR/lib && \
  make && make install
fi
rc=$?
if [ ! $rc -eq 0 ]; then
  exit $rc
fi
cd $CWD 
