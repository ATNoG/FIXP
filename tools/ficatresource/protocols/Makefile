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

TARGET=plugin-example.so plugin-pursuit.so plugin-ndn.so
SRC_DIR=.
INCLUDE_DIR=../../../include
BUILD_DIR=../../../dist/tools/ficatresource-protocols

CC=gcc
CXX=g++
RM=rm
CFLAGS=
CPPFLAGS=-I$(INCLUDE_DIR) -std=c++11 -fPIC
LDFLAGS=-shared
LDLIBS=

NDNCXXFLAGS=`pkg-config --cflags libndn-cxx`
NDNLDFLAGS=`pkg-config --libs libndn-cxx`

SOURCES=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

plugin-example.so: $(SRC_DIR)/plugin-example.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS)

plugin-ndn.so: $(SRC_DIR)/plugin-ndn.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(NDNCXXFLAGS) $(LDFLAGS) $(NDNLDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS)

plugin-pursuit.so: $(SRC_DIR)/plugin-pursuit.o $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS) -pthread -lblackadder

clean:
	$(RM) -f $(OBJS)

dist-clean:
	$(RM) -rf $(BUILD_DIR)

