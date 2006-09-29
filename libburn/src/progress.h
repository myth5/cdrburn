#ifndef PYBURN_PROGRESS_H
#define PYBURN_PROGRESS_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_progress *progress;
} Progress;

extern PyTypeObject ProgressType;

void Progress_Free(Progress* self);
int Progress_Create(Progress* self, PyObject* args);

int Progress_Setup_Types(void);

#endif
