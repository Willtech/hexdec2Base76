#==============================================================================
# Makefile for compiled b76pipe_stream sub-project
#==============================================================================

CC      = cc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS =

PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
MANDIR  = $(PREFIX)/man/man3

TARGET  = b76pipe_stream
OBJS    = b76pipe_stream.o

MANPAGES = b76pipe_stream.3 base76_alphabet.3

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

b76pipe_stream.o: b76pipe_stream.c b76pipe_stream.h base76_alphabet.h
	$(CC) $(CFLAGS) -c b76pipe_stream.c

install: $(TARGET)
	@echo "Installing $(TARGET) to $(BINDIR)"
	mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)

	@echo "Installing manpages to $(MANDIR)"
	mkdir -p $(MANDIR)
	-test -f b76pipe_stream.3 && install -m 644 b76pipe_stream.3 $(MANDIR)/b76pipe_stream.3
	-test -f base76_alphabet.3 && install -m 644 base76_alphabet.3 $(MANDIR)/base76_alphabet.3

uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)"
	-rm -f $(BINDIR)/$(TARGET)

	@echo "Removing manpages from $(MANDIR)"
	-rm -f $(MANDIR)/b76pipe_stream.3
	-rm -f $(MANDIR)/base76_alphabet.3

test: $(TARGET)
	@echo "Running Base76 pipeline test suite"
	@chmod +x tests/test.sh
	cd tests && ./test.sh

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean install uninstall test
