CC=gcc
CFLAGS=-c -Wall -I.
LDFLAGS=-pthread
SOURCES=main.c server.c client.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=program

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
