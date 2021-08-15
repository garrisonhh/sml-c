# sml-c

***sml-c is WIP, expect bugs and unexpected behavior.***

sml-c is an implementation of [SimpleML](https://www.simpleml.com/) by [Stefan John / Stenway](https://www.stenway.com/).

## usage

just like any single-header library:

```c
// in ONE .c file
#define SML_IMPL
#include "sml.h"

// everywhere else
#include "sml.h"
```

sml-c provides a simple api for loading sml files. the basic program structure looks like this:

```c
#define SML_IMPL
#include "sml.h"

int main() {
    sml_document_t doc;

    // load file from memory into the sml_document_t data structure
    sml_load(&doc, "example.sml");

    // do whatever you want with doc

    // free all memory associated with data structure
    sml_unload(&doc);

    return 0;
}
```

sml-c expects you to traverse the document data structure it provides yourself. the `sml_print` functions are short and clean examples for writing code to traverse the document and parse data, [see `sml.h`](https://github.com/garrisonhh/sml-c/blob/master/sml.h).

## documentation

#### `void sml_load(sml_document_t *, const char *filename)`

populate an sml_document_t with the sml file at filename.

#### `void sml_unload(sml_document_t *)`

frees all memory associated with the sml_document_t.

#### `void sml_print(sml_document_t *)`

prints SML tree as valid SML to stdout.

#### `void sml_fprint(sml_document_t *, FILE *file)`

prints SML tree as valid SML to a `FILE *`.

#### `SML_FOREACH(item, start_node)`
 data structures are generally linked list based, this macro just removes the linked list boilerplate. if you're not a fan of metaprogramming, it's unnecessary.

examples of usage:

```c
// assume this exists and is valid for the sake of the example
sml_element_t *root = ...;

// traversing sub elements of an element
sml_element_t *element;

SML_FOREACH(element, root->elements)
    /* do stuff */;

// traversing attributes of an element
sml_attribute_t *attrib;

SML_FOREACH(attrib, root->attributes)
    /* do stuff */;

// traversing values of an attribute
sml_attribute_t *attrib = ...; // assuming this is valid
sml_value_t *value;

SML_FOREACH(value, attrib->values)
    /* do stuff */;
```

### configuration

for memory and stack size concerns, sml-c provides macros to change certain settings.

```c
// configure before #including your SML implementation
#define SML_IMPL
#define SML_MALLOC(size) my_custom_malloc(size)
#include "sml.h"
```

#### `SML_FREAD_BUF_SIZE`

4096 by default. when reading in a file with `sml_load`, this is the size of the char array buffer.

#### `SML_ALLOCATOR_PAGE`

4096 by default. when allocating a new page for the sml_document allocator, this is the number of bytes allocated. if memory usage is higher than expected, lowering this value will typically result in less maximum memory usage but more `SML_MALLOC` calls.

#### `SML_MAX_TOKENS_PER_LINE`

256 by default. this is the number of tokens sml_parse stores per line. if one of your attributes has more than 255 values, increase this value to avoid an invalid write.

#### `SML_MAX_TREE_DESCENT`

256 by default. this is the number of nested elements sml_parse stores. if you have more than 256 levels of nesting, increase this value to avoid an invalid write.

#### `SML_MALLOC`, `SML_REALLOC`, `SML_FREE`

`malloc`, `realloc`, and `free` by default. sml-c will still use its page allocation model if these are redefined, but if you have your own custom allocator you would like to use below that, go ahead and redefine these.
