CC = gcc
CFLAGS = 
LDFLAGS =
OBJFILES = scanner.o main.o parser.o renamer.o scheduler.o graph.o
TARGET = schedule
build: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~ agm6.tar core.*
tar:
	tar cvf agm6.tar scanner.c main.c renamer.c constants.h scheduler.c graph.o graph.h parser.c makefile README