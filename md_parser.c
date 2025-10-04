#define _POSIX_C_SOURCE 200809L
#include "md_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper function to trim whitespace
static char* trim_whitespace(char *str) {
    if (!str) return NULL;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) return str;

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

// Helper function to duplicate string
static char* str_dup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, str, len + 1);
    }
    return dup;
}

// Helper function to extract value after a markdown bold marker
static char* extract_value(const char *line, const char *prefix) {
    const char *start = strstr(line, prefix);
    if (!start) return NULL;

    start += strlen(prefix);
    while (*start && isspace((unsigned char)*start)) start++;

    char *value = str_dup(start);
    if (value) {
        value = trim_whitespace(value);
    }
    return value;
}

// Add entry to conversation
static bool add_entry(MDConversation *conv, const char *type, const char *text, int order, bool has_thoughts, int parent_message) {
    if (conv->entry_count >= conv->entry_capacity) {
        size_t new_capacity = conv->entry_capacity == 0 ? 32 : conv->entry_capacity * 2;
        MDEntry *new_entries = realloc(conv->entries, new_capacity * sizeof(MDEntry));
        if (!new_entries) return false;
        conv->entries = new_entries;
        conv->entry_capacity = new_capacity;
    }

    MDEntry *entry = &conv->entries[conv->entry_count];
    entry->type = str_dup(type);
    entry->text = str_dup(text);
    entry->order = order;
    entry->has_thoughts = has_thoughts;
    entry->parent_message = parent_message;

    if (!entry->type || !entry->text) {
        free(entry->type);
        free(entry->text);
        return false;
    }

    conv->entry_count++;
    return true;
}

// Parse markdown file
MDConversation* md_parse_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    MDConversation *conv = calloc(1, sizeof(MDConversation));
    if (!conv) {
        fclose(file);
        return NULL;
    }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len;

    bool in_header = true;
    bool in_message = false;
    bool in_thoughts = false;

    char *current_text = NULL;
    size_t current_text_size = 0;
    size_t current_text_capacity = 0;

    int current_message_num = 0;
    int current_thoughts_parent = -1;
    int message_counter = 0;

    // Track which messages have thoughts
    bool *message_has_thoughts = calloc(10000, sizeof(bool)); // Max 10000 messages

    while ((line_len = getline(&line, &line_cap, file)) != -1) {
        char *trimmed = trim_whitespace(line);

        // Parse header
        if (in_header) {
            if (strncmp(trimmed, "**Captured:**", 13) == 0) {
                conv->metadata.timestamp = extract_value(trimmed, "**Captured:**");
            } else if (strncmp(trimmed, "**Total Blocks:**", 17) == 0) {
                char *val = extract_value(trimmed, "**Total Blocks:**");
                if (val) {
                    conv->metadata.total_blocks = atoi(val);
                    free(val);
                }
            } else if (strncmp(trimmed, "**Messages:**", 13) == 0) {
                char *val = extract_value(trimmed, "**Messages:**");
                if (val) {
                    conv->metadata.messages = atoi(val);
                    free(val);
                }
            } else if (strncmp(trimmed, "**Thought Sections:**", 21) == 0) {
                char *val = extract_value(trimmed, "**Thought Sections:**");
                if (val) {
                    conv->metadata.thoughts = atoi(val);
                    free(val);
                }
                in_header = false; // End of header
            }
            continue;
        }

        // Check for message start
        if (strncmp(trimmed, "## Message ", 11) == 0) {
            // Save previous message if exists
            if (in_message && current_text) {
                add_entry(conv, "MESSAGE", current_text, message_counter++,
                         message_has_thoughts[current_message_num], -1);
                free(current_text);
                current_text = NULL;
                current_text_size = 0;
                current_text_capacity = 0;
            }

            // Start new message
            current_message_num = atoi(trimmed + 11);
            in_message = true;
            in_thoughts = false;
            continue;
        }

        // Check for thoughts start
        if (strncmp(trimmed, "### 💭 Model Thoughts (Message ", 32) == 0) {
            // Save previous message or thoughts if exists
            if (in_message && current_text && current_text_size > 0) {
                add_entry(conv, "MESSAGE", current_text, message_counter++, true, -1);
                free(current_text);
                current_text = NULL;
                current_text_size = 0;
                current_text_capacity = 0;
            } else if (in_thoughts && current_text && current_text_size > 0) {
                add_entry(conv, "THOUGHTS", current_text, message_counter++, false, current_thoughts_parent);
                free(current_text);
                current_text = NULL;
                current_text_size = 0;
                current_text_capacity = 0;
            }

            // Extract parent message number
            const char *num_start = trimmed + 32;
            current_thoughts_parent = atoi(num_start);
            message_has_thoughts[current_thoughts_parent] = true;

            in_message = false;
            in_thoughts = true;
            continue;
        }

        // Handle separator lines - save current content
        if (strcmp(trimmed, "---") == 0) {
            // Save accumulated message or thoughts
            if (in_message && current_text && current_text_size > 0) {
                add_entry(conv, "MESSAGE", current_text, message_counter++,
                         message_has_thoughts[current_message_num], -1);
                free(current_text);
                current_text = NULL;
                current_text_size = 0;
                current_text_capacity = 0;
                in_message = false;
            } else if (in_thoughts && current_text && current_text_size > 0) {
                add_entry(conv, "THOUGHTS", current_text, message_counter++, false, current_thoughts_parent);
                free(current_text);
                current_text = NULL;
                current_text_size = 0;
                current_text_capacity = 0;
                in_thoughts = false;
            }
            continue;
        }

        // Remove "Expand to view model thoughts" suffix if present
        char *expand_marker = strstr(trimmed, " Expand to view model thoughts");
        if (expand_marker) {
            *expand_marker = '\0';  // Truncate the string at that point
        }

        // Skip blockquote marker for thoughts
        if (in_thoughts && trimmed[0] == '>') {
            trimmed++; // Skip the '>' character
            while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        }

        // Accumulate text
        if ((in_message || in_thoughts) && strlen(trimmed) > 0) {
            size_t len = strlen(trimmed);
            size_t needed = current_text_size + len + 2; // +2 for newline and null

            if (needed > current_text_capacity) {
                size_t new_capacity = current_text_capacity == 0 ? 4096 : current_text_capacity * 2;
                while (new_capacity < needed) new_capacity *= 2;

                char *new_text = realloc(current_text, new_capacity);
                if (!new_text) {
                    free(current_text);
                    free(line);
                    free(message_has_thoughts);
                    md_free_conversation(conv);
                    fclose(file);
                    return NULL;
                }
                current_text = new_text;
                current_text_capacity = new_capacity;

                // Initialize buffer on first allocation
                if (current_text_size == 0) {
                    current_text[0] = '\0';
                }
            }

            if (current_text && current_text_size == 0) {
                strcpy(current_text, trimmed);
                current_text_size = len;
            } else if (current_text) {
                current_text[current_text_size++] = '\n';
                strcpy(current_text + current_text_size, trimmed);
                current_text_size += len;
            }
        }

        // Save thoughts at end of section
        if (in_thoughts && strlen(trimmed) == 0 && current_text) {
            add_entry(conv, "THOUGHTS", current_text, message_counter++, false, current_thoughts_parent);
            free(current_text);
            current_text = NULL;
            current_text_size = 0;
            current_text_capacity = 0;
            in_thoughts = false;
        }
    }

    // Save final entry
    if (current_text) {
        if (in_message) {
            add_entry(conv, "MESSAGE", current_text, message_counter++,
                     message_has_thoughts[current_message_num], -1);
        } else if (in_thoughts) {
            add_entry(conv, "THOUGHTS", current_text, message_counter++, false, current_thoughts_parent);
        }
        free(current_text);
    }

    free(line);
    free(message_has_thoughts);
    fclose(file);
    return conv;
}

