#ifndef PYBURN_SOURCE_H
#define PYBURN_SOURCE_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_source *source; 
} Source;

extern PyTypeObject SourceType;

void Source_Free(Source* self);
int Source_Create(Source* self, PyObject* args, PyObject* kwargs);

int Source_Setup_Types(void);

#endif
