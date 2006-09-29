#ifndef PYBURN_MESSAGE_H
#define PYBURN_MESSAGE_H
#include "Python.h"
#include "libburn/libburn.h"

typedef struct {
    PyObject_HEAD
    struct burn_message *message;
} Message;

extern PyTypeObject MessageType;

void Message_Free(Message* self);
int Message_Create(Message* self, PyObject* args);

int Message_Setup_Types(void);

#endif
