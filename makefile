CC = gcc
CFLAGS = 
LDFLAGS =
OBJFILES = scanner.o main.o parser.o
TARGET = fe412
build: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~