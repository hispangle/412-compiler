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
	tar cvf agm6.tar scanner.c main.c renamer.c constants.h scheduler.c graph.o graph.h parser.c list.h makefile README