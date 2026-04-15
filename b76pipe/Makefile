#==============================================================================
# Makefile for compiled b76pipe sub-project
#
# Source Code produced by Willtech 2023–2026
# Compiled pipeline variant based on hexdec2Base76 project
# Converted & annotated with reference header by
# Professor. Damian A. James Williamson Grad. + Copilot
#==============================================================================

CC      = cc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS =

TARGET  = b76pipe
OBJS    = b76pipe.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

b76pipe.o: b76pipe.c b76pipe.h base76_alphabet.h
	$(CC) $(CFLAGS) -c b76pipe.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
