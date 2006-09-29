#ifndef PYBURN_WRITE_OPTS_H
#define PYBURN_WRITE_OPTS_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_read_opts *opts;
} WriteOpts;

extern PyTypeObject WriteOptsType;

void WriteOpts_Free(WriteOpts* self);
int WriteOpts_Create(WriteOpts* self, PyObject* args);

PyObject* WriteOpts_Set_Write_Type(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_TOC_Entries(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Format(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Simulate(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Underrun_Proof(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Perform_OPC(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Has_Mediacatalog(WriteOpts* self, PyObject* args);
PyObject* WriteOpts_Set_Mediacatalog(WriteOpts* self, PyObject* args);

int WriteOpts_Setup_Types(void);

#endif
