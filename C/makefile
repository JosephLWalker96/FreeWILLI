# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -march=native -Ofast -flto
LIBS = -lopenblas -llapack -lliquid -lfftw3
INCLUDES = -I/usr/include/sigpack-1.2.7/sigpack

# Source files
SRCS = multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp filters.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = listen

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
