/**
 * AI Chronicle Toolkit - aiquery (Batch Mode)
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 *
 * Search AI conversations with batch directory processing
 */

#define _POSIX_C_SOURCE 200809L
#include "query_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define MAX_PATH 4096

typedef struct {
    char *filename;
    size_t match_count;        // Limited count for display
    size_t actual_result_count; // Actual allocated count
    SearchResult *results;
    Conversation *conv;        // Keep conversation alive for results
} FileResult;

typedef struct {
    FileResult *files;
    size_t file_count;
    size_t file_capacity;
    size_t total_matches;
} SearchReport;

static void print_usage(const char *prog_name) {
    printf("AI Chronicle Toolkit - aiquery (Batch Mode)\n");
    printf("============================================\n\n");
    printf("Search AI conversations across multiple files or entire directories.\n");
    printf("Works with JSON files created by md2json from AI Chronicle exports.\n\n");
    printf("Usage: %s [OPTIONS] <search_term> <json_file|directory>\n\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help       Show this help message\n");
    printf("  -s, --stats      Show conversation statistics only\n");
    printf("  -l, --limit N    Limit results per file to N matches\n");
    printf("  -f, --files      Only list files with matches (no content)\n");
    printf("  -o, --output MD  Export results to Markdown report file\n\n");
    printf("Arguments:\n");
    printf("  search_term    The term to search for (case-insensitive)\n");
    printf("  json_file      Path to a JSON file OR directory of JSON files\n\n");
    printf("Examples:\n");
    printf("  %s \"quantum\" conversation.json           # Single file\n", prog_name);
    printf("  %s \"mirror guard\" convos-chatGPT_json/  # Entire directory\n", prog_name);
    printf("  %s -f \"neural\" convos-chatGPT_json/     # List matching files only\n", prog_name);
    printf("  %s -l 3 \"AI\" chatgpt_json/              # Limit to 3 results per file\n\n", prog_name);
    printf("Part of AI Chronicle Toolkit\n");
    printf("https://github.com/quantum-encoding/ai-chronicle-toolkit\n\n");
    printf("Author: Richard Tune <rich@quantumencoding.io>\n");
    printf("Company: QUANTUM ENCODING LTD\n\n");
}

static bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// Escape markdown special characters
static void write_escaped_md(FILE *f, const char *text) {
    if (!text) return;
    for (const char *p = text; *p; p++) {
        // Escape special markdown characters
        if (*p == '*' || *p == '_' || *p == '`' || *p == '[' || *p == ']') {
            fputc('\\', f);
        }
        fputc(*p, f);
    }
}

// Export search results to Markdown file
static bool export_to_markdown(const char *output_file, const char *search_term,
                               SearchReport *report, const char *search_path) {
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not create output file: %s\n", output_file);
        return false;
    }

    // Get current time
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Header
    fprintf(f, "# AI Chronicle Search Report\n\n");
    fprintf(f, "**Generated:** %s  \n", timestamp);
    fprintf(f, "**Search Term:** \"%s\"  \n", search_term);
    fprintf(f, "**Search Path:** %s  \n", search_path);
    fprintf(f, "**Files Searched:** %zu  \n", report->file_count);
    fprintf(f, "**Files with Matches:** %zu  \n", report->file_count);
    fprintf(f, "**Total Matches:** %zu  \n\n", report->total_matches);

    fprintf(f, "---\n\n");

    // Table of Contents
    fprintf(f, "## Table of Contents\n\n");
    for (size_t i = 0; i < report->file_count; i++) {
        FileResult *fr = &report->files[i];
        fprintf(f, "%zu. [%s](#file-%zu) (%zu match%s)\n",
                i + 1, fr->filename, i + 1, fr->match_count,
                fr->match_count == 1 ? "" : "es");
    }
    fprintf(f, "\n---\n\n");

    // Results by file
    for (size_t i = 0; i < report->file_count; i++) {
        FileResult *fr = &report->files[i];

        fprintf(f, "<a name=\"file-%zu\"></a>\n", i + 1);
        fprintf(f, "## File %zu: %s\n\n", i + 1, fr->filename);
        fprintf(f, "**Matches:** %zu\n\n", fr->match_count);

        for (size_t j = 0; j < fr->match_count; j++) {
            SearchResult *sr = &fr->results[j];
            ConversationEntry *entry = sr->entry;

            fprintf(f, "### Result #%zu\n\n", j + 1);
            fprintf(f, "- **Type:** %s\n", entry->type ? entry->type : "Unknown");
            fprintf(f, "- **Order:** %d\n", entry->order);
            if (entry->parent_message >= 0) {
                fprintf(f, "- **Parent:** Message #%d\n", entry->parent_message);
            }
            fprintf(f, "\n");

            // Print context
            if (entry->text) {
                int text_len = strlen(entry->text);
                fprintf(f, "```\n");
                if (text_len < 800) {
                    fprintf(f, "%s\n", entry->text);
                } else {
                    if (sr->context_start > 0) {
                        fprintf(f, "...");
                    }
                    for (int k = sr->context_start; k < sr->context_end && k < text_len; k++) {
                        fputc(entry->text[k], f);
                    }
                    if (sr->context_end < text_len) {
                        fprintf(f, "...");
                    }
                    fprintf(f, "\n");
                }
                fprintf(f, "```\n\n");
            }
        }

        fprintf(f, "---\n\n");
    }

    // Footer
    fprintf(f, "## Summary\n\n");
    fprintf(f, "- **Search Term:** \"%s\"\n", search_term);
    fprintf(f, "- **Total Files Processed:** %zu\n", report->file_count);
    fprintf(f, "- **Total Matches Found:** %zu\n\n", report->total_matches);
    fprintf(f, "---\n\n");
    fprintf(f, "*Generated by AI Chronicle Toolkit - aiquery_batch*  \n");
    fprintf(f, "*Author: Richard Tune <rich@quantumencoding.io>*  \n");
    fprintf(f, "*Company: QUANTUM ENCODING LTD*\n");

    fclose(f);
    return true;
}

