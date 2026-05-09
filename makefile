CXX := g++
CXXFLAGS := -std=c++17 -Wall -g -I./src -I./external/cadical/src -I./external/igraph-install/include/igraph
LDFLAGS := -L./external/cadical/build -L./external/igraph-install/lib
LDLIBS := -lcadical -ligraph

SRC_DIR := src
BUILD_DIR := build
TARGET := bdd_solver

# Recursive wildcard: finds files matching a pattern in $1 and all subdirectories.
# Used so we don't have to list every subfolder under src/ explicitly.
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SOURCES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

# Compile rule: src/foo/bar.cpp -> build/foo/bar.o
# `mkdir -p $(dir $@)` creates the matching subdirectory under build/ on demand,
# so we don't have to enumerate folders in advance.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean