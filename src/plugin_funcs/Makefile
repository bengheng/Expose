NAME=funcs
SRC=main.cpp
CC=g++
LD=g++
IDA=/home/bengheng/Research/ida-6.3
IDASDK=$(IDA)/idasdk63
PREFIX=/tmp/local
CFLAGS=-DNVERBOSE -O3 -m32 -D__IDP__ -D__PLUGIN__ -fPIC -D__LINUX__  -I$(IDASDK)/include
LDFLAGS=-DNVERBOSE -O3 -m32 --shared -Wl,--version-script=./plugin.script

all:funcs
	cp $(NAME).plx $(IDA)/plugins
	cp $(NAME).plx64 $(IDA)/plugins

funcs:
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

