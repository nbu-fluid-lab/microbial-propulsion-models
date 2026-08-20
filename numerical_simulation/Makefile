CXX ?= c++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS ?=

TARGET := build/funtrack_simulation
SOURCES := main.cpp
HEADERS := cylinder.h utils.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@

clean:
	rm -rf build
