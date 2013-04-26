LBITS = $(shell getconf LONG_BIT)
TIMESTAMP = `date +'%y.%m.%d %H:%M:%S'`
SRC=main.cpp \
	FileDist.cpp \
	fcg.cpp \
	aligner.cpp \
	sat_cosim.cpp \
	sat_impfunc.cpp \
	sat_cyccomp.cpp \
	Graph.cpp \
	asp.cpp \
	../utils/utils.cpp \
	../common/function.cpp \
	../Rex/rex.cpp \
	../Rex/rextranslator.cpp \
	../Rex/rexstp.cpp \
	../Rex/rexutils.cpp \
	../Rex/rexfunc.cpp \
	../Rex/rexbb.cpp \
	../Rex/rexio.cpp \
	../Rex/rexmachine.cpp \
	../Rex/MurmurHash3.cpp
CFLAGS=-DNVERBOSE -g \
	-DTIMESTAMP="$(TIMESTAMP)" \
	-DNUSE_STP_FILE \
	-DNO_OBSOLETE_FUNCS \
	-I./ \
	-I../valgrind/VEX/pub/ \
	-I../stp/src/c_interface \
	-I../Rex/ \
	-I../common/ \
	-I../../../ida-6.3/idasdk63/include \
	-I/tmp/local/include \
	-I/tmp/local/mysql/include \
	-I/usr/include
LDFLAGS=-L../valgrind/VEX/ -lvex-amd64-linux \
	-L../stp/src/c_interface/ -lcinterface \
	-L../stp/lib -lstp \
	-L/usr/local/lib \
	-L/tmp/local/mysql/lib/mysql \
	-llog4cpp -lmysqlclient \
	-lboost_regex -lboost_graph \
	-lgsl -lgslcblas \
	-Wl,--rpath,/tmp/local/x86_$(LBITS)/lib 

all:GraphMatcher
	cp ./GraphMatcher.exe ../../tools/
	echo -en "\007"
	
GraphMatcher:GraphMatcher.cpp GraphMatcher.h LMException.hpp
	g++ $(CFLAGS) $(SRC) $(LDFLAGS) -o GraphMatcher.exe

clean:
	rm -f *.o
	rm -f *.exe
