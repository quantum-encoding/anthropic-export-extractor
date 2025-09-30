/**
 * Anthropic Export Extractor
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 *
 * A production-grade tool for extracting and organizing conversations
 * from Anthropic Claude JSON exports into human-readable markdown files
 * with structured artifact management.
 */

#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#define MAX_PATH 2048
#define MAX_FILENAME 512

char g_root_output_dir[MAX_PATH];

typedef struct {
    char output_dir[MAX_PATH];
    char conv_name[MAX_FILENAME];
    FILE *markdown_file;
    FILE *manifest_file;
    int artifact_count;
    int external_file_count;
    int message_count;
} ConversationContext;

char* sanitize_filename(const char *name) {
    static char sanitized[MAX_FILENAME];
    int j = 0;

    for (int i = 0; name[i] && j < MAX_FILENAME - 1; i++) {
        unsigned char c = (unsigned char)name[i];

        if (c < 128 && (isalnum(c) || c == ' ' || c == '-' || c == '_')) {
            sanitized[j++] = (c == ' ') ? '_' : c;
        } else if (c >= 128) {
            sanitized[j++] = c;
        }
    }
    sanitized[j] = '\0';
    return sanitized;
}

int create_directory(const char *path) {
    #ifdef _WIN32
        return mkdir(path);
    #else
        return mkdir(path, 0755);
    #endif
}

