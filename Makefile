CC = gcc
CFLAGS = -Wextra -pedantic
TARGET = pit

$(TARGET): main.c hash_object.c include/file_handler.c
	$(CC) $(CFLAGS) main.c hash_object.c include/file_handler.c -o $(TARGET) -lssl -lcrypto -lz

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: install clean
