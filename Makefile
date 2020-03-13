CFLAGS=-O

all: dial

dial: dial.o
	$(CC) $(CFLAGS) dial.o -o dial -lm

dial.o: dial.c
	$(CC) $(CFLAGS) -c dial.c

clean:
	$(RM) dial.o dial
