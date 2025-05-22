CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses -pthread

SRCS = main.c system_info.c display.c
OBJS = $(SRCS:.c=.o)
TARGET = system_monitor
STRESS_TARGET = stress_test

all: $(TARGET) $(STRESS_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(STRESS_TARGET): stress_test.c
	$(CC) $(CFLAGS) -o $(STRESS_TARGET) stress_test.c -pthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(STRESS_TARGET)

.PHONY: all clean 