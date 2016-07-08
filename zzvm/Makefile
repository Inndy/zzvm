CC = gcc
CFLAGS = -O3

all: zzvm

zzvm: main.o zzvm.o
	$(CC) zzvm.o main.o -o $@

test: test.o zzvm.o
	$(CC) zzvm.o main.o -o $@

%.o: %.c
	$(CC) $< -c $(CFLAGS)

clean:
	rm *.o zzvm
