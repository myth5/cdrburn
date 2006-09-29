#include "message.h"

void Message_Free(Message* self)
{

}

int Message_Create(Message* self, PyObject* args)
{

}

static char Message_Doc[] =
PyDoc_STR("libBurn message object.");

PyTypeObject MessageType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.message",
    .tp_basicsize = sizeof(Message),
    .tp_dealloc = (destructor)Message_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Message_Doc,
    .tp_init = (initproc)Message_Create,
};


extern int Message_Setup_Types(void)
{
    MessageType.tp_new = PyType_GenericNew;
    return PyType_Ready(&MessageType);
}
