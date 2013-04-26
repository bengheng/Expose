NAME=dumpmnem
SRC=main.cpp dumpmnem.cpp function_analyzer.cpp ../tinyxml/tinystr.cpp ../tinyxml/tinyxml.cpp ../tinyxml/tinyxmlerror.cpp ../tinyxml/tinyxmlparser.cpp
CC=g++
LD=g++
IDA=/home/bengheng/Research/ida-6.3
IDASDK=$(IDA)/idasdk63
MYSQL=/home/bengheng/Downloads/mysql-5.5.28-linux2.6-i686
CFLAGS=-O3 -D__IDP__ -D__PLUGIN__ -m32 -fPIC -D__LINUX__ -I$(IDASDK)/include -I../tinyxml
LDFLAGS=-O3 -m32 --shared -L$(IDASDK)/lib -Wl,--version-script=./plugin.script

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx
	$(CC) -D__X64__ $(CFLAGS) $(LDFLAGS) $(SRC) -o $(NAME).plx64

	cp $(NAME).plx $(IDA)/plugins
	cp $(NAME).plx64 $(IDA)/plugins

clean:
	rm -rf *.o *.plx *.plx64
	
	@if [ -f $(IDA)/plugins/$(NAME).plx ]; then \
		rm $(IDA)/plugins/$(NAME).plx; \
	fi
	
	@if [ -f $(IDA)/plugins/$(NAME).plx64 ]; then \
		rm $(IDA)/plugins/$(NAME).plx64; \
	fi

