#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include "json_parser.h"
#include <stdbool.h>

// Structure to hold a conversation entry
typedef struct {
    char *type;           // "MESSAGE" or "THOUGHTS"
    char *text;
    int order;
    bool has_thoughts;
    int parent_message;   // -1 if not applicable
} ConversationEntry;

// Structure to hold search results
typedef struct {
    ConversationEntry *entry;
    int match_position;   // Position of match in text
    int context_start;    // Start index for context window
    int context_end;      // End index for context window
} SearchResult;

// Structure to hold the entire conversation
typedef struct {
    char *timestamp;
    char *platform;
    int total_count;
    int message_count;
    int thought_count;
    ConversationEntry *entries;
    size_t entry_count;
} Conversation;

// Load and parse a conversation JSON file
Conversation* load_conversation(const char *filename);

// Free conversation memory
void free_conversation(Conversation *conv);

// Search for a term in the conversation (case-insensitive)
SearchResult* search_conversation(Conversation *conv, const char *search_term, size_t *result_count);

// Free search results
void free_search_results(SearchResult *results, size_t count);

// Print a search result with context
void print_search_result(SearchResult *result, int result_number, const char *search_term, const char *filename);

// Print conversation statistics
void print_conversation_stats(Conversation *conv);

#endif