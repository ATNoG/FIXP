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

EXEC=ficatresource
SRC_DIR=.
INCLUDE_DIR=../../include
BUILD_DIR=../../dist/tools
PLUGINS_DIR = protocols

CC=gcc
CXX=g++
RM=rm
CFLAGS=
CPPFLAGS=-I$(INCLUDE_DIR) -std=c++14
LDFLAGS=
LDLIBS=-ldl -lboost_program_options

SOURCES=$(wildcard $(SRC_DIR)/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

all: $(EXEC)
	@ set -e ; \
	$(foreach dir, $(PLUGINS_DIR), make $@ -C $(dir);)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXEC): $(OBJS) $(BUILD_DIR)
	$(CXX) $(OBJS) $(CPPFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$(EXEC) $(LDLIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	@ set -e ; \
	$(foreach dir, $(PLUGINS_DIR), make $@ -C $(dir);)
	rm -f $(OBJS)

dist-clean:
	@ set -e ; \
	$(foreach dir, $(PLUGINS_DIR), make $@ -C $(dir);)
	rm -rf $(BUILD_DIR)/$(EXEC)

