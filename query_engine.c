#define _POSIX_C_SOURCE 200809L
#include "query_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

// Helper function to convert string to lowercase for case-insensitive comparison
static char* to_lowercase(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *lower = malloc(len + 1);
    if (!lower) return NULL;

    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower((unsigned char)str[i]);
    }
    lower[len] = '\0';

    return lower;
}

// Helper function to read entire file into memory
static char* read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Error: Could not allocate memory for file\n");
        return NULL;
    }

    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    return buffer;
}

// Load and parse conversation from JSON file
Conversation* load_conversation(const char *filename) {
    // Read file
    char *json_content = read_file(filename);
    if (!json_content) return NULL;

    // Parse JSON
    JsonValue *root = json_parse(json_content);
    free(json_content);

    if (!root) {
        fprintf(stderr, "Error: Failed to parse JSON file\n");
        fprintf(stderr, "The file may be corrupted or not valid JSON.\n");
        return NULL;
    }

    if (root->type != JSON_OBJECT) {
        fprintf(stderr, "Error: Invalid AI Chronicle JSON format\n");
        fprintf(stderr, "Expected a JSON object with 'entries' array.\n");
        fprintf(stderr, "This tool requires AI Chronicle markdown files to be converted using md2json first.\n");
        json_value_free(root);
        return NULL;
    }

    // Check for required fields
    JsonValue *entries = json_get_object_value(root, "entries");
    if (!entries) {
        fprintf(stderr, "Error: Missing 'entries' field in JSON\n");
        fprintf(stderr, "This doesn't appear to be an AI Chronicle JSON file.\n");
        fprintf(stderr, "Expected format:\n");
        fprintf(stderr, "{\n");
        fprintf(stderr, "  \"timestamp\": \"...\",\n");
        fprintf(stderr, "  \"platform\": \"...\",\n");
        fprintf(stderr, "  \"stats\": {...},\n");
        fprintf(stderr, "  \"entries\": [...]\n");
        fprintf(stderr, "}\n\n");
        fprintf(stderr, "If you have an AI Chronicle markdown file, convert it first:\n");
        fprintf(stderr, "  ./md2json conversation.md > conversation.json\n");
        json_value_free(root);
        return NULL;
    }

    // Allocate conversation structure
    Conversation *conv = calloc(1, sizeof(Conversation));
    if (!conv) {
        json_value_free(root);
        return NULL;
    }

    // Extract timestamp
    JsonValue *timestamp = json_get_object_value(root, "timestamp");
    if (timestamp && timestamp->type == JSON_STRING) {
        conv->timestamp = strdup(timestamp->data.string);
    }

    // Extract platform
    JsonValue *platform = json_get_object_value(root, "platform");
    if (platform && platform->type == JSON_STRING) {
        conv->platform = strdup(platform->data.string);
    }

    // Extract stats
    JsonValue *stats = json_get_object_value(root, "stats");
    if (stats && stats->type == JSON_OBJECT) {
        JsonValue *total = json_get_object_value(stats, "total");
        if (total && total->type == JSON_NUMBER) {
            conv->total_count = (int)total->data.number;
        }

        JsonValue *messages = json_get_object_value(stats, "messages");
        if (messages && messages->type == JSON_NUMBER) {
            conv->message_count = (int)messages->data.number;
        }

        JsonValue *thoughts = json_get_object_value(stats, "thoughts");
        if (thoughts && thoughts->type == JSON_NUMBER) {
            conv->thought_count = (int)thoughts->data.number;
        }
    }

    // Extract entries array (already checked above)
    if (entries->type == JSON_ARRAY) {
        conv->entry_count = entries->data.array.count;
        conv->entries = calloc(conv->entry_count, sizeof(ConversationEntry));

        if (!conv->entries) {
            free_conversation(conv);
            json_value_free(root);
            return NULL;
        }

        for (size_t i = 0; i < conv->entry_count; i++) {
            JsonValue *entry = json_get_array_item(entries, i);
            if (!entry || entry->type != JSON_OBJECT) continue;

            // Extract type
            JsonValue *type = json_get_object_value(entry, "type");
            if (type && type->type == JSON_STRING) {
                conv->entries[i].type = strdup(type->data.string);
            }

            // Extract text
            JsonValue *text = json_get_object_value(entry, "text");
            if (text && text->type == JSON_STRING) {
                conv->entries[i].text = strdup(text->data.string);
            }

            // Extract order
            JsonValue *order = json_get_object_value(entry, "order");
            if (order && order->type == JSON_NUMBER) {
                conv->entries[i].order = (int)order->data.number;
            }

            // Extract hasThoughts
            JsonValue *has_thoughts = json_get_object_value(entry, "hasThoughts");
            if (has_thoughts && has_thoughts->type == JSON_BOOLEAN) {
                conv->entries[i].has_thoughts = has_thoughts->data.boolean;
            }

            // Extract parentMessage (optional)
            conv->entries[i].parent_message = -1;
            JsonValue *parent = json_get_object_value(entry, "parentMessage");
            if (parent && parent->type == JSON_NUMBER) {
                conv->entries[i].parent_message = (int)parent->data.number;
            }
        }
    }

    json_value_free(root);
    return conv;
}

