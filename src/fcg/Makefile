SRC=main.cpp fcg.cpp ../utils/utils.cpp
CC=g++
LD=g++
PREFIX=/tmp/local
CFLAGS=-O3 -m32 -I$(PREFIX)/x86_32/include/
LDFLAGS=-O3 -m32 -L$(PREFIX)/x86_32/lib -lboost_graph -lgsl -lgslcblas

testfcg:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC)  -o testcfg.exe 
clean:
	rm -f *.o
	rm -f *.exe
