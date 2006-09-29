#include "drive_info.h"

void DriveInfo_Free(DriveInfo* self)
{

}

int DriveInfo_Create(DriveInfo* self, PyObject* args)
{

}

PyObject* DriveInfo_Get_Address(DriveInfo* self, PyObject* args)
{

}

static char DriveInfo_Doc[] =
PyDoc_STR("libBurn drive info object.");

static PyMethodDef DriveInfo_Methods[] = {
    {"get_address", (PyCFunction)DriveInfo_Get_Address, METH_VARARGS,
        PyDoc_STR("Returns the peristent address of the drive associated.")},
    {NULL, NULL}
};

PyTypeObject DriveInfoType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.drive_info",
    .tp_basicsize = sizeof(DriveInfo),
    .tp_dealloc = (destructor)DriveInfo_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = DriveInfo_Doc,
    .tp_methods = DriveInfo_Methods,
    .tp_init = (initproc)DriveInfo_Create,
};


extern int DriveInfo_Setup_Types(void)
{
    DriveInfoType.tp_new = PyType_GenericNew;
    return PyType_Ready(&DriveInfoType);
}
