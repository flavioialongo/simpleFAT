# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Werror

# Source files
SRCS = filesystem.c shell.c
TESTSRC = filesystem.c main.c
# Object files
OBJS = $(SRCS:.c=.o)
# Output executable
OUTPUT = myfat.exe

TESTOUTPUT = test.exe
# Default target
all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Test target
test: 
	$(CC) $(CFLAGS) $(TESTSRC) -o $(OUTPUT)
# Clean target
clean:
	rm -f $(OUTPUT) $(OBJS) test.img

# Phony targets
.PHONY: all clean

