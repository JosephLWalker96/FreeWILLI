CXX = g++
CXXFLAGS = -std=c++20
LIBS = -larmadillo -lliquid
INCLUDES = -I/usr/include/sigpack-1.2.7/sigpack

all: listen

listen: multi_datalogger_reader.o process_data.o utils.o TDOA_estimation.o 
	$(CXX) $(CXXFLAGS) multi_datalogger_reader.o process_data.o utils.o TDOA_estimation.o -o listen $(LIBS)

multi_datalogger_reader.o: multi_datalogger_reader.cpp 
	$(CXX)  -Ofast $(CXXFLAGS) $(INCLUDES) -c multi_datalogger_reader.cpp
process_data.o: process_data.cpp 
	$(CXX)  -Ofast $(CXXFLAGS) -c process_data.cpp
utils.o: utils.cpp 
	$(CXX)  -Ofast $(CXXFLAGS) -c utils.cpp
TDOA_estimation.o: TDOA_estimation.cpp 
	$(CXX)  -Ofast $(CXXFLAGS) $(INCLUDES) -c TDOA_estimation.cpp

clean:
	rm *.o listen


