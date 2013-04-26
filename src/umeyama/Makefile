LDFLAGS+=-lgsl
LDFLAGS+=-lgslcblas
CXXOBJECTS+=umeyama.o\
	../utils/utils64.o\
	../gsl_hungarian/hungarian.o
CXXFLAGS+=-Wall
CXXFLAGS+=-g

all:umeyama main

umeyama:umeyama.cpp
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -c umeyama.cpp

main:umeyama main.cpp
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CXXOBJECTS) main.cpp -o umeyama.exe
	
clean:
	rm -rf *.o
	rm -rf *.exe

