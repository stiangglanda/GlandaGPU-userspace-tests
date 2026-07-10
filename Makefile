CC = gcc

ifeq ($(ARCH),arm)
    CC = arm-linux-gnueabihf-gcc
endif

CFLAGS = -Wall -O2
LDFLAGS = -static
LDLIBS = -lm

TARGET = gpu_test
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean