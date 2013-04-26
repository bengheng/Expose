NAME=instrinfo
SRC=main.cpp
OBJS=$(SRC:.cpp=.o)
CC=g++
LD=g++
IDA=/home/bengheng/Research/ida-6.3
IDASDK=$(IDA)/idasdk63
MYSQL=/home/bengheng/Downloads/mysql-5.5.28-linux2.6-i686
PREFIX=/usr/
CFLAGS=-O3 -D__IDP__ -D__PLUGIN__ -m32 -fPIC -D__LINUX__ -I./ -I$(IDASDK)/include -I$(PREFIX)x86_32/include -I$(MYSQL)/include
LDFLAGS=-O3 -m32 --shared -L$(IDASDK)/lib -L$(PREFIX)x86_32/lib -L$(MYSQL)/lib -llog4cpp -lmysqlclient -Wl,--version-script=./plugin.script,--rpath=$(PREFIX)x86_32/lib

all:
	$(CC) $(CFLAGS)	$(LDFLAGS) $(SRC) -o $(NAME).plx
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

