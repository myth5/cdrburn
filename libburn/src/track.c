#include "track.h"

void Track_Free(Track* self)
{

}

int Track_Create(Track* self, PyObject* args)
{

}

PyObject* Track_Define_Data(Track* self, PyObject* args)
{

}

PyObject* Track_Set_Isrc(Track* self, PyObject* args)
{

}

PyObject* Track_Clear_Isrc(Track* self, PyObject* args)
{

}

PyObject* Track_Set_Source(Track* self, PyObject* args)
{

}

PyObject* Track_Get_Sectors(Track* self, PyObject* args)
{

}

PyObject* Track_Get_Entry(Track* self, PyObject* args)
{

}

PyObject* Track_Get_Mode(Track* self, PyObject* args)
{

}

static char Track_Doc[] =
PyDoc_STR("libBurn track object.");

static PyMethodDef Track_Methods[] = {
    {"define_data", (PyCFunction)Track_Define_Data, METH_VARARGS,
        PyDoc_STR("Defines the data in the track.")},
    {"set_isrc", (PyCFunction)Track_Set_Isrc, METH_VARARGS,
        PyDoc_STR("Sets the ISRC details for the track.")},
    {"clear_isrc", (PyCFunction)Track_Clear_Isrc, METH_VARARGS,
        PyDoc_STR("Clears all ISRC details for the track.")},
    {"set_source", (PyCFunction)Track_Set_Source, METH_VARARGS,
        PyDoc_STR("Sets the track's data source.")},
    {"get_sectors", (PyCFunction)Track_Get_Sectors, METH_VARARGS,
        PyDoc_STR("Returns the number of sectors currently on the track.")},
    {"get_entry", (PyCFunction)Track_Get_Entry, METH_VARARGS,
        PyDoc_STR("Returns the TOC entry associated with the track.")},
    {"get_mode", (PyCFunction)Track_Get_Mode, METH_VARARGS,
        PyDoc_STR("Returns the current mode of the track.")},
    {NULL, NULL}
};

PyTypeObject TrackType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.track",
    .tp_basicsize = sizeof(Track),
    .tp_dealloc = (destructor)Track_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Track_Doc,
    .tp_methods = Track_Methods,
    .tp_init = (initproc)Track_Create,
};

extern int Track_Setup_Types(void)
{
    TrackType.tp_new = PyType_GenericNew;
    return PyType_Ready(&TrackType);
}
