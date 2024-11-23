CC = gcc
CFLAGS = 
LDFLAGS =
OBJFILES = ir.o graph.o scanner.o parser.o renamer.o scheduler.o main.o
TARGET = schedule
build: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~ agm6.tar core.*
tar:
	tar cvf agm6.tar graph.c graph.h ir.c ir.h list.h main.c parser.c renamer.c scanner.c scheduler.c scheduler.h tokens.h makefile README