CFLAGS += -O2 -fpic -Wall `pkg-config gkrellm --cflags`

# maximum cpus supported by your kernel
#MAX_CPUS := $(shell cat /sys/devices/system/cpu/kernel_max)

# force a specific number of cpus
#MAX_CPUS := 4

# defaults to current CPU count
MAX_CPUS := $(shell nproc)



all: gkfreq.so

gkfreq.o: gkfreq.c
	$(CC) $(CFLAGS) -DGKFREQ_MAX_CPUS=$(MAX_CPUS) -c gkfreq.c

gkfreq.so: gkfreq.o
	$(CC) -shared -ogkfreq.so gkfreq.o

install:
	install -m755 gkfreq.so ~/.gkrellm2/plugins/

clean:
	rm -rf *.o *.so

# start gkrellm in plugin-test mode
# (of course gkrellm has to be in PATH)
test: gkfreq.so
	`which gkrellm` -p gkfreq.so

.PHONY: install clean