// Free conversation memory
void md_free_conversation(MDConversation *conv) {
    if (!conv) return;

    free(conv->metadata.timestamp);

    if (conv->entries) {
        for (size_t i = 0; i < conv->entry_count; i++) {
            free(conv->entries[i].type);
            free(conv->entries[i].text);
        }
        free(conv->entries);
    }

    free(conv);
}

// Escape JSON string
static void append_escaped_json(char **dest, size_t *dest_size, size_t *dest_capacity, const char *src) {
    if (!src) return;

    for (const char *p = src; *p; p++) {
        char c = *p;
        const char *escaped = NULL;
        char buf[7];

        switch (c) {
            case '"':  escaped = "\\\""; break;
            case '\\': escaped = "\\\\"; break;
            case '\b': escaped = "\\b"; break;
            case '\f': escaped = "\\f"; break;
            case '\n': escaped = "\\n"; break;
            case '\r': escaped = "\\r"; break;
            case '\t': escaped = "\\t"; break;
            default:
                if (c < 32) {
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    escaped = buf;
                } else {
                    buf[0] = c;
                    buf[1] = '\0';
                    escaped = buf;
                }
        }

        size_t len = strlen(escaped);
        size_t needed = *dest_size + len + 1;

        if (needed > *dest_capacity) {
            size_t new_capacity = *dest_capacity == 0 ? 4096 : *dest_capacity * 2;
            while (new_capacity < needed) new_capacity *= 2;

            char *new_dest = realloc(*dest, new_capacity);
            if (!new_dest) return;
            *dest = new_dest;
            *dest_capacity = new_capacity;
        }

        strcpy(*dest + *dest_size, escaped);
        *dest_size += len;
    }
}

