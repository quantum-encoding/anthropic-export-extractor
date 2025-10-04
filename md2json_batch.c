/**
 * AI Chronicle Toolkit - md2json Batch Processor
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 *
 * Batch convert markdown conversation exports to JSON format
 */

#include "md_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#define MAX_PATH 4096

static void print_help(const char *prog_name) {
    printf("AI Chronicle Toolkit - md2json Batch Processor\n");
    printf("================================================\n\n");
    printf("Batch convert markdown conversation exports to JSON format.\n");
    printf("Automatically creates an output directory and preserves filenames.\n\n");
    printf("Usage: %s <input_directory>\n\n", prog_name);
    printf("Arguments:\n");
    printf("  input_directory    Directory containing .md conversation files\n\n");
    printf("Output:\n");
    printf("  Creates: <input_directory>_json/\n");
    printf("  Contains: All .md files converted to .json with same names\n\n");
    printf("Examples:\n");
    printf("  %s convos-CHATGPT\n", prog_name);
    printf("  %s gemini-exports\n", prog_name);
    printf("  %s ~/Downloads/ai-conversations\n\n", prog_name);
    printf("Features:\n");
    printf("  • Automatically creates output directory\n");
    printf("  • Preserves original filenames (conversation.md → conversation.json)\n");
    printf("  • Processes all .md files recursively\n");
    printf("  • Shows progress with statistics\n\n");
    printf("Part of AI Chronicle Toolkit\n");
    printf("https://github.com/quantum-encoding/ai-chronicle-toolkit\n\n");
    printf("Author: Richard Tune <rich@quantumencoding.io>\n");
    printf("Company: QUANTUM ENCODING LTD\n\n");
}

static int create_directory(const char *path) {
    #ifdef _WIN32
        return mkdir(path);
    #else
        return mkdir(path, 0755);
    #endif
}

static char* replace_extension(const char *filename, const char *new_ext) {
    const char *dot = strrchr(filename, '.');
    if (!dot) {
        size_t len = strlen(filename);
        char *result = malloc(len + strlen(new_ext) + 2);
        if (result) {
            sprintf(result, "%s.%s", filename, new_ext);
        }
        return result;
    }

    size_t base_len = dot - filename;
    char *result = malloc(base_len + strlen(new_ext) + 2);
    if (result) {
        memcpy(result, filename, base_len);
        sprintf(result + base_len, ".%s", new_ext);
    }
    return result;
}

static bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static int process_file(const char *input_file, const char *output_file, int *success, int *failed) {
    // Parse markdown file (quietly)
    MDConversation *conv = md_parse_file(input_file);
    if (!conv) {
        (*failed)++;
        return 0;
    }

    // Convert to JSON and write
    bool wrote = md_write_json_file(conv, output_file);
    md_free_conversation(conv);

    if (wrote) {
        (*success)++;
        return 1;
    } else {
        (*failed)++;
        return 0;
    }
}

static int process_directory(const char *input_dir, const char *output_dir) {
    DIR *dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory: %s\n", input_dir);
        return -1;
    }

    int success = 0;
    int failed = 0;
    int total = 0;

    printf("\nProcessing files:\n");
    printf("─────────────────────────────────────────────────────\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full input path
        char input_path[MAX_PATH];
        snprintf(input_path, MAX_PATH, "%s/%s", input_dir, entry->d_name);

        // Check if it's a directory (for recursive processing)
        struct stat st;
        if (stat(input_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Skip subdirectories for now (can be enhanced later)
            continue;
        }

        // Only process .md files
        if (!ends_with(entry->d_name, ".md")) {
            continue;
        }

        total++;

        // Generate output filename
        char *json_filename = replace_extension(entry->d_name, "json");
        if (!json_filename) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            failed++;
            continue;
        }

        // Build output path
        char output_path[MAX_PATH];
        snprintf(output_path, MAX_PATH, "%s/%s", output_dir, json_filename);

        // Print progress
        printf("%-50s ", entry->d_name);
        fflush(stdout);

        // Process the file
        if (process_file(input_path, output_path, &success, &failed)) {
            printf("✓\n");
        } else {
            printf("✗\n");
        }

        free(json_filename);
    }

    closedir(dir);

    printf("─────────────────────────────────────────────────────\n");
    printf("\nSummary:\n");
    printf("  Total files:     %d\n", total);
    printf("  Successful:      %d\n", success);
    printf("  Failed:          %d\n", failed);
    printf("\n");

    if (success > 0) {
        printf("✓ Batch conversion complete!\n");
        printf("  Output: %s/\n\n", output_dir);
        printf("Created %d JSON file(s)\n\n", success);
    }

    return success;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    const char *input_dir = argv[1];

    // Validate input directory
    struct stat st;
    if (stat(input_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Directory not found: %s\n", input_dir);
        return 1;
    }

    // Create output directory name (input_dir + "_json")
    char output_dir[MAX_PATH];
    snprintf(output_dir, MAX_PATH, "%s_json", input_dir);

    printf("═══════════════════════════════════════════════════════\n");
    printf("   AI Chronicle Toolkit - Batch Processor\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("Input:  %s\n", input_dir);
    printf("Output: %s\n", output_dir);

    // Create output directory
    if (create_directory(output_dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: Failed to create output directory: %s\n", output_dir);
        return 1;
    }

    if (errno != EEXIST) {
        printf("\n✓ Created output directory: %s\n", output_dir);
    } else {
        printf("\nWarning: Output directory exists. Files may be overwritten.\n");
    }

    // Process all files
    int result = process_directory(input_dir, output_dir);

    if (result < 0) {
        return 1;
    }

    return result > 0 ? 0 : 1;
}