int create_root_output_directory(const char *input_filename) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char base_name[256] = "conversations";
    const char *slash = strrchr(input_filename, '/');
    const char *basename = slash ? slash + 1 : input_filename;

    const char *dot = strrchr(basename, '.');
    if (dot && dot != basename) {
        size_t len = dot - basename;
        if (len < sizeof(base_name)) {
            strncpy(base_name, basename, len);
            base_name[len] = '\0';
        }
    }

    snprintf(g_root_output_dir, MAX_PATH, "extracted_%s_%04d-%02d-%02d_%02d-%02d-%02d",
             base_name,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    if (create_directory(g_root_output_dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create root output directory: %s\n", g_root_output_dir);
        return 0;
    }

    printf("Created root output directory: %s/\n", g_root_output_dir);
    return 1;
}

int create_output_structure(ConversationContext *ctx, const char *name, const char *uuid) {
    char *sanitized = sanitize_filename(name);
    strncpy(ctx->conv_name, sanitized, MAX_FILENAME - 1);
    ctx->conv_name[MAX_FILENAME - 1] = '\0';

    snprintf(ctx->output_dir, MAX_PATH, "%s/%s_%.8s", g_root_output_dir, sanitized, uuid);

    if (create_directory(ctx->output_dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create directory: %s\n", ctx->output_dir);
        return 0;
    }

    char artifacts_dir[MAX_PATH];
    snprintf(artifacts_dir, MAX_PATH, "%s/artifacts", ctx->output_dir);
    if (create_directory(artifacts_dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create artifacts directory\n");
        return 0;
    }

    char markdown_path[MAX_PATH];
    snprintf(markdown_path, MAX_PATH, "%s/%s.md", ctx->output_dir, sanitized);
    ctx->markdown_file = fopen(markdown_path, "w");
    if (!ctx->markdown_file) {
        fprintf(stderr, "Failed to create %s.md\n", sanitized);
        return 0;
    }

    char manifest_path[MAX_PATH];
    snprintf(manifest_path, MAX_PATH, "%s/manifest.json", ctx->output_dir);
    ctx->manifest_file = fopen(manifest_path, "w");
    if (!ctx->manifest_file) {
        fclose(ctx->markdown_file);
        fprintf(stderr, "Failed to create manifest.json\n");
        return 0;
    }

    ctx->artifact_count = 0;
    ctx->external_file_count = 0;
    ctx->message_count = 0;

    return 1;
}

void escape_json_string(FILE *f, const char *str) {
    if (!str) return;

    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fprintf(f, "\\\""); break;
            case '\\': fprintf(f, "\\\\"); break;
            case '\b': fprintf(f, "\\b"); break;
            case '\f': fprintf(f, "\\f"); break;
            case '\n': fprintf(f, "\\n"); break;
            case '\r': fprintf(f, "\\r"); break;
            case '\t': fprintf(f, "\\t"); break;
            default:
                fprintf(f, "%c", *p);
        }
    }
}

void write_markdown_header(ConversationContext *ctx, JsonValue *conversation) {
    JsonValue *name = json_get_object_value(conversation, "name");
    JsonValue *created = json_get_object_value(conversation, "created_at");
    JsonValue *uuid = json_get_object_value(conversation, "uuid");

    fprintf(ctx->markdown_file, "# %s\n\n",
            (name && name->type == JSON_STRING) ? name->data.string : "Untitled Conversation");

    if (created && created->type == JSON_STRING) {
        fprintf(ctx->markdown_file, "**Created:** %s\n\n", created->data.string);
    }

    if (uuid && uuid->type == JSON_STRING) {
        fprintf(ctx->markdown_file, "**UUID:** %s\n\n", uuid->data.string);
    }

    fprintf(ctx->markdown_file, "---\n\n");
}

void write_manifest_header(ConversationContext *ctx, JsonValue *conversation) {
    fprintf(ctx->manifest_file, "{\n");
    fprintf(ctx->manifest_file, "  \"conversation\": {\n");

    JsonValue *uuid = json_get_object_value(conversation, "uuid");
    JsonValue *name = json_get_object_value(conversation, "name");
    JsonValue *created = json_get_object_value(conversation, "created_at");
    JsonValue *updated = json_get_object_value(conversation, "updated_at");

    if (uuid && uuid->type == JSON_STRING) {
        fprintf(ctx->manifest_file, "    \"uuid\": \"");
        escape_json_string(ctx->manifest_file, uuid->data.string);
        fprintf(ctx->manifest_file, "\",\n");
    }
    if (name && name->type == JSON_STRING) {
        fprintf(ctx->manifest_file, "    \"name\": \"");
        escape_json_string(ctx->manifest_file, name->data.string);
        fprintf(ctx->manifest_file, "\",\n");
    }
    if (created && created->type == JSON_STRING) {
        fprintf(ctx->manifest_file, "    \"created_at\": \"");
        escape_json_string(ctx->manifest_file, created->data.string);
        fprintf(ctx->manifest_file, "\",\n");
    }
    if (updated && updated->type == JSON_STRING) {
        fprintf(ctx->manifest_file, "    \"updated_at\": \"");
        escape_json_string(ctx->manifest_file, updated->data.string);
        fprintf(ctx->manifest_file, "\"\n");
    }

    fprintf(ctx->manifest_file, "  },\n");
    fprintf(ctx->manifest_file, "  \"artifacts\": [\n");
}

int extract_attachment(ConversationContext *ctx, JsonValue *attachment, int msg_index) {
    JsonValue *filename = json_get_object_value(attachment, "file_name");
    JsonValue *content = json_get_object_value(attachment, "extracted_content");
    JsonValue *filetype = json_get_object_value(attachment, "file_type");

    if (!filename || filename->type != JSON_STRING) {
        return 0;
    }

    if (content && content->type == JSON_STRING) {
        char artifact_path[MAX_PATH];
        snprintf(artifact_path, MAX_PATH, "%s/artifacts/%s",
                 ctx->output_dir, filename->data.string);

        FILE *artifact = fopen(artifact_path, "w");
        if (artifact) {
            fprintf(artifact, "%s", content->data.string);
            fclose(artifact);

            if (ctx->artifact_count > 0) {
                fprintf(ctx->manifest_file, ",\n");
            }
            fprintf(ctx->manifest_file, "    {\n");
            fprintf(ctx->manifest_file, "      \"type\": \"attachment\",\n");
            fprintf(ctx->manifest_file, "      \"filename\": \"");
            escape_json_string(ctx->manifest_file, filename->data.string);
            fprintf(ctx->manifest_file, "\",\n");
            fprintf(ctx->manifest_file, "      \"message_index\": %d", msg_index);
            if (filetype && filetype->type == JSON_STRING) {
                fprintf(ctx->manifest_file, ",\n      \"file_type\": \"");
                escape_json_string(ctx->manifest_file, filetype->data.string);
                fprintf(ctx->manifest_file, "\"\n");
            } else {
                fprintf(ctx->manifest_file, "\n");
            }
            fprintf(ctx->manifest_file, "    }");

            ctx->artifact_count++;
            return 1;
        }
    }

    return 0;
}

void note_external_file(ConversationContext *ctx, JsonValue *file_ref, int msg_index) {
    JsonValue *filename = json_get_object_value(file_ref, "file_name");

    if (filename && filename->type == JSON_STRING) {
        if (ctx->artifact_count > 0 || ctx->external_file_count > 0) {
            fprintf(ctx->manifest_file, ",\n");
        }
        fprintf(ctx->manifest_file, "    {\n");
        fprintf(ctx->manifest_file, "      \"type\": \"external_reference\",\n");
        fprintf(ctx->manifest_file, "      \"filename\": \"");
        escape_json_string(ctx->manifest_file, filename->data.string);
        fprintf(ctx->manifest_file, "\",\n");
        fprintf(ctx->manifest_file, "      \"message_index\": %d,\n", msg_index);
        fprintf(ctx->manifest_file, "      \"note\": \"File not embedded in JSON export\"\n");
        fprintf(ctx->manifest_file, "    }");

        if (ctx->artifact_count == 0 && ctx->external_file_count == 0) {
            ctx->artifact_count = 1;
        }
        ctx->external_file_count++;
    }
}

void process_message(ConversationContext *ctx, JsonValue *message, int msg_index) {
    JsonValue *sender = json_get_object_value(message, "sender");
    JsonValue *text = json_get_object_value(message, "text");
    JsonValue *created = json_get_object_value(message, "created_at");
    JsonValue *uuid = json_get_object_value(message, "uuid");

    const char *sender_name = "Unknown";
    if (sender && sender->type == JSON_STRING) {
        sender_name = sender->data.string;
    }

    fprintf(ctx->markdown_file, "## Message %d: %s\n\n", msg_index + 1, sender_name);

    if (created && created->type == JSON_STRING) {
        fprintf(ctx->markdown_file, "**Timestamp:** %s\n\n", created->data.string);
    }

    if (uuid && uuid->type == JSON_STRING) {
        fprintf(ctx->markdown_file, "**UUID:** `%s`\n\n", uuid->data.string);
    }

    if (text && text->type == JSON_STRING) {
        fprintf(ctx->markdown_file, "%s\n\n", text->data.string);
    }

    JsonValue *attachments = json_get_object_value(message, "attachments");
    if (attachments && attachments->type == JSON_ARRAY && attachments->data.array.count > 0) {
        fprintf(ctx->markdown_file, "**Attachments:**\n");
        for (size_t i = 0; i < attachments->data.array.count; i++) {
            JsonValue *attachment = attachments->data.array.items[i];
            if (extract_attachment(ctx, attachment, msg_index)) {
                JsonValue *filename = json_get_object_value(attachment, "file_name");
                if (filename && filename->type == JSON_STRING) {
                    fprintf(ctx->markdown_file, "- `%s` (saved to artifacts/)\n", filename->data.string);
                }
            }
        }
        fprintf(ctx->markdown_file, "\n");
    }

    JsonValue *files = json_get_object_value(message, "files");
    if (files && files->type == JSON_ARRAY && files->data.array.count > 0) {
        fprintf(ctx->markdown_file, "**Referenced Files:**\n");
        for (size_t i = 0; i < files->data.array.count; i++) {
            JsonValue *file_ref = files->data.array.items[i];
            note_external_file(ctx, file_ref, msg_index);
            JsonValue *filename = json_get_object_value(file_ref, "file_name");
            if (filename && filename->type == JSON_STRING) {
                fprintf(ctx->markdown_file, "- `%s` (external reference)\n", filename->data.string);
            }
        }
        fprintf(ctx->markdown_file, "\n");
    }

    fprintf(ctx->markdown_file, "---\n\n");
}

void write_manifest_footer(ConversationContext *ctx) {
    fprintf(ctx->manifest_file, "\n  ],\n");
    fprintf(ctx->manifest_file, "  \"statistics\": {\n");
    fprintf(ctx->manifest_file, "    \"total_messages\": %d,\n", ctx->message_count);
    fprintf(ctx->manifest_file, "    \"total_artifacts\": %d,\n", ctx->artifact_count);
    fprintf(ctx->manifest_file, "    \"external_references\": %d\n", ctx->external_file_count);
    fprintf(ctx->manifest_file, "  }\n");
    fprintf(ctx->manifest_file, "}\n");
}

int process_conversation(JsonValue *conversation) {
    ConversationContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    JsonValue *name = json_get_object_value(conversation, "name");
    JsonValue *uuid = json_get_object_value(conversation, "uuid");

    const char *conv_name = (name && name->type == JSON_STRING) ?
                            name->data.string : "Untitled";
    const char *conv_uuid = (uuid && uuid->type == JSON_STRING) ?
                            uuid->data.string : "unknown";

    if (!create_output_structure(&ctx, conv_name, conv_uuid)) {
        return 0;
    }

    write_markdown_header(&ctx, conversation);
    write_manifest_header(&ctx, conversation);

    JsonValue *messages = json_get_object_value(conversation, "chat_messages");
    if (messages && messages->type == JSON_ARRAY) {
        for (size_t i = 0; i < messages->data.array.count; i++) {
            JsonValue *message = messages->data.array.items[i];
            if (message && message->type == JSON_OBJECT) {
                process_message(&ctx, message, i);
                ctx.message_count++;
            }
        }
    }

    write_manifest_footer(&ctx);

    fclose(ctx.markdown_file);
    fclose(ctx.manifest_file);

    printf("  [%d] %s (msg:%d art:%d ext:%d)\n",
           ctx.message_count, ctx.conv_name, ctx.message_count,
           ctx.artifact_count, ctx.external_file_count);

    return 1;
}

void print_help(const char *program_name) {
    printf("═══════════════════════════════════════════════════════\n");
    printf("   ANTHROPIC EXPORT EXTRACTOR\n");
    printf("   Author: Richard Tune <rich@quantumencoding.io>\n");
    printf("   Company: QUANTUM ENCODING LTD\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("DESCRIPTION:\n");
    printf("  Extracts conversations and artifacts from official Anthropic\n");
    printf("  Claude JSON exports (conversations.json) into organized,\n");
    printf("  human-readable markdown files with structured artifact\n");
    printf("  management.\n\n");

    printf("USAGE:\n");
    printf("  %s <conversations.json>\n\n", program_name);

    printf("ARGUMENTS:\n");
    printf("  <conversations.json>    Path to your Anthropic export file\n\n");

    printf("OPTIONS:\n");
    printf("  -h, --help              Display this help message\n\n");

    printf("OUTPUT:\n");
    printf("  Creates a timestamped directory containing:\n");
    printf("    • Markdown files for each conversation\n");
    printf("    • JSON manifests with metadata\n");
    printf("    • Extracted artifacts (code, images, attachments)\n\n");

    printf("EXAMPLE:\n");
    printf("  %s conversations.json\n\n", program_name);

    printf("HOW TO GET YOUR EXPORT:\n");
    printf("  1. Visit: https://claude.ai/settings/export\n");
    printf("  2. Request your data export\n");
    printf("  3. Download the conversations.json file\n");
    printf("  4. Run this tool on the downloaded file\n\n");

    printf("For more information, see README.md\n");
    printf("Report issues to: rich@quantumencoding.io\n\n");
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

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';
    fclose(file);

    printf("═══════════════════════════════════════════════════════\n");
    printf("   JSON CONVERSATION EXTRACTOR V2\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("Input: %s (%ld bytes)\n\n", argv[1], size);
    printf("Parsing JSON...\n");

    JsonValue *root = json_parse(content);
    free(content);

    if (!root) {
        fprintf(stderr, "Failed to parse JSON\n");
        return 1;
    }

    if (root->type != JSON_ARRAY) {
        fprintf(stderr, "Expected array of conversations at root\n");
        json_value_free(root);
        return 1;
    }

    printf("Found %zu conversations\n\n", root->data.array.count);

    if (!create_root_output_directory(argv[1])) {
        json_value_free(root);
        return 1;
    }

    printf("\nExtracting conversations:\n");
    printf("───────────────────────────────────────────────────────\n");

    int extracted = 0;
    for (size_t i = 0; i < root->data.array.count; i++) {
        JsonValue *conversation = root->data.array.items[i];
        if (conversation && conversation->type == JSON_OBJECT) {
            if (process_conversation(conversation)) {
                extracted++;
            }
        }
    }

    printf("───────────────────────────────────────────────────────\n");
    printf("\n✓ Extraction complete: %d/%zu conversations processed\n",
           extracted, root->data.array.count);
    printf("✓ Output directory: %s/\n\n", g_root_output_dir);

    json_value_free(root);
    return 0;
}