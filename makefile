CC = gcc
CFLAGS = 
LDFLAGS =
OBJFILES = scanner.o main.o parser.o
TARGET = 412fe
build: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~