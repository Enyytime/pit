CC = gcc
CFLAGS = -Wextra -pedantic
TARGET = pit

SRCS = $(wildcard *.c) $(wildcard include/*.c)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -lssl -lcrypto -lz

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: install clean
