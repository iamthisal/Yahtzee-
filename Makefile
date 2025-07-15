CC = gcc
CFLAGS = -Wall -Wextra -std=c11

ifeq ($(OS),Windows_NT)
TARGET = yahtzee.exe
RUN = .\$(TARGET)
REMOVE = del /Q
else
TARGET = yahtzee
RUN = ./$(TARGET)
REMOVE = rm -f
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): yahtzee.c
	$(CC) $(CFLAGS) yahtzee.c -o $(TARGET)

run: $(TARGET)
	$(RUN)

clean:
	$(REMOVE) $(TARGET)
