SOURCES=part.c main.c xtaf.c disk.c
OBJECTS=$(SOURCES:.c=.o)
EXEC=reader
MY_CFLAGS += -Wall -Werror -O0 -g
MY_LIBS += 

all: $(OBJECTS)
	$(CC) $(LIBS) $(LDFLAGS) $(OBJECTS) $(MY_LIBS) -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)

.c.o:
	$(CC) -c $(CFLAGS) $(MY_CFLAGS) $< -o $@

