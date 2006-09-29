#ifndef PYBURN_SESSION_H
#define PYBURN_SESSION_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_session *session; 
} Session;

extern PyTypeObject SessionType;

void Session_Free(Session* self);
int Session_Create(Session* self, PyObject* args);

PyObject* Session_Add_Track(Session* self, PyObject* args);
PyObject* Session_Remove_Track(Session* self, PyObject* args);
PyObject* Session_Set_Hidefirst(Session* self, PyObject* args);
PyObject* Session_Get_Hidefirst(Session* self, PyObject* args);
PyObject* Session_Get_Leadout_Entry(Session* self, PyObject* args);
PyObject* Session_Get_Tracks(Session* self, PyObject* args);
PyObject* Session_Get_Sectors(Session* self, PyObject* args);

int Session_Setup_Types(void);

#endif
