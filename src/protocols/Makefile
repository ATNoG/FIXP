##  Brief: Automate project compilation and linkage
##  Copyright (C) 2016  Carlos Guimaraes <cguimaraes@av.it.pt>
##
##  This program is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program. If not, see <http://www.gnu.org/licenses/>.

TARGET=pursuit-protocol.so pursuit-multipath-protocol.so http-protocol.so ndn-protocol.so
SRC_DIR=.
INCLUDE_DIR=../../include
BUILD_DIR=../../dist/fixp-protocols

CC=gcc
CXX=g++
RM=rm
CFLAGS=
CPPFLAGS=-I$(INCLUDE_DIR) -std=c++14 -fPIC
LDFLAGS=-shared
LDLIBS=

NDNCXXFLAGS=`pkg-config --cflags libndn-cxx`
NDNLDFLAGS=`pkg-config --libs libndn-cxx`

SOURCES=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

pursuit-protocol.so: $(SRC_DIR)/pursuit-protocol.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS) -lblackadder -lcryptopp

pursuit-multipath-protocol.so: $(SRC_DIR)/pursuit-multipath-protocol.o $(SRC_DIR)/pursuit/chunk.o $(BUILD_DIR)
	$(CXX) $(word 1,$^) $(word 2,$^) $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS) -lblackadder -lcryptopp

ndn-protocol.so: $(SRC_DIR)/ndn-protocol.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(NDNCXXFLAGS) $(LDFLAGS) $(NDNLDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS)

http-protocol.so: $(SRC_DIR)/http-protocol.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS) -lcurl -lmicrohttpd

clean:
	$(RM) -f $(OBJS)

dist-clean:
	$(RM) -rf $(BUILD_DIR)

