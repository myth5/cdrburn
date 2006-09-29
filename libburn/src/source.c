#include "source.h"

void Source_Free(Source* self)
{

}

int Source_Create(Source* self, PyObject* args, PyObject* kwargs)
{

}

static char Source_Doc[] =
PyDoc_STR("libBurn source object.");

PyTypeObject SourceType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.source",
    .tp_basicsize = sizeof(Source),
    .tp_dealloc = (destructor)Source_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Source_Doc,
    .tp_init = (initproc)Source_Create,
};


extern int Source_Setup_Types(void)
{
    SourceType.tp_new = PyType_GenericNew;
    return PyType_Ready(&SourceType);
}
