#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <stdbool.h>
#include <stdio.h>

// Structure to hold parsed metadata
typedef struct {
    char *timestamp;
    int total_blocks;
    int messages;
    int thoughts;
} MDMetadata;

// Structure to hold a conversation entry
typedef struct {
    char *type;        // "MESSAGE" or "THOUGHTS"
    char *text;
    int order;
    bool has_thoughts;
    int parent_message; // -1 if not applicable
} MDEntry;

// Structure to hold the entire conversation
typedef struct {
    MDMetadata metadata;
    MDEntry *entries;
    size_t entry_count;
    size_t entry_capacity;
} MDConversation;

// Parse a markdown file
MDConversation* md_parse_file(const char *filename);

// Free conversation memory
void md_free_conversation(MDConversation *conv);

// Convert MD conversation to JSON string
char* md_to_json(MDConversation *conv);

// Write JSON to file
bool md_write_json_file(MDConversation *conv, const char *output_file);

#endif