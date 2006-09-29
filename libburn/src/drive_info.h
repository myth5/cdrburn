#ifndef PYBURN_DRIVE_INFO_H
#define PYBURN_DRIVE_INFO_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_drive_info *info;
} DriveInfo;

extern PyTypeObject DriveInfoType;

void DriveInfo_Free(DriveInfo* self);
/* For private use only?! */
int DriveInfo_Create(DriveInfo* self, PyObject* args);

PyObject* DriveInfo_Get_Address(DriveInfo* self, PyObject* args);

int DriveInfo_Setup_Types(void);

#endif
