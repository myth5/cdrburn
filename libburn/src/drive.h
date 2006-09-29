#ifndef PYBURN_DRIVE_H
#define PYBURN_DRIVE_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_drive *drive; 
} Drive;

extern PyTypeObject DriveType;

void Drive_Free(Drive* self);
/* For Private Use Only?! */
int Drive_Create(Drive* self, PyObject* args);

PyObject* Drive_Grab(Drive* self, PyObject* args);
PyObject* Drive_Release(Drive* self, PyObject* args);
PyObject* Drive_Get_Status(Drive* self, PyObject* args);
PyObject* Drive_Get_Write_Speed(Drive* self, PyObject* args);
PyObject* Drive_Get_Read_Speed(Drive* self, PyObject* args);
PyObject* Drive_Set_Speed(Drive* self, PyObject* args);
PyObject* Drive_Cancel(Drive* self, PyObject* args);

PyObject* Drive_Get_Disc(Drive* self, PyObject* args);
PyObject* Drive_Disc_Get_Status(Drive* self, PyObject* args);
PyObject* Drive_Disc_Erasable(Drive* self, PyObject* args);
PyObject* Drive_Disc_Erase(Drive* self, PyObject* args);
PyObject* Drive_Disc_Read(Drive* self, PyObject* args);

PyObject* Drive_New_ReadOpts(Drive* self, PyObject* args);

int Drive_Setup_Types(void);

#endif
