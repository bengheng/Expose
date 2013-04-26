NAME=rex
LBITS = $(shell getconf LONG_BIT)
SRC=main.cpp \
	rex.cpp \
	rextranslator.cpp \
	rexstp.cpp \
	rexutils.cpp \
	rexfunc.cpp \
	rexbb.cpp \
	rexmachine.cpp \
	rexio.cpp \
	MurmurHash3.cpp
OBJ=$(SRC:.cpp=.o)
CFLAGS=-DNVERBOSE -DUSE_STP_FILE -g \
	-I./ \
	-I../valgrind/VEX/pub/ \
	-I../stp/src/c_interface \
	-I/tmp/local/include
LDFLAGS=-L../valgrind/VEX/ -lvex-amd64-linux -L../stp/src/c_interface -lcinterface -L../stp/lib -lstp -L/tmp/local/lib -llog4cpp -lpthread

all:$(NAME)
	echo -en "\007"
	
$(NAME):
	g++ $(CFLAGS) $(SRC) $(LDFLAGS) -o $(NAME).1.exe

test:
	g++ $(CFLAGS) test.cpp $(LDFLAGS) -o test.exe

testlog4cpp:
	g++ $(CFLAGS) testlog4cpp.cpp $(LDFLAGS) -o testlog4cpp.exe

clean:
	rm -f log*
	rm -f *.o
	rm -f *.exe
