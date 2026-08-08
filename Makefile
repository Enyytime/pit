CC       = gcc
CFLAGS   = -std=gnu23 -Wall -Wextra -pedantic -g -I.
LDLIBS   = -lssl -lcrypto -lz
TARGET   = pit
PREFIX  ?= /usr/local

SRCS = $(wildcard *.c) $(wildcard include/*.c) \
       $(wildcard pit_commands/*.c) $(wildcard data_structures/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

debug: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
debug: LDLIBS += -fsanitize=address,undefined
debug: clean $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

test: $(TARGET)
	PATH="$(CURDIR):$$PATH" ./tests/compat.sh

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)

.PHONY: all debug install test clean