// Convert to JSON
char* md_to_json(MDConversation *conv) {
    if (!conv) return NULL;

    size_t json_size = 0;
    size_t json_capacity = 8192;
    char *json = malloc(json_capacity);
    if (!json) return NULL;

    // Start JSON object
    snprintf(json, json_capacity, "{\n  \"timestamp\": \"%s\",\n",
             conv->metadata.timestamp ? conv->metadata.timestamp : "unknown");
    json_size = strlen(json);

    // Add platform
    const char *platform = "  \"platform\": \"Google AI Studio (Gemini)\",\n";
    size_t needed = json_size + strlen(platform) + 1;
    if (needed > json_capacity) {
        json_capacity *= 2;
        char *new_json = realloc(json, json_capacity);
        if (!new_json) { free(json); return NULL; }
        json = new_json;
    }
    strcat(json, platform);
    json_size += strlen(platform);

    // Add stats
    char stats[512];
    snprintf(stats, sizeof(stats),
             "  \"stats\": {\n"
             "    \"total\": %d,\n"
             "    \"messages\": %d,\n"
             "    \"thoughts\": %d\n"
             "  },\n",
             conv->metadata.total_blocks, conv->metadata.messages, conv->metadata.thoughts);

    needed = json_size + strlen(stats) + 1;
    if (needed > json_capacity) {
        json_capacity *= 2;
        char *new_json = realloc(json, json_capacity);
        if (!new_json) { free(json); return NULL; }
        json = new_json;
    }
    strcat(json, stats);
    json_size += strlen(stats);

    // Start entries array
    const char *entries_start = "  \"entries\": [\n";
    needed = json_size + strlen(entries_start) + 1;
    if (needed > json_capacity) {
        json_capacity *= 2;
        char *new_json = realloc(json, json_capacity);
        if (!new_json) { free(json); return NULL; }
        json = new_json;
    }
    strcat(json, entries_start);
    json_size += strlen(entries_start);

    // Add entries
    for (size_t i = 0; i < conv->entry_count; i++) {
        MDEntry *entry = &conv->entries[i];

        const char *entry_start = i == 0 ? "    {\n" : ",\n    {\n";
        needed = json_size + strlen(entry_start) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, entry_start);
        json_size += strlen(entry_start);

        // Add type
        char type_buf[256];
        snprintf(type_buf, sizeof(type_buf), "      \"type\": \"%s\",\n", entry->type);
        needed = json_size + strlen(type_buf) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, type_buf);
        json_size += strlen(type_buf);

        // Add text (escaped)
        const char *text_prefix = "      \"text\": \"";
        needed = json_size + strlen(text_prefix) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, text_prefix);
        json_size += strlen(text_prefix);

        append_escaped_json(&json, &json_size, &json_capacity, entry->text);

        const char *text_suffix = "\",\n";
        needed = json_size + strlen(text_suffix) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, text_suffix);
        json_size += strlen(text_suffix);

        // Add order
        char order_buf[256];
        snprintf(order_buf, sizeof(order_buf), "      \"order\": %d,\n", entry->order);
        needed = json_size + strlen(order_buf) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, order_buf);
        json_size += strlen(order_buf);

        // Add hasThoughts
        const char *has_thoughts = entry->has_thoughts ? "      \"hasThoughts\": true" : "      \"hasThoughts\": false";
        needed = json_size + strlen(has_thoughts) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, has_thoughts);
        json_size += strlen(has_thoughts);

        // Add parentMessage if applicable
        if (entry->parent_message >= 0) {
            char parent_buf[256];
            snprintf(parent_buf, sizeof(parent_buf), ",\n      \"parentMessage\": %d", entry->parent_message);
            needed = json_size + strlen(parent_buf) + 1;
            if (needed > json_capacity) {
                json_capacity *= 2;
                char *new_json = realloc(json, json_capacity);
                if (!new_json) { free(json); return NULL; }
                json = new_json;
            }
            strcat(json, parent_buf);
            json_size += strlen(parent_buf);
        }

        // Close entry
        const char *entry_end = "\n    }";
        needed = json_size + strlen(entry_end) + 1;
        if (needed > json_capacity) {
            json_capacity *= 2;
            char *new_json = realloc(json, json_capacity);
            if (!new_json) { free(json); return NULL; }
            json = new_json;
        }
        strcat(json, entry_end);
        json_size += strlen(entry_end);
    }

    // Close JSON
    const char *json_end = "\n  ]\n}\n";
    needed = json_size + strlen(json_end) + 1;
    if (needed > json_capacity) {
        json_capacity *= 2;
        char *new_json = realloc(json, json_capacity);
        if (!new_json) { free(json); return NULL; }
        json = new_json;
    }
    strcat(json, json_end);

    return json;
}

// Write JSON to file
bool md_write_json_file(MDConversation *conv, const char *output_file) {
    char *json = md_to_json(conv);
    if (!json) return false;

    FILE *file = fopen(output_file, "w");
    if (!file) {
        free(json);
        return false;
    }

    fputs(json, file);
    fclose(file);
    free(json);

    return true;
}