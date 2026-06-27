CXX := g++
CXXFLAGS := -std=c++17 -Wall -g -O2 -I../external/cadical/src
LDFLAGS := -L../external/cadical/build
LDLIBS := -lcadical

SRC := up_check.cpp
TARGET := ../up_check

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
