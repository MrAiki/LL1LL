CFLAGS=-Wall -ansi -g -O0
GCC=gcc

clean:
	rm *.o

compile_test: compile_test.o compile.o lexical.o error.o error_message.o
	$(GCC) $(CFLAGS) compile.o lexical.o compile_test.o error.o error_message.o -o compile_test

generate_test: generate_test.o compile.o lexical.o generate.o table.o heap.o memory.o error.o error_message.o
	$(GCC) $(CFLAGS) $^ -o $@

execute_test: execute_test.o compile.o lexical.o generate.o table.o heap.o execute.o memory.o error.o error_message.o
	$(GCC) $(CFLAGS) $^ -o $@

compile.o: compile.c compile.h memory.o
	$(GCC) $(CFLAGS) -c compile.c

lexical.o: lexical.c lexical.h
	$(GCC) $(CFLAGS) -c lexical.c

generate.o: generate.h generate.c
	$(GCC) $(CFLAGS) -c generate.c

table.o: table.c table.h
	$(GCC) $(CFLAGS) -c table.c

heap.o: heap.c heap.h
	$(GCC) $(CFLAGS) -c heap.c

execute.o: execute.c execute.h
	$(GCC) $(CFLAGS) -c execute.c

error.o: error.c error.h
	$(GCC) $(CFLAGS) -c error.c 

memory.o: memory.c memory.h
	$(GCC) $(CFLAGS) -c memory.c

error_message.o: error_message.c
	$(GCC) $(CFLAGS) -c error_message.c

compile_test.o: compile_test.c
	$(GCC) $(CFLAGS) -c compile_test.c

