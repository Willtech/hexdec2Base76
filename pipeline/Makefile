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

TARGET  = b76pipe_stream
OBJS    = b76pipe_stream.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

b76pipe_stream.o: b76pipe_stream.c b76pipe.h base76_alphabet.h
	$(CC) $(CFLAGS) -c b76pipe_stream.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
