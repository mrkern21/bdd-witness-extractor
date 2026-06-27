CXX := g++
CXXFLAGS := -std=c++17 -Wall -g -I../external/cadical/src
LDFLAGS := -L../external/cadical/build
LDLIBS := -lcadical

SRC := verify_witnesses.cpp
TARGET := ../verify_witnesses

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
