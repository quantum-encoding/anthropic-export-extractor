/**
 * JSON Parser - Validation and Pretty-Print Tool
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 *
 * A command-line tool for validating and pretty-printing JSON files
 * using the libjson_parser library.
 */

#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void print_help(const char *program_name) {
    printf("═══════════════════════════════════════════════════════\n");
    printf("   JSON PARSER & VALIDATOR\n");
    printf("   Author: Richard Tune <rich@quantumencoding.io>\n");
    printf("   Company: QUANTUM ENCODING LTD\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("DESCRIPTION:\n");
    printf("  Validates and pretty-prints JSON files using a production-grade\n");
    printf("  RFC 8259 compliant JSON parser. Useful for checking JSON syntax,\n");
    printf("  formatting, and structure.\n\n");

    printf("USAGE:\n");
    printf("  %s [OPTIONS] <file.json>\n\n", program_name);

    printf("ARGUMENTS:\n");
    printf("  <file.json>             Path to JSON file to parse\n\n");

    printf("OPTIONS:\n");
    printf("  -h, --help              Display this help message\n");
    printf("  -v, --validate          Validate only (no output)\n");
    printf("  -p, --pretty            Pretty-print JSON (formatted)\n");
    printf("  -c, --compact           Compact JSON (minified)\n\n");

    printf("EXAMPLES:\n");
    printf("  # Validate a JSON file\n");
    printf("  %s data.json\n\n", program_name);

    printf("  # Validate only (exit code 0 = valid, 1 = invalid)\n");
    printf("  %s --validate config.json\n\n", program_name);

    printf("  # Pretty-print JSON\n");
    printf("  %s --pretty data.json\n\n", program_name);

    printf("  # Compact/minify JSON\n");
    printf("  %s --compact data.json\n\n", program_name);

    printf("EXIT CODES:\n");
    printf("  0    JSON is valid\n");
    printf("  1    JSON is invalid or file error\n\n");

    printf("FEATURES:\n");
    printf("  • RFC 8259 compliant JSON parser\n");
    printf("  • Comprehensive error detection\n");
    printf("  • Memory-safe implementation\n");
    printf("  • Handles large files efficiently\n");
    printf("  • Part of the libjson_parser.a library\n\n");

    printf("For more information, see README.md\n");
    printf("Report issues to: rich@quantumencoding.io\n\n");
}

int main(int argc, char *argv[]) {
    bool validate_only = false;
    bool pretty_print = false;
    bool compact = false;
    const char *filename = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--validate") == 0) {
            validate_only = true;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pretty") == 0) {
            pretty_print = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compact") == 0) {
            compact = true;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }

    if (!filename) {
        print_help(argv[0]);
        return 1;
    }

    // Read the JSON file
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file: %s\n", filename);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fprintf(stderr, "Error: File is empty or invalid: %s\n", filename);
        fclose(file);
        return 1;
    }

    char *content = malloc(size + 1);
    if (!content) {
        fprintf(stderr, "Error: Out of memory\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';
    fclose(file);

    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Warning: Expected %ld bytes, read %zu bytes\n", size, bytes_read);
    }

    // Parse the JSON
    if (!validate_only) {
        printf("Parsing: %s (%ld bytes)\n", filename, size);
    }

    JsonValue *value = json_parse(content);
    free(content);

    if (!value) {
        if (!validate_only) {
            fprintf(stderr, "\n✗ JSON parsing failed: Invalid syntax\n");
        }
        return 1;
    }

    if (validate_only) {
        printf("✓ Valid JSON\n");
    } else {
        printf("✓ Parse successful\n\n");

        if (pretty_print) {
            printf("Pretty-printed JSON:\n");
            printf("─────────────────────────────────────────\n");
            json_print_value(stdout, value, 0, true);
            printf("\n");
        } else if (compact) {
            printf("Compact JSON:\n");
            printf("─────────────────────────────────────────\n");
            json_print_value(stdout, value, 0, false);
            printf("\n");
        } else {
            printf("Parsed structure:\n");
            printf("─────────────────────────────────────────\n");
            json_print(value);
        }
    }

    json_value_free(value);
    return 0;
}