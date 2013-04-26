CXXFLAGS+=-Wall

all:utils

utils:utils.cpp
	$(CXX) $(CXXFLAGS) -c utils.cpp -o utils64.o
	$(CXX) $(CXXFLAGS) -m32 -c utils.cpp -o utils32.o

idautils:utils.cpp
	$(CXX) $(CXXFLAGS) -m32 -D__IDP__ -D__LINUX__ -DIDA -I../idasdk/5.6/include -c utils.cpp -o idautils32.o

clean:
	rm -rf *.o
