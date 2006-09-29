#include "session.h"

void Session_Free(Session* self)
{

}

int Session_Create(Session* self, PyObject* args)
{

}

PyObject* Session_Add_Track(Session* self, PyObject* args)
{

}

PyObject* Session_Remove_Track(Session* self, PyObject* args)
{

}

PyObject* Session_Set_Hidefirst(Session* self, PyObject* args)
{

}

PyObject* Session_Get_Hidefirst(Session* self, PyObject* args)
{

}

PyObject* Session_Get_Leadout_Entry(Session* self, PyObject* args)
{

}

PyObject* Session_Get_Tracks(Session* self, PyObject* args)
{

}

PyObject* Session_Get_Sectors(Session* self, PyObject* args)
{

}

static char Session_Doc[] = 
PyDoc_STR("libBurn session object.");

static PyMethodDef Session_Methods[] = {
    {"add_track", (PyCFunction)Session_Add_Track, METH_VARARGS,
        PyDoc_STR("Adds a track at the specified position in the session.")},
    {"remove_track", (PyCFunction)Session_Remove_Track, METH_VARARGS,
        PyDoc_STR("Removes the track from the session.")},
    {"set_hidefirst", (PyCFunction)Session_Set_Hidefirst, METH_VARARGS,
        PyDoc_STR("Sets whether the first track is to be hidden in pregrap.")},
    {"get_hidefirst", (PyCFunction)Session_Get_Hidefirst, METH_VARARGS,
        PyDoc_STR("Returns whether the first track is hidden in pregap.")},
    {"get_leadout_entry", (PyCFunction)Session_Get_Leadout_Entry, METH_VARARGS,
        PyDoc_STR("Returns the TOC entry object of the session's leadout")},
    {"get_tracks", (PyCFunction)Session_Get_Tracks, METH_VARARGS,
        PyDoc_STR("Returns the set of tracks associated with the session.")},
    {"get_sectors", (PyCFunction)Session_Get_Sectors, METH_VARARGS,
        PyDoc_STR("Returns the number of sectors of the session.")},
    {NULL, NULL}
};

PyTypeObject SessionType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.session",
    .tp_basicsize = sizeof(Session),
    .tp_dealloc = (destructor)Session_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Session_Doc,
    .tp_methods = Session_Methods,
    .tp_init = (initproc)Session_Create,
};

extern int Session_Setup_Types(void)
{
    SessionType.tp_new = PyType_GenericNew;
    return PyType_Ready(&SessionType);
}
