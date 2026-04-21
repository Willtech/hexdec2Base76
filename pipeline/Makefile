#==============================================================================
# Makefile for compiled b76pipe_stream sub-project
#
# Source Code produced by Willtech 2023–2026
# Compiled pipeline variant based on hexdec2Base76 project
# Converted & annotated with reference header by
# Professor. Damian A. James Williamson Grad. + Copilot
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
	@if [ -f b76pipe_stream.3 ]; then \
		install -m 644 b76pipe_stream.3 $(MANDIR)/b76pipe_stream.3; \
		echo "  installed b76pipe_stream.3"; \
	else \
		echo "  b76pipe_stream.3 not found, skipping"; \
	fi
	@if [ -f base76_alphabet.3 ]; then \
		install -m 644 base76_alphabet.3 $(MANDIR)/base76_alphabet.3; \
		echo "  installed base76_alphabet.3"; \
	else \
		echo "  base76_alphabet.3 not found, skipping"; \
	fi

uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)"
	rm -f $(BINDIR)/$(TARGET)

	@echo "Removing manpages from $(MANDIR)"
	rm -f $(MANDIR)/b76pipe_stream.3
	rm -f $(MANDIR)/base76_alphabet.3

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean install uninstall
