#include "query_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog_name) {
    printf("AI Chronicle Toolkit - aiquery\n");
    printf("===============================\n\n");
    printf("Search AI conversations with context and statistics.\n");
    printf("Works with JSON files created by md2json from AI Chronicle exports.\n\n");
    printf("Usage: %s [OPTIONS] <search_term> <json_file>\n\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -s, --stats    Show conversation statistics only\n");
    printf("  -l, --limit N  Limit results to N matches (default: show all)\n\n");
    printf("Arguments:\n");
    printf("  search_term    The term to search for (case-insensitive)\n");
    printf("  json_file      Path to the JSON conversation file\n\n");
    printf("Examples:\n");
    printf("  %s \"DPDK\" conversation.json              # Search for term\n", prog_name);
    printf("  %s \"neural network\" my-chat.json        # Multi-word search\n", prog_name);
    printf("  %s -s conversation.json                  # Show stats only\n", prog_name);
    printf("  %s -l 5 \"machine learning\" *.json       # First 5 results\n\n", prog_name);
    printf("Part of AI Chronicle Toolkit\n");
    printf("https://github.com/quantum-encoding/ai-chronicle-toolkit\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    const char *search_term = NULL;
    const char *json_file = NULL;
    bool stats_only = false;
    int result_limit = -1;  // -1 means no limit

    // Parse command line arguments
    int arg_idx = 1;
    while (arg_idx < argc) {
        if (strcmp(argv[arg_idx], "-h") == 0 || strcmp(argv[arg_idx], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[arg_idx], "-s") == 0 || strcmp(argv[arg_idx], "--stats") == 0) {
            stats_only = true;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "-l") == 0 || strcmp(argv[arg_idx], "--limit") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -l/--limit requires a number\n");
                return 1;
            }
            result_limit = atoi(argv[arg_idx + 1]);
            arg_idx += 2;
        } else if (search_term == NULL && !stats_only) {
            search_term = argv[arg_idx];
            arg_idx++;
        } else {
            json_file = argv[arg_idx];
            arg_idx++;
        }
    }

    // Validate arguments
    if (!stats_only && search_term == NULL) {
        fprintf(stderr, "Error: Search term required (use -h for help)\n");
        print_usage(argv[0]);
        return 1;
    }

    if (json_file == NULL) {
        fprintf(stderr, "Error: JSON file path required (use -h for help)\n");
        print_usage(argv[0]);
        return 1;
    }

    // Load conversation
    printf("Loading conversation from: %s\n", json_file);
    fflush(stdout);  // Ensure message is printed before any errors
    Conversation *conv = load_conversation(json_file);
    if (!conv) {
        return 1;
    }

    printf("Loaded successfully!\n");

    // Print statistics
    print_conversation_stats(conv);

    if (stats_only) {
        free_conversation(conv);
        return 0;
    }

    // Perform search
    printf("Searching for: \"%s\"\n", search_term);
    printf("\n");

    size_t result_count = 0;
    SearchResult *results = search_conversation(conv, search_term, &result_count);

    if (result_count == 0) {
        printf("No matches found for \"%s\"\n", search_term);
        free_conversation(conv);
        return 0;
    }

    // Print summary
    printf("Found %zu match%s\n", result_count, result_count == 1 ? "" : "es");

    // Apply limit if specified
    size_t display_count = result_count;
    if (result_limit > 0 && (size_t)result_limit < result_count) {
        display_count = result_limit;
        printf("Displaying first %d result%s\n", result_limit, result_limit == 1 ? "" : "s");
    }

    // Print results
    for (size_t i = 0; i < display_count; i++) {
        print_search_result(&results[i], i + 1, search_term, json_file);
    }

    // Print footer summary
    printf("\n");
    printf("================================================================================\n");
    printf("Search Summary\n");
    printf("================================================================================\n");
    printf("Search term:      \"%s\"\n", search_term);
    printf("Total matches:    %zu\n", result_count);
    printf("Results shown:    %zu\n", display_count);
    printf("File:             %s\n", json_file);
    printf("================================================================================\n");

    // Cleanup
    free_search_results(results, result_count);
    free_conversation(conv);

    return 0;
}