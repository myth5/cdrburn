#ifndef PYBURN_TRACK_H
#define PYBURN_TRACK_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_track *track;
} Track;

extern PyTypeObject TrackType;

void Track_Free(Track* self);
int Track_Create(Track* self, PyObject* args);

PyObject* Track_Define_Data(Track* self, PyObject* args);
PyObject* Track_Set_Isrc(Track* self, PyObject* args);
PyObject* Track_Clear_Isrc(Track* self, PyObject* args);
PyObject* Track_Set_Source(Track* self, PyObject* args);
PyObject* Track_Get_Sectors(Track* self, PyObject* args);
PyObject* Track_Get_Entry(Track* self, PyObject* args);
PyObject* Track_Get_Mode(Track* self, PyObject* args);

int Track_Setup_Types(void);

#endif