static int search_file(const char *filename, const char *search_term,
                       int result_limit, bool files_only, size_t *total_matches) {
    Conversation *conv = load_conversation(filename);
    if (!conv) {
        return 0;
    }

    size_t result_count = 0;
    SearchResult *results = search_conversation(conv, search_term, &result_count);

    if (result_count > 0) {
        *total_matches += result_count;

        if (files_only) {
            // Just print the filename and count
            printf("%-60s %zu match%s\n",
                   filename, result_count, result_count == 1 ? "" : "es");
        } else {
            // Print full results
            printf("\n");
            printf("════════════════════════════════════════════════════════════════════════════════\n");
            printf("File: %s\n", filename);
            printf("════════════════════════════════════════════════════════════════════════════════\n");
            printf("Found %zu match%s\n\n", result_count, result_count == 1 ? "" : "es");

            // Apply limit if specified
            size_t display_count = result_count;
            if (result_limit > 0 && (size_t)result_limit < result_count) {
                display_count = result_limit;
                printf("(Showing first %d result%s)\n\n",
                       result_limit, result_limit == 1 ? "" : "s");
            }

            // Print results
            for (size_t i = 0; i < display_count; i++) {
                print_search_result(&results[i], i + 1, search_term, filename);
            }
        }

        free_search_results(results, result_count);
    }

    free_conversation(conv);
    return result_count > 0 ? 1 : 0;
}

// Collect search results for export
static SearchReport* collect_search_results(const char *dir_path, const char *search_term,
                                            int result_limit) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory: %s\n", dir_path);
        return NULL;
    }

    SearchReport *report = calloc(1, sizeof(SearchReport));
    if (!report) {
        closedir(dir);
        return NULL;
    }

    report->file_capacity = 32;
    report->files = calloc(report->file_capacity, sizeof(FileResult));
    if (!report->files) {
        free(report);
        closedir(dir);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (!ends_with(entry->d_name, ".json")) {
            continue;
        }

        char file_path[MAX_PATH];
        snprintf(file_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }

        // Load and search
        Conversation *conv = load_conversation(file_path);
        if (!conv) continue;

        size_t result_count = 0;
        SearchResult *results = search_conversation(conv, search_term, &result_count);

        if (result_count > 0) {
            // Expand capacity if needed
            if (report->file_count >= report->file_capacity) {
                report->file_capacity *= 2;
                FileResult *new_files = realloc(report->files,
                                               report->file_capacity * sizeof(FileResult));
                if (!new_files) {
                    free_conversation(conv);
                    break;
                }
                report->files = new_files;
            }

            // Store results and conversation
            FileResult *fr = &report->files[report->file_count];
            fr->filename = strdup(file_path);
            fr->actual_result_count = result_count;
            fr->match_count = result_limit > 0 && (size_t)result_limit < result_count
                             ? (size_t)result_limit : result_count;
            fr->results = results;
            fr->conv = conv;  // Keep conversation alive

            report->total_matches += result_count;
            report->file_count++;
        } else {
            free_search_results(results, result_count);
            free_conversation(conv);
        }
    }

    closedir(dir);
    return report;
}

// Free search report
static void free_search_report(SearchReport *report) {
    if (!report) return;

    for (size_t i = 0; i < report->file_count; i++) {
        free(report->files[i].filename);
        free_search_results(report->files[i].results, report->files[i].actual_result_count);
        free_conversation(report->files[i].conv);
    }
    free(report->files);
    free(report);
}

