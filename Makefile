CC = gcc
CFLAGS = -Wextra -pedantic
TARGET = pit

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: install clean
