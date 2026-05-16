CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11
TARGET  = uart
SRC     = uart.c

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(SRC) uart.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -f $(TARGET)
