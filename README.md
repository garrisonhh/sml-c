# sml-c

sml-c is an implementation of [SimpleML](https://www.simpleml.com/) by [Stefan John / Stenway](https://www.stenway.com/).

## getting started

just like any single-header library, to include in your program, include once with `#define SML_IMPL` and then include anywhere.

```c
#define SML_IMPL
#include "sml.h"
```

## usage

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
