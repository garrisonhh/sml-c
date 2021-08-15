#define SML_IMPL
#include "sml.h"

int main() {
    sml_document_t doc;

    sml_load(&doc, "example.sml");

    sml_print(&doc);

    sml_unload(&doc);

    return 0;
}
