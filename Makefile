# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Werror

# Source files
SRCS = filesystem.c main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output executable
OUTPUT = myfat

# Default target
all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OUTPUT) $(OBJS) test.img

# Phony targets
.PHONY: all clean