static int search_directory(const char *dir_path, const char *search_term,
                           int result_limit, bool files_only) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory: %s\n", dir_path);
        return -1;
    }

    printf("═══════════════════════════════════════════════════════\n");
    printf("   AI Chronicle Toolkit - Batch Search\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("Directory:    %s\n", dir_path);
    printf("Search term:  \"%s\"\n", search_term);
    printf("\n");

    if (files_only) {
        printf("Matching Files:\n");
        printf("────────────────────────────────────────────────────────────────────────────────\n");
    }

    int files_processed = 0;
    int files_with_matches = 0;
    size_t total_matches = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Only process .json files
        if (!ends_with(entry->d_name, ".json")) {
            continue;
        }

        // Build full path
        char file_path[MAX_PATH];
        snprintf(file_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);

        // Check if it's a regular file
        struct stat st;
        if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }

        files_processed++;

        // Search the file
        if (search_file(file_path, search_term, result_limit, files_only, &total_matches)) {
            files_with_matches++;
        }
    }

    closedir(dir);

    // Print summary
    printf("\n");
    printf("════════════════════════════════════════════════════════════════════════════════\n");
    printf("Batch Search Summary\n");
    printf("════════════════════════════════════════════════════════════════════════════════\n");
    printf("Search term:        \"%s\"\n", search_term);
    printf("Directory:          %s\n", dir_path);
    printf("Files processed:    %d\n", files_processed);
    printf("Files with matches: %d\n", files_with_matches);
    printf("Total matches:      %zu\n", total_matches);
    printf("════════════════════════════════════════════════════════════════════════════════\n");

    return files_with_matches;
}

int main(int argc, char *argv[]) {
    const char *search_term = NULL;
    const char *target = NULL;
    const char *output_file = NULL;
    bool stats_only = false;
    bool files_only = false;
    int result_limit = -1;

    // Parse command line arguments
    int arg_idx = 1;
    while (arg_idx < argc) {
        if (strcmp(argv[arg_idx], "-h") == 0 || strcmp(argv[arg_idx], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[arg_idx], "-s") == 0 || strcmp(argv[arg_idx], "--stats") == 0) {
            stats_only = true;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "-f") == 0 || strcmp(argv[arg_idx], "--files") == 0) {
            files_only = true;
            arg_idx++;
        } else if (strcmp(argv[arg_idx], "-o") == 0 || strcmp(argv[arg_idx], "--output") == 0) {
            if (arg_idx + 1 >= argc) {
                fprintf(stderr, "Error: -o/--output requires a filename\n");
                return 1;
            }
            output_file = argv[arg_idx + 1];
            arg_idx += 2;
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
            target = argv[arg_idx];
            arg_idx++;
        }
    }

    // Validate arguments
    if (!stats_only && search_term == NULL) {
        fprintf(stderr, "Error: Search term required (use -h for help)\n");
        print_usage(argv[0]);
        return 1;
    }

    if (target == NULL) {
        fprintf(stderr, "Error: File or directory path required (use -h for help)\n");
        print_usage(argv[0]);
        return 1;
    }

    // Check if target is a directory or file
    struct stat st;
    if (stat(target, &st) != 0) {
        fprintf(stderr, "Error: Cannot access: %s\n", target);
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        // Export to MD if requested
        if (output_file) {
            printf("Collecting search results for export...\n");
            SearchReport *report = collect_search_results(target, search_term, result_limit);
            if (!report) {
                fprintf(stderr, "Error: Failed to collect search results\n");
                return 1;
            }

            printf("Found %zu matches across %zu files\n",
                   report->total_matches, report->file_count);
            printf("Exporting to: %s\n", output_file);

            if (export_to_markdown(output_file, search_term, report, target)) {
                printf("✓ Successfully exported to %s\n", output_file);
                free_search_report(report);
                return 0;
            } else {
                fprintf(stderr, "Error: Failed to export to Markdown\n");
                free_search_report(report);
                return 1;
            }
        }

        // Batch search directory (normal mode)
        int result = search_directory(target, search_term, result_limit, files_only);
        return result >= 0 ? 0 : 1;
    } else {
        // Single file search
        printf("Loading conversation from: %s\n", target);
        Conversation *conv = load_conversation(target);
        if (!conv) {
            return 1;
        }

        printf("Loaded successfully!\n");
        print_conversation_stats(conv);

        if (stats_only) {
            free_conversation(conv);
            return 0;
        }

        printf("Searching for: \"%s\"\n\n", search_term);

        size_t result_count = 0;
        SearchResult *results = search_conversation(conv, search_term, &result_count);

        if (result_count == 0) {
            printf("No matches found for \"%s\"\n", search_term);
            free_conversation(conv);
            return 0;
        }

        printf("Found %zu match%s\n\n", result_count, result_count == 1 ? "" : "es");

        // Apply limit
        size_t display_count = result_count;
        if (result_limit > 0 && (size_t)result_limit < result_count) {
            display_count = result_limit;
            printf("(Displaying first %d result%s)\n\n",
                   result_limit, result_limit == 1 ? "" : "s");
        }

        // Print results
        for (size_t i = 0; i < display_count; i++) {
            print_search_result(&results[i], i + 1, search_term, target);
        }

        printf("\n════════════════════════════════════════════════════════════════════════════════\n");
        printf("Search Summary\n");
        printf("════════════════════════════════════════════════════════════════════════════════\n");
        printf("Search term:      \"%s\"\n", search_term);
        printf("Total matches:    %zu\n", result_count);
        printf("Results shown:    %zu\n", display_count);
        printf("File:             %s\n", target);
        printf("════════════════════════════════════════════════════════════════════════════════\n");

        free_search_results(results, result_count);
        free_conversation(conv);
    }

    return 0;
}
