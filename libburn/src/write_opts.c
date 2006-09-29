#include "write_opts.h"

void WriteOpts_Free(WriteOpts* self)
{

}

int WriteOpts_Create(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Write_Type(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_TOC_Entries(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Format(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Simulate(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Underrun_Proof(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Perform_OPC(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Has_Mediacatalog(WriteOpts* self, PyObject* args)
{

}

PyObject* WriteOpts_Set_Mediacatalog(WriteOpts* self, PyObject* args)
{

}

static char WriteOpts_Doc[] =
PyDoc_STR("libBurn write_opts object.");

static PyMethodDef WriteOpts_Methods[] = {
    {"set_write_type", (PyCFunction)WriteOpts_Set_Write_Type, METH_VARARGS,
        PyDoc_STR("Sets the write type.")},
    {"set_toc_entries", (PyCFunction)WriteOpts_Set_TOC_Entries, METH_VARARGS,
        PyDoc_STR("Supplies TOC entries for writing.")},
    {"set_format", (PyCFunction)WriteOpts_Set_Format, METH_VARARGS,
        PyDoc_STR("Sets the session format for a disc.")},
    {"set_simulate", (PyCFunction)WriteOpts_Set_Simulate, METH_VARARGS,
        PyDoc_STR("Sets whether a simulation is performed.")},
    {"set_underrun_proof", (PyCFunction)WriteOpts_Set_Underrun_Proof,
        METH_VARARGS,
        PyDoc_STR("Contols buffer underrun prevention.")},
    {"set_perform_opc", (PyCFunction)WriteOpts_Set_Perform_OPC, METH_VARARGS,
        PyDoc_STR("Sets whether OPC is used.")},
    {"set_has_mediacatalog", (PyCFunction)WriteOpts_Set_Has_Mediacatalog,
        METH_VARARGS,
        PyDoc_STR("Controls whether the session will have a Mediacatalog.")},
    {"set_mediacatalog",
        (PyCFunction)WriteOpts_Set_Mediacatalog, METH_VARARGS,
        PyDoc_STR("Sets the Mediacatalog for the session.")},
    {NULL, NULL}
};

PyTypeObject WriteOptsType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "burn.read_opts",
    .tp_basicsize = sizeof(WriteOpts),
    .tp_dealloc = (destructor)WriteOpts_Free,
    .tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_doc = WriteOpts_Doc,
    .tp_methods = WriteOpts_Methods,
    .tp_init = (initproc)WriteOpts_Create,
};

extern int WriteOpts_Setup_Types(void)
{
    WriteOptsType.tp_new = PyType_GenericNew;
    return PyType_Ready(&WriteOptsType);
}
