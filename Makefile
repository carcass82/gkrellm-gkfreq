CFLAGS += -fPIC -Wall `pkg-config gkrellm --cflags`

all: gkfreq.so

gkfreq.o: gkfreq.c
	$(CC) $(CFLAGS) -c gkfreq.c

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
