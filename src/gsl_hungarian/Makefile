all:main

hungarian:hungarian.cpp hungarian.h
	$(CXX) -DNVERBOSE ../utils/utils.cpp -c -g -o utils64.o
	$(CXX) ../utils/utils.cpp -c -g -m32 -o utils32.o
	$(CXX) -DNVERBOSE hungarian.cpp -c -g -lm -o hungarian64.o
	$(CXX) hungarian.cpp -c -g -lm -m32 -o hungarian32.o

main:hungarian main.cpp
	g++ -DNVERBOSE -g main.cpp hungarian64.o utils64.o -lgsl -lgslcblas -o main64.exe
	g++ -g main.cpp hungarian32.o utils32.o -lgsl -lgslcblas -m32 -o main32.exe

clean:
	rm -rf *.o
	rm -rf *.exe
