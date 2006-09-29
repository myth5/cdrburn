#ifndef PYBURN_READ_OPTS_H
#define PYBURN_READ_OPTS_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_read_opts *opts;
} ReadOpts;

extern PyTypeObject ReadOptsType;

void ReadOpts_Free(ReadOpts* self);
int ReadOpts_Create(ReadOpts* self, PyObject* args);

PyObject* ReadOpts_Set_Raw(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Set_C2Errors(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Read_Subcodes_Audio(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Read_Subcodes_Data(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Set_Hardware_Recovery(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Report_Recovered_Errors(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Transfer_Damaged_Blocks(ReadOpts* self, PyObject* args);
PyObject* ReadOpts_Set_Hardware_Retries(ReadOpts* self, PyObject* args);

int ReadOpts_Setup_Types(void);

#endif
