CC=g++
INC1 =/usr/local/src/libosip2-5.3.1/include
INC2 =
#INCDIRS = -I${INC1}  -I${INC2}
INCDIRS = -I${INC1}

LIBDIR1 =/usr/local/lib
LIBDIR2 =

CFLAGS=-c -Wall -std=c++11 -g
#LDFLAGS= -L${LIBDIR1} -L${LIBDIR2} -lpthread -lpcap -lncurses 
LDFLAGS= -lpthread -losip2 -losipparser2
SOURCES=epoll.cpp log.cpp main.cpp channel.cpp sip_parser.cpp util.cpp rtp.cpp
OBJECTS=$(SOURCES:.cpp=.o)
BACKUP=$(SOURCES:.cpp=.cpp~)
EXECUTABLE=record_server

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@


.cpp.o:
	$(CC) -L. $(INCDIRS) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)  $(BACKUP)

