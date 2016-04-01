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

TARGET=http-to-pursuit.so http-to-ndn.so
SRC_DIR=.
INCLUDE_DIR=../../include
BUILD_DIR=../../dist/fixp-converters

CC=gcc
CXX=g++
RM=rm
CFLAGS=
CPPFLAGS=-I$(INCLUDE_DIR) -std=c++11 -fPIC
LDFLAGS=-shared
LDLIBS=

SOURCES=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

http-to-pursuit.so: $(SRC_DIR)/http-to-pursuit.cpp $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS)

http-to-ndn.so: $(SRC_DIR)/http-to-ndn.cpp $(BUILD_DIR)
	$(CXX) $< $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(LDLIBS)

clean:
	$(RM) -f $(OBJS)

dist-clean:
	$(RM) -rf $(BUILD_DIR)

