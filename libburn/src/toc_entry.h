#ifndef PYBURN_TOC_ENTRY_H
#define PYBURN_TOC_ENTRY_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_toc_entry *entry;
} TOCEntry;

extern PyTypeObject TOCEntryType;

void TOCEntry_Free(TOCEntry* self);
int TOCEntry_Create(TOCEntry* self, PyObject* args);

int TOCEntry_Setup_Types(void);

#endif
