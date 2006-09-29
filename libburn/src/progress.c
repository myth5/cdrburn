#include "progress.h"

void Progress_Free(Progress* self)
{

}

int Progress_Create(Progress* self, PyObject* args)
{

}

static char Progress_Doc[] =
PyDoc_STR("libBurn progress object.");

PyTypeObject ProgressType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.progress",
    .tp_basicsize = sizeof(Progress),
    .tp_dealloc = (destructor)Progress_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Progress_Doc,
    .tp_init = (initproc)Progress_Create,
};


extern int Progress_Setup_Types(void)
{
    ProgressType.tp_new = PyType_GenericNew;
    return PyType_Ready(&ProgressType);
}
