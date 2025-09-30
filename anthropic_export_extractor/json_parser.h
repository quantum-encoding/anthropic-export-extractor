/**
 * JSON Parser Library - Public API
 *
 * Author: Richard Tune <rich@quantumencoding.io>
 * Company: QUANTUM ENCODING LTD
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;
typedef struct JsonPair JsonPair;

struct JsonPair {
    char *key;
    JsonValue *value;
};

struct JsonValue {
    JsonType type;
    union {
        bool boolean;
        double number;
        char *string;
        struct {
            JsonValue **items;
            size_t count;
            size_t capacity;
        } array;
        struct {
            JsonPair *pairs;
            size_t count;
            size_t capacity;
        } object;
    } data;
};

JsonValue* json_parse(const char *input);
void json_value_free(JsonValue *value);
void json_print(JsonValue *value);
void json_print_value(FILE *file, JsonValue *value, int indent, bool pretty);
JsonValue* json_get_array_item(JsonValue *array, size_t index);
JsonValue* json_get_object_value(JsonValue *object, const char *key);

#endif