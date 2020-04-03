CFLAGS=-Wall -Wextra -std=c99 -g3 -O0
GCC=gcc
SRC=main.c compile.c lexical.c generate.c table.c heap.c execute.c error.c memory.c error_message.c
OBJ=$(SRC:.c=.o)
TARGET=ll1

.PHONY: clean all

all: $(TARGET)

clean:
	rm -rf *.o

rebuild:
	make clean
	make all

$(TARGET): $(OBJ)
	$(GCC) $(CFLAGS) $^ -o $@

.c.o: 
	$(GCC) $(CFLAGS) -c $<
