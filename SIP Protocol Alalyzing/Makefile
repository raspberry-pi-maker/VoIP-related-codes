CC=g++
INC1 = ./libosip2-5.3.1/include
INC2 =
INCDIRS = -I${INC1}  -I${INC2}

LIBDIR1 =/usr/local/lib
LIBDIR2 =

CFLAGS=-c -Wall  ${INCDIRS} -std=c++11 -g
LDFLAGS= -L${LIBDIR1} -L${LIBDIR2} -losip2 -losipparser2
SOURCES=osip_test.cpp
OBJECTS=$(SOURCES:.cpp=.o)
BACKUP=$(SOURCES:.cpp=.cpp-)
EXECUTABLE=osip_test

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@


.cpp.o:
	$(CC) -L.  $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) 

