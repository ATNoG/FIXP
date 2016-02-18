#/bin/bash

#Update
apt-get update -qq

#Install build tools 
#TODO: install just the required packages
apt-get install -y build-essential && \
apt-get install -y git && \
apt-get install -y libboost-all-dev && \

#Install FIFu library requirements
apt-get install -y libcrypto++-dev libcurl4-gnutls-dev

#Install Click
apt-get install -y tcpdump libpcap-dev time
git clone https://github.com/kohler/click.git /tmp/click && \
cd /tmp/click && \
./configure --disable-linuxmodule --enable-ip6 --enable-json && make && make install && \ #TODO: Check why make check fails

#Install Blackadder
apt-get -y install libtool autoconf automake libigraph0 libigraph0-dev libconfig++8 libconfig++8-dev libtclap-dev libboost-graph-dev && \
git clone https://github.com/fp7-pursuit/blackadder /tmp/blackadder && \
cd /tmp/blackadder/src && \
./configure --disable-linuxmodule && make && make install && \
cd /tmp/blackadder/lib && \
#autoreconf –fi && \ #TODO: Check why this fails on the script
./configure && make && make install && \
cd ~
