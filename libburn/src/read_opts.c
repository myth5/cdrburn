#include "read_opts.h"

void ReadOpts_Free(ReadOpts* self)
{

}

int ReadOpts_Create(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Set_Raw(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Set_C2Errors(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Read_Subcodes_Audio(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Read_Subcodes_Data(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Set_Hardware_Error_Recovery(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Report_Recovered_Errors(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Transfer_Damaged_Blocks(ReadOpts* self, PyObject* args)
{

}

PyObject* ReadOpts_Set_Hardware_Error_Retries(ReadOpts* self, PyObject* args)
{

}

static char ReadOpts_Doc[] =
PyDoc_STR("libBurn read_opts object.");

static PyMethodDef ReadOpts_Methods[] = {
    {"set_raw", (PyCFunction)ReadOpts_Set_Raw, METH_VARARGS,
        PyDoc_STR("Sets whether reading is in raw mode.")},
    {"set_c2errors", (PyCFunction)ReadOpts_Set_C2Errors, METH_VARARGS,
        PyDoc_STR("Sets whether c2 errors are reported.")},
    {"read_subcodes_audio", (PyCFunction)ReadOpts_Read_Subcodes_Audio,
        METH_VARARGS,
        PyDoc_STR("Sets whether subcodes from audio tracks are read.")},
    {"read_subcodes_data", (PyCFunction)ReadOpts_Read_Subcodes_Data,
        METH_VARARGS,
        PyDoc_STR("Sets whether subcodes from data tracks are read.")},
    {"set_hardware_error_recovery",
        (PyCFunction)ReadOpts_Set_Hardware_Error_Recovery, METH_VARARGS,
        PyDoc_STR("Sets whether error recovery should be performed.")},
    {"report_recovered_errors", (PyCFunction)ReadOpts_Report_Recovered_Errors,
        METH_VARARGS,
        PyDoc_STR("Sets whether recovered errors are reported.")},
    {"transfer_damaged_blocks", (PyCFunction)ReadOpts_Transfer_Damaged_Blocks,
        METH_VARARGS,
        PyDoc_STR("Sets whether blocks with unrecoverable errors are read.")},
    {"set_hardware_error_retries",
        (PyCFunction)ReadOpts_Set_Hardware_Error_Retries, METH_VARARGS,
        PyDoc_STR("Sets the number of attempts when correcting an error.")},
    {NULL, NULL}
};

PyTypeObject ReadOptsType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.read_opts",
    .tp_basicsize = sizeof(ReadOpts),
    .tp_dealloc = (destructor)ReadOpts_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = ReadOpts_Doc,
    .tp_methods = ReadOpts_Methods,
    .tp_init = (initproc)ReadOpts_Create,
};

extern int ReadOpts_Setup_Types(void)
{
    ReadOptsType.tp_new = PyType_GenericNew;
    return PyType_Ready(&ReadOptsType);
}
