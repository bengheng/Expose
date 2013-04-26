NAME=makedot
SRC=main.cpp \
	lmobject.cpp \
	lmfunction.cpp \
	lmcfg.cpp \
	../common/function_analyzer.cpp \
	../common/blacklist.cpp \
	../utils/utils.cpp \
	../fcg/fcg.cpp sha1.cpp
OBJS=$(SRC:.cpp=.o)
CC=g++
LD=g++
IDA=/home/bengheng/Research/ida-6.3
IDASDK=$(IDA)/idasdk63
PREFIX=/usr
CFLAGS=-DNVERBOSE -O3 -m32 -D__IDP__ -D__PLUGIN__ -fPIC -D__LINUX__ -I../common -I$(PREFIX)/x86_32/include -I$(IDASDK)/include
LDFLAGS=-DNVERBOSE -O3 -m32 --shared -L$(PREFIX)/x86_32/lib -llog4cpp -lboost_regex -lboost_graph -lgsl -lgslcblas -L$(IDASDK)/lib -Wl,--version-script=./plugin.script,--rpath=$(PREFIX)/x86_32/lib

all:makedot
	cp $(NAME).plx $(IDA)/plugins
	cp $(NAME).plx64 $(IDA)/plugins

makedot:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx
	$(CC) -D__X64__ $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx64


clean:
	rm -rf *.o *.plx *.plx64
	
	@if [ -f $(IDA)/plugins/$(NAME).plx ]; then \
		rm $(IDA)/plugins/$(NAME).plx; \
	fi
	
	@if [ -f $(IDA)/plugins/$(NAME).plx64 ]; then \
		rm $(IDA)/plugins/$(NAME).plx64; \
	fi

