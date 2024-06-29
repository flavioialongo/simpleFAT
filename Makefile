# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Werror -g

# Target executable
TARGET = myfat

# Source files
SRCS = filesystem.c main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -g -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET) test.img

# Phony targets
.PHONY: all clean