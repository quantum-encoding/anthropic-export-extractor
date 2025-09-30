/**
 * JSON Parser Library
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 *
 * A robust, production-grade JSON parser implementation in C
 * with full RFC 8259 compliance and comprehensive error handling.
 */

#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#define MAX_DEPTH 128
#define INITIAL_CAPACITY 16
#define MAX_STRING_SIZE 2097152
#define MAX_NUMBER_SIZE 64

typedef struct {
    const char *input;
    size_t position;
    size_t length;
    int line;
    int column;
    char error[256];
    int depth;
} Parser;

JsonValue* json_value_create(JsonType type) {
    JsonValue *value = calloc(1, sizeof(JsonValue));
    if (!value) return NULL;

    value->type = type;

    if (type == JSON_ARRAY) {
        value->data.array.capacity = INITIAL_CAPACITY;
        value->data.array.items = calloc(INITIAL_CAPACITY, sizeof(JsonValue*));
        if (!value->data.array.items) {
            free(value);
            return NULL;
        }
    } else if (type == JSON_OBJECT) {
        value->data.object.capacity = INITIAL_CAPACITY;
        value->data.object.pairs = calloc(INITIAL_CAPACITY, sizeof(JsonPair));
        if (!value->data.object.pairs) {
            free(value);
            return NULL;
        }
    }

    return value;
}

void json_value_free(JsonValue *value) {
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->data.string);
            break;

        case JSON_ARRAY:
            for (size_t i = 0; i < value->data.array.count; i++) {
                json_value_free(value->data.array.items[i]);
            }
            free(value->data.array.items);
            break;

        case JSON_OBJECT:
            for (size_t i = 0; i < value->data.object.count; i++) {
                free(value->data.object.pairs[i].key);
                json_value_free(value->data.object.pairs[i].value);
            }
            free(value->data.object.pairs);
            break;

        default:
            break;
    }

    free(value);
}

void skip_whitespace(Parser *parser) {
    while (parser->position < parser->length) {
        char c = parser->input[parser->position];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (c == '\n') {
                parser->line++;
                parser->column = 0;
            } else {
                parser->column++;
            }
            parser->position++;
        } else {
            break;
        }
    }
}

bool peek_char(Parser *parser, char expected) {
    skip_whitespace(parser);
    if (parser->position >= parser->length) return false;
    return parser->input[parser->position] == expected;
}

bool consume_char(Parser *parser, char expected) {
    skip_whitespace(parser);
    if (parser->position >= parser->length) {
        snprintf(parser->error, sizeof(parser->error),
                "Unexpected end of input at line %d, column %d",
                parser->line, parser->column);
        return false;
    }

    if (parser->input[parser->position] != expected) {
        snprintf(parser->error, sizeof(parser->error),
                "Expected '%c' at line %d, column %d, got '%c'",
                expected, parser->line, parser->column,
                parser->input[parser->position]);
        return false;
    }

    parser->position++;
    parser->column++;
    return true;
}

JsonValue* parse_value(Parser *parser);

JsonValue* parse_null(Parser *parser) {
    if (parser->position + 4 <= parser->length &&
        strncmp(parser->input + parser->position, "null", 4) == 0) {
        parser->position += 4;
        parser->column += 4;
        return json_value_create(JSON_NULL);
    }

    snprintf(parser->error, sizeof(parser->error),
            "Invalid null value at line %d, column %d",
            parser->line, parser->column);
    return NULL;
}

JsonValue* parse_boolean(Parser *parser) {
    JsonValue *value = json_value_create(JSON_BOOLEAN);
    if (!value) return NULL;

    if (parser->position + 4 <= parser->length &&
        strncmp(parser->input + parser->position, "true", 4) == 0) {
        value->data.boolean = true;
        parser->position += 4;
        parser->column += 4;
        return value;
    }

    if (parser->position + 5 <= parser->length &&
        strncmp(parser->input + parser->position, "false", 5) == 0) {
        value->data.boolean = false;
        parser->position += 5;
        parser->column += 5;
        return value;
    }

    json_value_free(value);
    snprintf(parser->error, sizeof(parser->error),
            "Invalid boolean value at line %d, column %d",
            parser->line, parser->column);
    return NULL;
}

