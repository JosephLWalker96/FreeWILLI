
#Use -I to tell the compiler where to find third-party or project-specific header files.
#Use -L to tell the linker where to find third-party or project-specific libraries during the linking phase.


# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -march=native -O3 -flto=auto   # remove -g -Wall for production build
LIBS = -larmadillo -lopenblas -llapack -lliquid -lfftw3   
INCLUDES = -I/usr/include/sigpack-1.2.7/sigpack 

# Directories 
SRC_DIR := ../src
OBJ_DIR := ../src

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
