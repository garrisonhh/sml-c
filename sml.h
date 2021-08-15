#ifndef SML_H
#define SML_H

#include <stddef.h>

typedef struct sml_value sml_value_t;
struct sml_value {
    sml_value_t *next;

    enum {
        SML_STRING,
        SML_FLOAT,
        SML_INT,
        SML_TRUE,
        SML_FALSE,
        SML_NULL
    } type;

    union {
        const char *str;
        double f;
        long i;
    } value;
};

typedef struct sml_attribute sml_attribute_t;
struct sml_attribute {
    const char *name;
    sml_value_t *values;

    sml_attribute_t *next;
};

typedef struct sml_element sml_element_t;
struct sml_element {
    const char *name;
    sml_element_t *elements;
    sml_attribute_t *attributes;

    sml_element_t *next;
};

typedef struct sml_document {
    sml_element_t *root;

    // memory (don't touch unless you know what's up)
    char *text;
    char **allocated;
    size_t pages, alloc_pages; // list of pages
    size_t size_used; // used on current page
} sml_document_t;

void sml_load(sml_document_t *, const char *filename);
void sml_unload(sml_document_t *);

void sml_print(sml_document_t *);

#ifdef SML_IMPL

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// TODO allow for user replacements for memory stuff, like malloc/realloc/free, buffer sizes...

#define SML_FREAD_BUF_SIZE 4096
#define SML_ALLOCATOR_PAGE 4096
#define SML_MAX_TOKENS_PER_LINE 256
#define SML_MAX_TREE_DESCENT 256

#define SML_ERROR(...)\
    do {\
        fprintf(stderr, "SML ERROR: ");\
        fprintf(stderr, __VA_ARGS__);\
        exit(-1);\
    } while (0);

#define SML_LINE_ERROR(line, ...)\
    do {\
        fprintf(stderr, "SML ERROR on line %zu:\n", line + 1);\
        fprintf(stderr, __VA_ARGS__);\
        exit(0);\
    } while (0);

#define SML_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static void sml_parse(sml_document_t *doc, const char *filename);

void sml_load(sml_document_t *doc, const char *filename) {
    *doc = (sml_document_t){
        .text = NULL
    };

    doc->alloc_pages = 2;
    doc->allocated = malloc(doc->alloc_pages * sizeof(*doc->allocated));
    doc->allocated[doc->pages++] = malloc(SML_ALLOCATOR_PAGE);

    sml_parse(doc, filename);
}

void sml_unload(sml_document_t *doc) {
    free(doc->text);

    for (size_t i = 0; i < doc->pages; ++i)
        free(doc->allocated[i]);

    free(doc->allocated);
}

static void *sml_alloc(sml_document_t *doc, size_t bytes) {
    if (bytes + doc->size_used > SML_ALLOCATOR_PAGE) {
        if (doc->pages + 1 > doc->alloc_pages) {
            doc->alloc_pages <<= 1;
            doc->allocated = realloc(doc->allocated, doc->alloc_pages * sizeof(*doc->allocated));
        }

        doc->allocated[doc->pages++] = malloc(SML_ALLOCATOR_PAGE);
        doc->size_used = 0;
    }

    void *ptr = doc->allocated[doc->pages - 1] + doc->size_used;

    doc->size_used += bytes;

    return ptr;
}

static void sml_print_doc_lower(sml_element_t *root, int level) {
#define SML_PRINT_LVL(lvl)\
    for (size_t i = 0; i < (lvl) * 2; ++i)\
        putchar(' ');

    SML_PRINT_LVL(level);
    printf("<E> %s\n", root->name);

    for (sml_attribute_t *attrib = root->attributes; attrib; attrib = attrib->next) {
        SML_PRINT_LVL(level + 1);
        printf("<A> %s", attrib->name);

        for (sml_value_t *value = attrib->values; value; value = value->next) {
            putchar(' ');

            switch (value->type) {
            case SML_STRING:
                printf(value->value.str);
                break;
            case SML_FLOAT:
                printf("%f", value->value.f);
                break;
            case SML_INT:
                printf("%ld", value->value.i);
                break;
            case SML_TRUE:
                printf("true");
                break;
            case SML_FALSE:
                printf("false");
                break;
            case SML_NULL:
                printf("-");
                break;
            }
        }

        putchar('\n');
    }

    for (sml_element_t *elem = root->elements; elem; elem = elem->next)
        sml_print_doc_lower(elem, level + 1);

    SML_PRINT_LVL(level);
    puts("end");

#undef SML_PRINT_LVL
}

void sml_print(sml_document_t *doc) {
    sml_print_doc_lower(doc->root, 0);
}

static char *sml_read_file(const char *filename) {
    // open and check for existance
    FILE *file = fopen(filename, "r");

    if (!file)
        SML_ERROR("could not open file: \"%s\"\n", filename);

    // read text through buffer
    char *text = NULL;
    char buffer[SML_FREAD_BUF_SIZE];
    size_t num_read, text_len = 0;
    bool done = false;

    while (!done) {
        num_read = fread(buffer, sizeof(*buffer), SML_FREAD_BUF_SIZE, file);

        if (num_read != SML_FREAD_BUF_SIZE) {
            done = true;
            ++num_read;
        }

        // copy buffer into doc->text
        size_t last_len = text_len;

        text_len += num_read;
        text = realloc(text, text_len);

        memcpy(text + last_len, buffer, num_read * sizeof(*buffer));
    };

    text[text_len - 1] = '\0';

    fclose(file);

    return text;
}

