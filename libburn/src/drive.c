#include "drive.h"

void Drive_Free(Drive* self)
{

}

int Drive_Create(Drive* self, PyObject* args)
{

}

PyObject* Drive_Grab(Drive* self, PyObject* args)
{

}

PyObject* Drive_Release(Drive* self, PyObject* args)
{

}

PyObject* Drive_Get_Write_Speed(Drive* self, PyObject* args)
{

}

PyObject* Drive_Get_Read_Speed(Drive* self, PyObject* args)
{

}

PyObject* Drive_Set_Speed(Drive* self, PyObject* args)
{

}

PyObject* Drive_Cancel(Drive* self, PyObject* args)
{

}

PyObject* Drive_Get_Disc(Drive* self, PyObject* args)
{

}

PyObject* Drive_Disc_Get_Status(Drive* self, PyObject* args)
{

}

PyObject* Drive_Disc_Erasable(Drive* self, PyObject* args)
{

}

PyObject* Drive_Disc_Erase(Drive* self, PyObject* args)
{

}

PyObject* Drive_Disc_Read(Drive* self, PyObject* args)
{

}

PyObject* Drive_Get_Status(Drive* self, PyObject* args)
{

}

PyObject* Drive_New_ReadOpts(Drive* self, PyObject* args)
{

}

static char Drive_Doc[] = 
PyDoc_STR("libBurn drive object.");

static PyMethodDef Drive_Methods[] = {
    {"grab", (PyCFunction)Drive_Grab, METH_VARARGS,
        PyDoc_STR("Grabs the drive.")},
    {"release", (PyCFunction)Drive_Release, METH_VARARGS,
        PyDoc_STR("Releases the drive.")},
    {"get_status", (PyCFunction)Drive_Get_Status, METH_VARARGS,
        PyDoc_STR("Returns the status of the drive.")},
    {"get_write_speed", (PyCFunction)Drive_Get_Write_Speed, METH_VARARGS,
        PyDoc_STR("Returns the write speed of the drive.")},
    {"get_read_speed", (PyCFunction)Drive_Get_Read_Speed, METH_VARARGS,
        PyDoc_STR("Returns the read speed of the drive.")},
    {"set_speed", (PyCFunction)Drive_Set_Speed, METH_VARARGS,
        PyDoc_STR("Sets the speed of the drive.")},
    {"cancel", (PyCFunction)Drive_Cancel, METH_VARARGS,
        PyDoc_STR("Cancels the current operation on the drive.")},
    {"get_disc", (PyCFunction)Drive_Disc_Erase, METH_VARARGS,
        PyDoc_STR("Returns the disc object associated with the drive.")},
    {"disc_get_status", (PyCFunction)Drive_Disc_Get_Status, METH_VARARGS,
        PyDoc_STR("Returns the status of the disc in the drive.")},
    {"disc_erasable", (PyCFunction)Drive_Disc_Erasable, METH_VARARGS,
        PyDoc_STR("Checks if the disc in the drive is erasable.")},
    {"disc_erase", (PyCFunction)Drive_Disc_Erase, METH_VARARGS,
        PyDoc_STR("Erases the disc in the drive.")},
    {"disc_read", (PyCFunction)Drive_Disc_Read, METH_VARARGS,
        PyDoc_STR("Reads the disc in the drive with the given options.")},
    {"new_readopts", (PyCFunction)Drive_New_ReadOpts, METH_VARARGS,
        PyDoc_STR("Returns a new ReadOpts object associated with the drive.")},
    {NULL, NULL}
};

PyTypeObject DriveType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.drive",
    .tp_basicsize = sizeof(Drive),
    .tp_dealloc = (destructor)Drive_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = Drive_Doc,
    .tp_methods = Drive_Methods,
    .tp_init = (initproc)Drive_Create,
    // ^^?^^
};

extern int Drive_Setup_Types(void)
{
    DriveType.tp_new = PyType_GenericNew;
    return PyType_Ready(&DriveType);
}
