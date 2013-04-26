SOURCES=distance.cpp euclid.cpp mahal.cpp cosim.cpp bitap.cpp libbitap.c
LBITS=$(shell getconf LONG_BIT)
CPPFLAGS=-DNVERBOSE -O3 -I./ -I/tmp/local/include -I/tmp/local/mysql/include -I/usr/local/x86_$(LBITS)/include -I/usr/include
LDFLAGS=-L/tmp/local/lib -L/tmp/local/mysql/lib/mysql -L/usr/local/x86_$(LBITS)/lib -llog4cpp -lm -lmysqlclient -lboost_regex -lboost_graph -lgsl -lgslcblas $(SOURCES) -Wl,--rpath,/usr/local/x86_$(LBITS)/lib

distance:$(SOURCES)
	g++ $(CPPFLAGS) $(LDFLAGS) -o distance.exe
	cp distance.exe ~/Research/libmatcher/tools/

clean:
	rm -f *.o
	rm -f *.exe
