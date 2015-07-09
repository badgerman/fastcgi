EXT=
PREFIX = /opt
CFLAGS = -g -Wall -Werror -Wextra -I$(EXT)critbit -std=c99 -Wconversion
PROGRAMS = counters
TESTS =
WEBSITE = /var/www/html

ifeq "$(CC)" "clang"
CFLAGS += -Weverything -Wno-padded 
endif

# http://www.thinkplexx.com/learn/howto/build-chain/make-based/prevent-gnu-make-from-always-removing-files-it-says-things-like-rm-or-removing-intermediate-files
.SECONDARY: prefix.o

all: $(PROGRAMS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

CuTest.o: $(EXT)critbit/CuTest.c
	$(CC) $(CFLAGS) -Wno-format-nonliteral -o $@ -c $< $(INCLUDES)

critbit.o: $(EXT)critbit/critbit.c
	$(CC) $(CFLAGS) -Wno-sign-conversion -o $@ -c $< $(INCLUDES)

counters: cgiapp.o counters.o
	$(CC) $(CFLAGS) -o $@ $^ -lfcgi $(LDFLAGS)

counter-cgi: counter.o
	$(CC) $(CFLAGS) -o $@ $^ -lfcgi $(LDFLAGS)

cgiapp.a: cgiapp.o critbit.o
	$(AR) -q $@ $^

%-cgi: %.o cgiapp.a
	$(CC) $(CFLAGS) -o $@ $^ -lfcgi $(LDFLAGS)

clean:
	rm -f .*~ *~ *.a *.o $(PROGRAMS) $(TESTS)

install: $(PROGRAMS)
	sudo mkdir -p $(PREFIX)/bin
	sudo install $(PROGRAMS) $(PREFIX)/bin
	[ -d /etc/init.d ] && sudo install -C etc/init.d/* /etc/init.d/
	[ -d /etc/systemd ] && sudo install -C etc/systemd/* `pkg-config systemd --variable=systemdsystemunitdir`
	sudo mkdir -p $(PREFIX)/share/doc/fastcgi/examples
	sudo install doc/examples/* $(PREFIX)/share/doc/fastcgi/examples/