// Free conversation memory
void free_conversation(Conversation *conv) {
    if (!conv) return;

    free(conv->timestamp);
    free(conv->platform);

    if (conv->entries) {
        for (size_t i = 0; i < conv->entry_count; i++) {
            free(conv->entries[i].type);
            free(conv->entries[i].text);
        }
        free(conv->entries);
    }

    free(conv);
}

// Search for a term in the conversation
SearchResult* search_conversation(Conversation *conv, const char *search_term, size_t *result_count) {
    if (!conv || !search_term || !result_count) return NULL;

    *result_count = 0;

    // Convert search term to lowercase
    char *search_lower = to_lowercase(search_term);
    if (!search_lower) return NULL;

    // First pass: count matches
    size_t match_count = 0;
    for (size_t i = 0; i < conv->entry_count; i++) {
        if (!conv->entries[i].text) continue;

        char *text_lower = to_lowercase(conv->entries[i].text);
        if (!text_lower) continue;

        if (strstr(text_lower, search_lower)) {
            match_count++;
        }

        free(text_lower);
    }

    if (match_count == 0) {
        free(search_lower);
        return NULL;
    }

    // Allocate results array
    SearchResult *results = calloc(match_count, sizeof(SearchResult));
    if (!results) {
        free(search_lower);
        return NULL;
    }

    // Second pass: populate results
    size_t result_idx = 0;
    for (size_t i = 0; i < conv->entry_count; i++) {
        if (!conv->entries[i].text) continue;

        char *text_lower = to_lowercase(conv->entries[i].text);
        if (!text_lower) continue;

        char *match_pos = strstr(text_lower, search_lower);
        if (match_pos) {
            results[result_idx].entry = &conv->entries[i];
            results[result_idx].match_position = match_pos - text_lower;

            // Calculate context window (200 chars before and after)
            int text_len = strlen(conv->entries[i].text);
            results[result_idx].context_start = results[result_idx].match_position - 200;
            if (results[result_idx].context_start < 0) {
                results[result_idx].context_start = 0;
            }

            results[result_idx].context_end = results[result_idx].match_position + strlen(search_term) + 200;
            if (results[result_idx].context_end > text_len) {
                results[result_idx].context_end = text_len;
            }

            result_idx++;
        }

        free(text_lower);
    }

    free(search_lower);
    *result_count = match_count;
    return results;
}

// Free search results
void free_search_results(SearchResult *results, size_t count) {
    if (results) {
        free(results);
    }
}

// Print a search result with context
void print_search_result(SearchResult *result, int result_number, const char *search_term, const char *filename) {
    if (!result || !result->entry) return;

    ConversationEntry *entry = result->entry;

    printf("\n");
    printf("================================================================================\n");
    printf("Result #%d\n", result_number);
    printf("================================================================================\n");
    if (filename) {
        printf("File:     %s\n", filename);
    }
    printf("Type:     %s\n", entry->type ? entry->type : "Unknown");
    printf("Order:    %d\n", entry->order);

    if (entry->parent_message >= 0) {
        printf("Parent:   Message #%d\n", entry->parent_message);
    }

    printf("--------------------------------------------------------------------------------\n");

    if (entry->text) {
        int text_len = strlen(entry->text);

        // Print context
        if (text_len < 800) {
            // If text is short, print it all
            printf("%s\n", entry->text);
        } else {
            // Print context window
            if (result->context_start > 0) {
                printf("...");
            }

            // Print text from context_start to context_end
            for (int i = result->context_start; i < result->context_end && i < text_len; i++) {
                putchar(entry->text[i]);
            }

            if (result->context_end < text_len) {
                printf("...");
            }
            printf("\n");
        }
    }

    printf("================================================================================\n");
}

// Print conversation statistics
void print_conversation_stats(Conversation *conv) {
    if (!conv) return;

    printf("\n");
    printf("================================================================================\n");
    printf("CONVERSATION STATISTICS\n");
    printf("================================================================================\n");
    printf("Platform:      %s\n", conv->platform ? conv->platform : "Unknown");
    printf("Timestamp:     %s\n", conv->timestamp ? conv->timestamp : "Unknown");
    printf("Total Entries: %d\n", conv->total_count);
    printf("Messages:      %d\n", conv->message_count);
    printf("Thoughts:      %d\n", conv->thought_count);
    printf("================================================================================\n");
    printf("\n");
}