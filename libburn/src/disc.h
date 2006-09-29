#ifndef PYBURN_DISC_H
#define PYBURN_DISC_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_disc *disc;
} Disc;

extern PyTypeObject DiscType;

void Disc_Free(Disc* self);
int Disc_Create(Disc* self, PyObject* args);

PyObject* Disc_Write(Disc* self, PyObject* args);
PyObject* Disc_Add_Session(Disc* self, PyObject* args);
PyObject* Disc_Remove_Session(Disc* self, PyObject* args);
PyObject* Disc_Get_Sessions(Disc* self, PyObject* args);
PyObject* Disc_Get_Sectors(Disc* self, PyObject* args);

int Disc_Setup_Types(void);

#endif
