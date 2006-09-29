#include "disc.h"

void Disc_Free(Disc* self)
{

}

int Disc_Create(Disc* self, PyObject* args)
{

}

PyObject* Disc_Write(Disc* self, PyObject* args)
{

}

PyObject* Disc_Add_Session(Disc* self, PyObject* args)
{

}

PyObject* Disc_Remove_Session(Disc* self, PyObject* args)
{

}

PyObject* Disc_Get_Sessions(Disc* self, PyObject* args)
{

}

PyObject* Disc_Get_Sectors(Disc* self, PyObject* args)
{

}

static char Disc_Doc[] =
PyDoc_STR("libBurn disc object.");

static PyMethodDef Disc_Methods[] = {
    {"write", (PyCFunction)Disc_Write, METH_VARARGS,
        PyDoc_STR("Writes the disc with the given write options.")},
    {"add_session", (PyCFunction)Disc_Add_Session, METH_VARARGS,
        PyDoc_STR("Adds a session to the disc.")},
    {"remove_session", (PyCFunction)Disc_Remove_Session, METH_VARARGS,
        PyDoc_STR("Removes a session from the disc.")},
    {"get_sessions", (PyCFunction)Disc_Get_Sessions, METH_VARARGS,
        PyDoc_STR("Returns the set of sessions associated with the disc.")},
    {"get_sectors", (PyCFunction)Disc_Get_Sectors, METH_VARARGS,
        PyDoc_STR("Returns the number of sectors currently on the disc.")},
    {NULL, NULL}
};

PyTypeObject DiscType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.disc",
    .tp_basicsize = sizeof(Disc),
    .tp_dealloc = (destructor)Disc_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Disc_Doc,
    .tp_methods = Disc_Methods,
    .tp_init = (initproc)Disc_Create,
};

extern int Disc_Setup_Types(void)
{
    DiscType.tp_new = PyType_GenericNew;
    return PyType_Ready(&DiscType);
}
