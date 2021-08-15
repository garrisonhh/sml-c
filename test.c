#define SML_IMPL
#include "sml.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    sml_document_t doc;

    sml_load(&doc, "example.sml");

    sml_print(&doc);

    sml_unload(&doc);

    return 0;
}
