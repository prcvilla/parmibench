CC := $(CROSS_COMPILE)gcc
CFLAGS := -lpthread -lm

SRCS = parallel_basicmath.c rad2deg.c cubic.c isqrt.c

OBJS := parallel_basicmath

all: $(OBJS)

parallel_basicmath: $(FILE1)
	$(CC) $(SRCS) -o $@ $(CFLAGS)

clean:
	rm -rf $(OBJS) output*

.PHONY: all clean $(PARAMS)