JsonValue* parse_number(Parser *parser) {
    char buffer[MAX_NUMBER_SIZE];
    size_t buffer_pos = 0;

    if (parser->input[parser->position] == '-') {
        buffer[buffer_pos++] = parser->input[parser->position++];
        parser->column++;
    }

    if (parser->position >= parser->length || !isdigit(parser->input[parser->position])) {
        snprintf(parser->error, sizeof(parser->error),
                "Invalid number at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    if (parser->input[parser->position] == '0') {
        buffer[buffer_pos++] = parser->input[parser->position++];
        parser->column++;

        if (parser->position < parser->length && isdigit(parser->input[parser->position])) {
            snprintf(parser->error, sizeof(parser->error),
                    "Leading zeros not allowed at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        }
    } else {
        while (parser->position < parser->length && isdigit(parser->input[parser->position])) {
            if (buffer_pos >= MAX_NUMBER_SIZE - 1) {
                snprintf(parser->error, sizeof(parser->error),
                        "Number too large at line %d, column %d",
                        parser->line, parser->column);
                return NULL;
            }
            buffer[buffer_pos++] = parser->input[parser->position++];
            parser->column++;
        }
    }

    if (parser->position < parser->length && parser->input[parser->position] == '.') {
        buffer[buffer_pos++] = parser->input[parser->position++];
        parser->column++;

        if (parser->position >= parser->length || !isdigit(parser->input[parser->position])) {
            snprintf(parser->error, sizeof(parser->error),
                    "Invalid decimal number at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        }

        while (parser->position < parser->length && isdigit(parser->input[parser->position])) {
            if (buffer_pos >= MAX_NUMBER_SIZE - 1) {
                snprintf(parser->error, sizeof(parser->error),
                        "Number too large at line %d, column %d",
                        parser->line, parser->column);
                return NULL;
            }
            buffer[buffer_pos++] = parser->input[parser->position++];
            parser->column++;
        }
    }

    if (parser->position < parser->length &&
        (parser->input[parser->position] == 'e' || parser->input[parser->position] == 'E')) {
        buffer[buffer_pos++] = parser->input[parser->position++];
        parser->column++;

        if (parser->position < parser->length &&
            (parser->input[parser->position] == '+' || parser->input[parser->position] == '-')) {
            buffer[buffer_pos++] = parser->input[parser->position++];
            parser->column++;
        }

        if (parser->position >= parser->length || !isdigit(parser->input[parser->position])) {
            snprintf(parser->error, sizeof(parser->error),
                    "Invalid exponent at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        }

        while (parser->position < parser->length && isdigit(parser->input[parser->position])) {
            if (buffer_pos >= MAX_NUMBER_SIZE - 1) {
                snprintf(parser->error, sizeof(parser->error),
                        "Number too large at line %d, column %d",
                        parser->line, parser->column);
                return NULL;
            }
            buffer[buffer_pos++] = parser->input[parser->position++];
            parser->column++;
        }
    }

    buffer[buffer_pos] = '\0';

    JsonValue *value = json_value_create(JSON_NUMBER);
    if (!value) return NULL;

    char *endptr;
    value->data.number = strtod(buffer, &endptr);

    if (*endptr != '\0' || errno == ERANGE) {
        json_value_free(value);
        snprintf(parser->error, sizeof(parser->error),
                "Invalid number format at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    return value;
}

int parse_hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

JsonValue* parse_string(Parser *parser) {
    if (!consume_char(parser, '"')) return NULL;

    char *buffer = malloc(MAX_STRING_SIZE);
    if (!buffer) return NULL;

    size_t buffer_pos = 0;

    while (parser->position < parser->length && parser->input[parser->position] != '"') {
        if (buffer_pos >= MAX_STRING_SIZE - 1) {
            free(buffer);
            snprintf(parser->error, sizeof(parser->error),
                    "String too long at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        }

        if (parser->input[parser->position] == '\\') {
            parser->position++;
            parser->column++;

            if (parser->position >= parser->length) {
                free(buffer);
                snprintf(parser->error, sizeof(parser->error),
                        "Unterminated string at line %d, column %d",
                        parser->line, parser->column);
                return NULL;
            }

            switch (parser->input[parser->position]) {
                case '"':  buffer[buffer_pos++] = '"'; break;
                case '\\': buffer[buffer_pos++] = '\\'; break;
                case '/':  buffer[buffer_pos++] = '/'; break;
                case 'b':  buffer[buffer_pos++] = '\b'; break;
                case 'f':  buffer[buffer_pos++] = '\f'; break;
                case 'n':  buffer[buffer_pos++] = '\n'; break;
                case 'r':  buffer[buffer_pos++] = '\r'; break;
                case 't':  buffer[buffer_pos++] = '\t'; break;
                case 'u': {
                    parser->position++;
                    parser->column++;

                    int codepoint = 0;
                    for (int i = 0; i < 4; i++) {
                        if (parser->position >= parser->length) {
                            free(buffer);
                            snprintf(parser->error, sizeof(parser->error),
                                    "Invalid unicode escape at line %d, column %d",
                                    parser->line, parser->column);
                            return NULL;
                        }

                        int digit = parse_hex_digit(parser->input[parser->position]);
                        if (digit < 0) {
                            free(buffer);
                            snprintf(parser->error, sizeof(parser->error),
                                    "Invalid hex digit at line %d, column %d",
                                    parser->line, parser->column);
                            return NULL;
                        }

                        codepoint = (codepoint << 4) | digit;
                        parser->position++;
                        parser->column++;
                    }

                    if (codepoint < 0x80) {
                        buffer[buffer_pos++] = codepoint;
                    } else if (codepoint < 0x800) {
                        buffer[buffer_pos++] = 0xC0 | (codepoint >> 6);
                        buffer[buffer_pos++] = 0x80 | (codepoint & 0x3F);
                    } else {
                        buffer[buffer_pos++] = 0xE0 | (codepoint >> 12);
                        buffer[buffer_pos++] = 0x80 | ((codepoint >> 6) & 0x3F);
                        buffer[buffer_pos++] = 0x80 | (codepoint & 0x3F);
                    }
                    parser->position--;
                    parser->column--;
                    break;
                }
                default:
                    free(buffer);
                    snprintf(parser->error, sizeof(parser->error),
                            "Invalid escape sequence at line %d, column %d",
                            parser->line, parser->column);
                    return NULL;
            }
            parser->position++;
            parser->column++;
        } else if ((unsigned char)parser->input[parser->position] < 0x20) {
            free(buffer);
            snprintf(parser->error, sizeof(parser->error),
                    "Invalid control character in string at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        } else {
            buffer[buffer_pos++] = parser->input[parser->position++];
            parser->column++;
        }
    }

    if (parser->position >= parser->length) {
        free(buffer);
        snprintf(parser->error, sizeof(parser->error),
                "Unterminated string at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    parser->position++;
    parser->column++;

    buffer[buffer_pos] = '\0';

    JsonValue *value = json_value_create(JSON_STRING);
    if (!value) {
        free(buffer);
        return NULL;
    }

    char *resized = realloc(buffer, buffer_pos + 1);
    if (resized) {
        value->data.string = resized;
    } else {
        value->data.string = buffer;
    }

    return value;
}

JsonValue* parse_array(Parser *parser) {
    if (!consume_char(parser, '[')) return NULL;

    parser->depth++;
    if (parser->depth > MAX_DEPTH) {
        snprintf(parser->error, sizeof(parser->error),
                "Maximum nesting depth exceeded at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    JsonValue *array = json_value_create(JSON_ARRAY);
    if (!array) return NULL;

    skip_whitespace(parser);

    if (peek_char(parser, ']')) {
        consume_char(parser, ']');
        parser->depth--;
        return array;
    }

    while (1) {
        JsonValue *item = parse_value(parser);
        if (!item) {
            json_value_free(array);
            return NULL;
        }

        if (array->data.array.count >= array->data.array.capacity) {
            size_t new_capacity = array->data.array.capacity * 2;
            JsonValue **new_items = realloc(array->data.array.items,
                                          new_capacity * sizeof(JsonValue*));
            if (!new_items) {
                json_value_free(item);
                json_value_free(array);
                return NULL;
            }
            array->data.array.items = new_items;
            array->data.array.capacity = new_capacity;
        }

        array->data.array.items[array->data.array.count++] = item;

        skip_whitespace(parser);

        if (peek_char(parser, ']')) {
            consume_char(parser, ']');
            break;
        }

        if (!consume_char(parser, ',')) {
            json_value_free(array);
            return NULL;
        }
    }

    parser->depth--;
    return array;
}

JsonValue* parse_object(Parser *parser) {
    if (!consume_char(parser, '{')) return NULL;

    parser->depth++;
    if (parser->depth > MAX_DEPTH) {
        snprintf(parser->error, sizeof(parser->error),
                "Maximum nesting depth exceeded at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    JsonValue *object = json_value_create(JSON_OBJECT);
    if (!object) return NULL;

    skip_whitespace(parser);

    if (peek_char(parser, '}')) {
        consume_char(parser, '}');
        parser->depth--;
        return object;
    }

    while (1) {
        JsonValue *key_value = parse_string(parser);
        if (!key_value || key_value->type != JSON_STRING) {
            if (key_value) json_value_free(key_value);
            json_value_free(object);
            snprintf(parser->error, sizeof(parser->error),
                    "Expected string key at line %d, column %d",
                    parser->line, parser->column);
            return NULL;
        }

        char *key = key_value->data.string;
        key_value->data.string = NULL;
        json_value_free(key_value);

        if (!consume_char(parser, ':')) {
            free(key);
            json_value_free(object);
            return NULL;
        }

        JsonValue *value = parse_value(parser);
        if (!value) {
            free(key);
            json_value_free(object);
            return NULL;
        }

        if (object->data.object.count >= object->data.object.capacity) {
            size_t new_capacity = object->data.object.capacity * 2;
            JsonPair *new_pairs = realloc(object->data.object.pairs,
                                        new_capacity * sizeof(JsonPair));
            if (!new_pairs) {
                free(key);
                json_value_free(value);
                json_value_free(object);
                return NULL;
            }
            object->data.object.pairs = new_pairs;
            object->data.object.capacity = new_capacity;
        }

        object->data.object.pairs[object->data.object.count].key = key;
        object->data.object.pairs[object->data.object.count].value = value;
        object->data.object.count++;

        skip_whitespace(parser);

        if (peek_char(parser, '}')) {
            consume_char(parser, '}');
            break;
        }

        if (!consume_char(parser, ',')) {
            json_value_free(object);
            return NULL;
        }
    }

    parser->depth--;
    return object;
}

JsonValue* parse_value(Parser *parser) {
    skip_whitespace(parser);

    if (parser->position >= parser->length) {
        snprintf(parser->error, sizeof(parser->error),
                "Unexpected end of input at line %d, column %d",
                parser->line, parser->column);
        return NULL;
    }

    char c = parser->input[parser->position];

    if (c == 'n') return parse_null(parser);
    if (c == 't' || c == 'f') return parse_boolean(parser);
    if (c == '"') return parse_string(parser);
    if (c == '[') return parse_array(parser);
    if (c == '{') return parse_object(parser);
    if (c == '-' || isdigit(c)) return parse_number(parser);

    snprintf(parser->error, sizeof(parser->error),
            "Unexpected character '%c' at line %d, column %d",
            c, parser->line, parser->column);
    return NULL;
}

JsonValue* json_parse(const char *input) {
    if (!input) return NULL;

    Parser parser = {
        .input = input,
        .position = 0,
        .length = strlen(input),
        .line = 1,
        .column = 1,
        .depth = 0
    };

    JsonValue *value = parse_value(&parser);

    if (value) {
        skip_whitespace(&parser);
        if (parser.position < parser.length) {
            json_value_free(value);
            snprintf(parser.error, sizeof(parser.error),
                    "Unexpected data after JSON at line %d, column %d",
                    parser.line, parser.column);
            fprintf(stderr, "JSON Parse Error: %s\n", parser.error);
            return NULL;
        }
    } else {
        fprintf(stderr, "JSON Parse Error: %s\n", parser.error);
    }

    return value;
}

void json_print_indent(FILE *file, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(file, "  ");
    }
}

void json_print_value(FILE *file, JsonValue *value, int indent, bool pretty) {
    if (!value) {
        fprintf(file, "null");
        return;
    }

    switch (value->type) {
        case JSON_NULL:
            fprintf(file, "null");
            break;

        case JSON_BOOLEAN:
            fprintf(file, value->data.boolean ? "true" : "false");
            break;

        case JSON_NUMBER:
            if (value->data.number == floor(value->data.number) &&
                fabs(value->data.number) < 1e10) {
                fprintf(file, "%.0f", value->data.number);
            } else {
                fprintf(file, "%g", value->data.number);
            }
            break;

        case JSON_STRING:
            fprintf(file, "\"%s\"", value->data.string);
            break;

        case JSON_ARRAY:
            fprintf(file, "[");
            if (pretty && value->data.array.count > 0) fprintf(file, "\n");

            for (size_t i = 0; i < value->data.array.count; i++) {
                if (pretty) json_print_indent(file, indent + 1);
                json_print_value(file, value->data.array.items[i], indent + 1, pretty);
                if (i < value->data.array.count - 1) {
                    fprintf(file, ",");
                }
                if (pretty) fprintf(file, "\n");
            }

            if (pretty && value->data.array.count > 0) json_print_indent(file, indent);
            fprintf(file, "]");
            break;

        case JSON_OBJECT:
            fprintf(file, "{");
            if (pretty && value->data.object.count > 0) fprintf(file, "\n");

            for (size_t i = 0; i < value->data.object.count; i++) {
                if (pretty) json_print_indent(file, indent + 1);
                fprintf(file, "\"%s\":", value->data.object.pairs[i].key);
                if (pretty) fprintf(file, " ");
                json_print_value(file, value->data.object.pairs[i].value, indent + 1, pretty);
                if (i < value->data.object.count - 1) {
                    fprintf(file, ",");
                }
                if (pretty) fprintf(file, "\n");
            }

            if (pretty && value->data.object.count > 0) json_print_indent(file, indent);
            fprintf(file, "}");
            break;
    }
}

void json_print(JsonValue *value) {
    json_print_value(stdout, value, 0, true);
    printf("\n");
}

JsonValue* json_get_array_item(JsonValue *array, size_t index) {
    if (!array || array->type != JSON_ARRAY) return NULL;
    if (index >= array->data.array.count) return NULL;
    return array->data.array.items[index];
}

JsonValue* json_get_object_value(JsonValue *object, const char *key) {
    if (!object || object->type != JSON_OBJECT || !key) return NULL;

    for (size_t i = 0; i < object->data.object.count; i++) {
        if (strcmp(object->data.object.pairs[i].key, key) == 0) {
            return object->data.object.pairs[i].value;
        }
    }

    return NULL;
}