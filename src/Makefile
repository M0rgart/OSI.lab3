CC = gcc
CFLAGS = -O2 -Wall -Wextra -Iinc
LDLIBS = -lkernel32
SRC = src/main.c src/timsort.c src/pool.c src/common.c
OBJ = $(SRC:.c=.o)
TARGET = lab3.exe

.PHONY: all clean run bench

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-del /Q $(subst /,\,$(OBJ)) 2>NUL
	-del /Q $(TARGET) 2>NUL