static inline bool sml_is_whitespace(char ch) {
    const char whitespace[] = " \t";

    for (size_t i = 0; i < SML_ARRAY_SIZE(whitespace); ++i)
        if (ch == whitespace[i])
            return true;

    return false;
}

static bool sml_is_end(const char *str) {
    const char end[] = "end";

    for (size_t i = 0; i < SML_ARRAY_SIZE(end); ++i)
        if (!(str[i] == end[i] || str[i] == end[i] + ('A' - 'a')))
            return false;

    return true;
}

static bool sml_parse_num(sml_value_t *value) {
    long num = 0;
    double dot_div = 0.0;

    for (char *trav = (char *)value->value.str; *trav; ++trav) {
        if (*trav >= '0' && *trav <= '9') {
            num *= 10;
            num += *trav - '0';
            dot_div *= 10.0;
        } else if (*trav == '.') {
            dot_div = 1.0;
        } else {
            return false;
        }
    }

    if (dot_div) {
        value->type = SML_FLOAT;
        value->value.f = (double)num / dot_div;
    } else {
        value->type = SML_INT;
        value->value.i = num;
    }

    return true;
}

static bool sml_parse_string(sml_value_t *value) {
    char *str = (char *)value->value.str;

    if (str[0] == '\"') {
        // quoted string
        while (*++str) {
            if (!strncmp(str, "\"\"", 2)) {
                strcpy(str, str + 1);
            } else if (!strncmp(str, "\"/\"", 3)) {
                strcpy(str, str + 2);
                *str = '\n';
            } else if (*str == '\"' && *(str + 1) != '\0') {
                return false;
            }
        }

        if (*(str - 1) != '\"')
            return false;

        return true;
    }

    // unquoted string
    return true;
}

static void sml_parse_value(sml_value_t *value) {
    const char *str = value->value.str;

    if (!strcmp(str, "-"))
        value->type = SML_NULL;
     else if (!strcmp(str, "true"))
        value->type = SML_TRUE;
     else if (!strcmp(str, "false"))
        value->type = SML_FALSE;
     else if (!(sml_parse_num(value) || sml_parse_string(value)))
        SML_ERROR("unknown token: \"%s\"\n", str);
}

#define SML_LINKEDLIST_PUSH(root, item)\
    do {\
        item->next = root;\
        root = item;\
    } while (0)

static void sml_parse(sml_document_t *doc, const char *filename) {
    doc->text = sml_read_file(filename);

    // generation
    char *trav = doc->text;
    sml_element_t *parents[SML_MAX_TREE_DESCENT];
    sml_element_t *parent = NULL;
    size_t level = 0;

    // parse text to generate tree
    while (*trav) {
        // generate token list for this line
        char *tokens[SML_MAX_TOKENS_PER_LINE];
        size_t num_tokens = 0;
        bool whitespace = true, sml_string = false;

        while (*trav) {
            if (*trav == '\n') {
                // end of line
                *trav++ = '\0';
                break;
            } else if (*trav == '#') {
                // ignore the rest of the line
                *trav = '\0';

                while (*++trav != '\n')
                    ;

                *trav++ = '\0';
                break;
            } else if (sml_string) {
                // wait for end of string
                if (*trav == '\"') {
                    if (*(trav + 1) == '\"') // double quote escapestatic void sml_parse(sml_document_t *doc, const char *filename) {

                        ++trav;
                    else
                        sml_string = false;
                }
            } else {
                // no special state, check whitespace transitions
                bool next_whitespace = sml_is_whitespace(*trav);

                if (next_whitespace)
                    *trav = '\0';

                if (whitespace && !next_whitespace) {
                    tokens[num_tokens++] = trav;

                    if (*trav == '\"')
                        sml_string = true;
                }

                whitespace = next_whitespace;
            }

            ++trav;
        }

        // parse token list
        if (num_tokens == 1) {
            // must be element or `end` keyword
            if (sml_is_end(tokens[0])) {
                if (!--level)
                    break; // root element is done. TODO error check afterwards for extra tokens

                SML_LINKEDLIST_PUSH(parents[level - 1]->elements, parent);
                parent = parents[level - 1];
            } else {
                // push new parent element
                sml_element_t *elem = sml_alloc(doc, sizeof(*elem));

                elem->name = tokens[0];

                elem->elements = NULL;
                elem->attributes = NULL;
                elem->next = NULL;

                parent = parents[level++] = elem;
            }
        } else if (num_tokens > 1) {
            // push new attribute
            sml_attribute_t *attrib = sml_alloc(doc, sizeof(*attrib));

            attrib->name = tokens[0];
            attrib->values = NULL;
            attrib->next = NULL;

            SML_LINKEDLIST_PUSH(parent->attributes, attrib);

            // push values
            for (size_t j = num_tokens - 1; j >= 1; --j) {
                sml_value_t *value = sml_alloc(doc, sizeof(*value));

                // value types checked after parsing
                value->type = SML_STRING;
                value->value.str = tokens[j];

                sml_parse_value(value);

                SML_LINKEDLIST_PUSH(attrib->values, value);
            }
        }
    }

    doc->root = parent;
}

#endif // SML_IMPL
#endif // SML_H
