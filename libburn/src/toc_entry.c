#include "toc_entry.h"

void TOCEntry_Free(TOCEntry* self)
{

}

int TOCEntry_Create(TOCEntry* self, PyObject* args)
{

}

static char TOCEntry_Doc[] =
PyDoc_STR("libBurn toc_entry object.");

PyTypeObject TOCEntryType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.toc_entry",
    .tp_basicsize = sizeof(TOCEntry),
    .tp_dealloc = (destructor)TOCEntry_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = TOCEntry_Doc,
    .tp_init = (initproc)TOCEntry_Create,
};


extern int TOCEntry_Setup_Types(void)
{
    TOCEntryType.tp_new = PyType_GenericNew;
    return PyType_Ready(&TOCEntryType);
}
