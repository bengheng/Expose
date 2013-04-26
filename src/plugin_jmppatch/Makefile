NAME=jmppatch
SRC=main.cpp jmppatch.cpp makefuncs.cpp
OBJS=$(SRC:.cpp=.o)
CC=g++
LD=g++
IDA=/home/bengheng/Research/ida-6.3
IDASDK=$(IDA)/idasdk63
MYSQL=/home/bengheng/Downloads/mysql-5.5.28-linux2.6-i686
CFLAGS=-g -m32 -D__IDP__ -D__PLUGIN__ -fPIC -D__LINUX__ \
	-I./ \
	-I$(IDASDK)/include \
	-I/tmp/local/x86_32/include \
	-I/usr/local/x86_32/include
LDFLAGS=--shared \
	-L$(IDASDK)/lib/ \
	-L/tmp/local/x86_32/lib/ \
	-L/usr/local/x86_32/lib/ \
	-llog4cpp  \
	-Wl,--version-script=./plugin.script,--rpath=/usr/local/x86_32/lib

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx
	$(CC) -D__X64__ $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx64
	
	cp $(NAME).plx $(IDA)/plugins
	cp $(NAME).plx64 $(IDA)/plugins
	
	@if [ -f log_$(NAME) ]; then \
		rm log_$(NAME); \
	fi

clean:
	rm -rf *.o *.plx *.plx64
	
	@if [ -f $(IDA)/plugins/$(NAME).plx ]; then \
		rm $(IDA)/plugins/$(NAME).plx; \
	fi
	
	@if [ -f $(IDA)/plugins/$(NAME).plx64 ]; then \
		rm $(IDA)/plugins/$(NAME).plx64; \
	fi

