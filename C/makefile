# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -march=native -Ofast -flto -g -Wall   # remove -g -Wall for production build
LIBS = -lopenblas -llapack -lliquid -lfftw3
INCLUDES = -I/usr/include/sigpack-1.2.7/sigpack

# Directories 
SRC_DIR := src
OBJ_DIR := src

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Target executable
TARGET := listen

# Make all targets by default
all: $(TARGET)

# Linking target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# PHONY targets to prevent conflicts with file names
.PHONY: all clean
