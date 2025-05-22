CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

SRCS = main.c cpu_info.c memory_info.c disk_info.c gpu_info.c network_info.c
OBJS = $(SRCS:.c=.o)

all: system_monitor

system_monitor: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c system_info.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) system_monitor

.PHONY: all clean 