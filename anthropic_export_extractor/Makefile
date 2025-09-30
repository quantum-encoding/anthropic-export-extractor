# Anthropic Export Extractor - Makefile
# Author: Richard Tune <rich@quantumencoding.io>
# Company: QUANTUM ENCODING LTD

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS = -lm

# Targets
EXTRACTOR = anthropic_export_extractor
PARSER = json_parser
LIBRARY = libjson_parser.a

# Object files
PARSER_OBJS = json_parser.o
EXTRACTOR_OBJS = json_extractor.o
MAIN_OBJS = main.o

all: $(LIBRARY) $(EXTRACTOR) $(PARSER)

# Build the extractor (the main tool)
$(EXTRACTOR): $(LIBRARY) $(EXTRACTOR_OBJS)
	$(CC) $(EXTRACTOR_OBJS) -L. -ljson_parser -o $(EXTRACTOR) $(LDFLAGS)

# Build the parser test binary
$(PARSER): $(LIBRARY) $(MAIN_OBJS)
	$(CC) $(MAIN_OBJS) -L. -ljson_parser -o $(PARSER) $(LDFLAGS)

# Build the JSON parser library
$(LIBRARY): $(PARSER_OBJS)
	ar rcs $(LIBRARY) $(PARSER_OBJS)

# Compile object files
json_parser.o: json_parser.c json_parser.h
	$(CC) $(CFLAGS) -c json_parser.c

json_extractor.o: json_extractor.c json_parser.h
	$(CC) $(CFLAGS) -c json_extractor.c

main.o: main.c json_parser.h
	$(CC) $(CFLAGS) -c main.c

# Utilities
clean:
	rm -f $(EXTRACTOR) $(PARSER) $(LIBRARY) *.o

rebuild: clean all

test: $(PARSER)
	./$(PARSER)

extract: $(EXTRACTOR)
	@echo "Usage: ./$(EXTRACTOR) <conversations.json>"

help:
	@./$(EXTRACTOR) --help

.PHONY: all clean rebuild test extract help