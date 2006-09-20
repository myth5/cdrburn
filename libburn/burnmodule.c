#include "Python.h"
#include "libburn/libburn.h"

static PyObject *ErrorObject;


/* Declarations for objects of type toc_entry */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_toc_entryobject;

static PyTypeObject B_toc_entrytype;


/* Declarations for objects of type source */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_sourceobject;

static PyTypeObject B_sourcetype;


/* Declarations for objects of type drive_info */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_drive_infoobject;

static PyTypeObject B_drive_infotype;


/* Declarations for objects of type drive */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_driveobject;

static PyTypeObject B_drivetype;


/* Declarations for objects of type message */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_messageobject;

static PyTypeObject B_messagetype;


/* Declarations for objects of type progress */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_progressobject;

static PyTypeObject B_progresstype;


/* Declarations for objects of type write_options */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_write_optsobject;

static PyTypeObject B_write_optstype;


/* Declarations for objects of type read_options */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_read_optsobject;

static PyTypeObject B_read_optstype;


/* Declarations for objects of type disc */

typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_discobject;

static PyTypeObject B_disctype;


/* Declarations for objects of type session */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_sessionobject;

static PyTypeObject B_sessiontype;


/* Declarations for objects of type track */
typedef struct {
    PyObject_HEAD
    /* XXXX Add your own stuff here */
} b_trackobject;

static PyTypeObject B_tracktype;


/* BEGIN burn_toc_entry */
static struct PyMethodDef b_toc_entry_methods[] = {
    {NULL,      NULL}       /* sentinel */
};

static b_toc_entryobject *
newb_toc_entryobject()
{
    b_toc_entryobject *self;
    
    self = PyObject_NEW(b_toc_entryobject, &B_toc_entrytype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_toc_entry_dealloc(b_toc_entryobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_toc_entrytype__doc__[] = 
""
;
static PyTypeObject B_toc_entrytype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.toc_entry",                /*tp_name*/
    sizeof(b_toc_entryobject),  /*tp_basicsize*/
    0,                          /*tp_itemsize*/

    /* methods */
    (destructor)b_toc_entry_dealloc,    /*tp_dealloc*/
    (printfunc)0,                       /*tp_print*/
    (getattrfunc)0,                     /*tp_getattr*/
    (setattrfunc)0,                     /*tp_setattr*/
    (cmpfunc)0,                         /*tp_compare*/
    (reprfunc)0,                        /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    (hashfunc)0,                        /*tp_hash*/
    (ternaryfunc)0,                     /*tp_call*/
    (reprfunc)0,                        /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_toc_entrytype__doc__              /* Documentation string */
};
/* END burn_toc_entry */


/* BEGIN burn_source */
static char b_source_free__doc__[] = 
""
;
static PyObject *
b_source_free(b_sourceobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_source_file_new__doc__[] = 
""
;
static PyObject *
b_source_file_new(b_sourceobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_source_fd_new__doc__[] = 
""
;
static PyObject *
b_source_fd_new(b_sourceobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_source_methods[] = {
 {"free", (PyCFunction)b_source_free, METH_VARARGS, b_source_free__doc__},
 {"file_new", (PyCFunction)b_source_file_new, METH_VARARGS, b_source_file_new__doc__},
 {"fd_new", (PyCFunction)b_source_fd_new, METH_VARARGS, b_source_fd_new__doc__},
 {NULL,     NULL}
};

static PyObject *
b_source_getattr(b_sourceobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_source_methods, (PyObject *)self, name);
}

static b_sourceobject *
newb_sourceobject()
{
    b_sourceobject *self;
    
    self = PyObject_NEW(b_sourceobject, &B_sourcetype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_source_dealloc(b_sourceobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_sourcetype__doc__[] = 
""
;
static PyTypeObject B_sourcetype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                      /*ob_size*/
    "burn.source",          /*tp_name*/
    sizeof(b_sourceobject), /*tp_basicsize*/
    0,                      /*tp_itemsize*/

    /* methods */
    (destructor)b_source_dealloc,   /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_source_getattr,  /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_sourcetype__doc__             /* Documentation string */
};
/* END burn_source */


/* BEGIN burn_drive_info */
static char b_drive_info_forget__doc__[] = 
""
;
static PyObject *
b_drive_info_forget(b_drive_infoobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_info_free__doc__[] = 
""
;
static PyObject *
b_drive_info_free(b_drive_infoobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_drive_info_methods[] = {
    {"forget", (PyCFunction)b_drive_info_forget, METH_VARARGS, b_drive_info_forget__doc__},
    {"free", (PyCFunction)b_drive_info_free, METH_VARARGS, b_drive_info_free__doc__},
    {NULL,      NULL}
};

static PyObject *
b_drive_info_getattr(b_drive_infoobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_drive_info_methods, (PyObject *)self, name);
}

static b_drive_infoobject *
newb_drive_infoobject()
{
    b_drive_infoobject *self;
    
    self = PyObject_NEW(b_drive_infoobject, &B_drive_infotype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_drive_info_dealloc(b_drive_infoobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_drive_infotype__doc__[] = 
""
;
static PyTypeObject B_drive_infotype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.drive_info",          /*tp_name*/
    sizeof(b_drive_infoobject), /*tp_basicsize*/
    0,                          /*tp_itemsize*/

    /* methods */
    (destructor)b_drive_info_dealloc,   /*tp_dealloc*/
    (printfunc)0,                       /*tp_print*/
    (getattrfunc)b_drive_info_getattr,  /*tp_getattr*/
    (setattrfunc)0,                     /*tp_setattr*/
    (cmpfunc)0,                         /*tp_compare*/
    (reprfunc)0,                        /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    (hashfunc)0,                        /*tp_hash*/
    (ternaryfunc)0,                     /*tp_call*/
    (reprfunc)0,                        /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_drive_infotype__doc__             /* Documentation string */
};
/* END burn_drive_info */


/* BEGIN burn_drive */
static char b_drive_grab__doc__[] = 
""
;
static PyObject *
b_drive_grab(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_release__doc__[] = 
""
;
static PyObject *
b_drive_release(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_disc_get_status__doc__[] = 
""
;
static PyObject *
b_drive_get_status(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_cancel__doc__[] = 
""
;
static PyObject *
b_drive_cancel(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_get_disc__doc__[] = 
""
;
static PyObject *
b_drive_get_disc(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_set_speed__doc__[] = 
""
;
static PyObject *
b_drive_set_speed(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_get_write_speed__doc__[] = 
""
;
static PyObject *
b_drive_get_write_speed(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_drive_get_read_speed__doc__[] = 
""
;
static PyObject *
b_drive_get_read_speed(b_driveobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_drive_methods[] = {
 {"grab", (PyCFunction)b_drive_grab, METH_VARARGS, b_drive_grab__doc__},
 {"release", (PyCFunction)b_drive_release, METH_VARARGS, b_drive_release__doc__},
 {"get_status", (PyCFunction)b_drive_get_status, METH_VARARGS, b_drive_disc_get_status__doc__},
 {"cancel", (PyCFunction)b_drive_cancel, METH_VARARGS, b_drive_cancel__doc__},
 {"get_disc", (PyCFunction)b_drive_get_disc, METH_VARARGS, b_drive_get_disc__doc__},
 {"set_speed", (PyCFunction)b_drive_set_speed, METH_VARARGS, b_drive_set_speed__doc__},
 {"get_write_speed", (PyCFunction)b_drive_get_write_speed, METH_VARARGS, b_drive_get_write_speed__doc__},
 {"get_read_speed", (PyCFunction)b_drive_get_read_speed, METH_VARARGS, b_drive_get_read_speed__doc__},
 {NULL,      NULL} 
};

static PyObject *
b_drive_getattr(b_driveobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_drive_methods, (PyObject *)self, name);
}

static b_driveobject *
newb_driveobject()
{
    b_driveobject *self;
    
    self = PyObject_NEW(b_driveobject, &B_drivetype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_drive_dealloc(b_driveobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_drivetype__doc__[] = 
""
;
static PyTypeObject B_drivetype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                      /*ob_size*/
    "burn.drive",           /*tp_name*/
    sizeof(b_driveobject),  /*tp_basicsize*/
    0,                      /*tp_itemsize*/

    /* methods */
    (destructor)b_drive_dealloc,    /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_drive_getattr,   /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_drivetype__doc__              /* Documentation string */
};
/* END burn_drive */


/* BEGIN burn_message */
static char b_message_free__doc__[] = 
""
;
static PyObject *
b_message_free(b_messageobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_message_methods[] = {
    {"free", (PyCFunction)b_message_free, METH_VARARGS, b_message_free__doc__},
    {NULL,      NULL} 
};

static PyObject *
b_message_getattr(b_messageobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_message_methods, (PyObject *)self, name);
}

static b_messageobject *
newb_messageobject()
{
    b_messageobject *self;
    
    self = PyObject_NEW(b_messageobject, &B_messagetype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_message_dealloc(b_messageobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_messagetype__doc__[] = 
""
;
static PyTypeObject B_messagetype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.message",             /*tp_name*/
    sizeof(b_messageobject),    /*tp_basicsize*/
    0,                          /*tp_itemsize*/

    /* methods */
    (destructor)b_message_dealloc,  /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_message_getattr, /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_messagetype__doc__            /* Documentation string */
};
/* END burn_message */


/* BEGIN burn_progress */
static struct PyMethodDef b_progress_methods[] = {
    {NULL,      NULL} 
};

static b_progressobject *
newb_progressobject()
{
    b_progressobject *self;
    
    self = PyObject_NEW(b_progressobject, &B_progresstype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_progress_dealloc(b_progressobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_progresstype__doc__[] = 
""
;
static PyTypeObject B_progresstype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.progress",            /*tp_name*/
    sizeof(b_progressobject),   /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    
    /* methods */
    (destructor)b_progress_dealloc, /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)0,                 /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_progresstype__doc__           /* Documentation string */
};
/* END burn_progress */


/* BEGIN burn_write_opts */
static char b_write_opts_free__doc__[] = 
""
;
static PyObject *
b_write_opts_free(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static char b_write_opts_set_write_type__doc__[] = 
""
;
static PyObject *
b_write_opts_set_write_type(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_toc_entries__doc__[] = 
""
;
static PyObject *
b_write_opts_set_toc_entries(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_format__doc__[] = 
""
;
static PyObject *
b_write_opts_set_format(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_simulate__doc__[] = 
""
;
static PyObject *
b_write_opts_set_simulate(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static char b_write_opts_set_underrun_proof__doc__[] = 
""
;
static PyObject *
b_write_opts_set_underrun_proof(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_perform_opc__doc__[] = 
""
;
static PyObject *
b_write_opts_set_perform_opc(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_has_mediacatalog__doc__[] = 
""
;
static PyObject *
b_write_opts_set_has_mediacatalog(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_write_opts_set_mediacatalog__doc__[] = 
""
;
static PyObject *
b_write_opts_set_mediacatalog(b_write_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_write_opts_methods[] = {
 {"free", (PyCFunction)b_write_opts_free, METH_VARARGS, b_write_opts_free__doc__},
 {"set_write_type", (PyCFunction)b_write_opts_set_write_type, METH_VARARGS, b_write_opts_set_write_type__doc__},
 {"set_toc_entries", (PyCFunction)b_write_opts_set_toc_entries, METH_VARARGS, b_write_opts_set_toc_entries__doc__},
 {"set_format", (PyCFunction)b_write_opts_set_format, METH_VARARGS, b_write_opts_set_format__doc__},
 {"set_simulate", (PyCFunction)b_write_opts_set_simulate, METH_VARARGS, b_write_opts_set_simulate__doc__},
 {"set_underrun_proof", (PyCFunction)b_write_opts_set_underrun_proof, METH_VARARGS, b_write_opts_set_underrun_proof__doc__},
 {"set_perform_opc", (PyCFunction)b_write_opts_set_perform_opc, METH_VARARGS, b_write_opts_set_perform_opc__doc__},
 {"set_has_mediacatalog", (PyCFunction)b_write_opts_set_has_mediacatalog, METH_VARARGS, b_write_opts_set_has_mediacatalog__doc__},
 {"set_has_mediacatalog", (PyCFunction)b_write_opts_set_has_mediacatalog, METH_VARARGS, b_write_opts_set_has_mediacatalog__doc__}, 
 {NULL,      NULL}      
};

static PyObject *
b_write_opts_getattr(b_write_optsobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_write_opts_methods, (PyObject *)self, name);
}

static b_write_optsobject *
newb_write_optsobject()
{
    b_write_optsobject *self;
    
    self = PyObject_NEW(b_write_optsobject, &B_write_optstype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_write_opts_dealloc(b_write_optsobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_write_optstype__doc__[] = 
""
;
static PyTypeObject B_write_optstype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.write_opts",       /*tp_name*/
    sizeof(b_write_optsobject), /*tp_basicsize*/
    0,                          /*tp_itemsize*/

    /* methods */
    (destructor)b_write_opts_dealloc,   /*tp_dealloc*/
    (printfunc)0,                       /*tp_print*/
    (getattrfunc)b_write_opts_getattr,  /*tp_getattr*/
    (setattrfunc)0,                     /*tp_setattr*/
    (cmpfunc)0,                         /*tp_compare*/
    (reprfunc)0,                        /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    (hashfunc)0,                        /*tp_hash*/
    (ternaryfunc)0,                     /*tp_call*/
    (reprfunc)0,                        /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_write_optstype__doc__             /* Documentation string */
};
/* END burn_write_opts */


/* BEGIN burn_read_opts */
static char b_read_opts_free__doc__[] = 
""
;
static PyObject *
b_read_opts_free(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_set_raw__doc__[] = 
""
;
static PyObject *
b_read_opts_set_raw(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_set_c2errors__doc__[] = 
""
;
static PyObject *
b_read_opts_set_c2errors(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_read_subcodes_audio__doc__[] = 
""
;
static PyObject *
b_read_opts_read_subcodes_audio(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_read_subcodes_data__doc__[] = 
""
;
static PyObject *
b_read_opts_read_subcodes_data(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_set_hardware_error_recovery__doc__[] = 
""
;
static PyObject *
b_read_opts_set_hardware_error_recovery(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_report_recovered_errors__doc__[] = 
""
;
static PyObject *
b_read_opts_report_recovered_errors(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_transfer_damaged_blocks__doc__[] = 
""
;
static PyObject *
b_read_opts_transfer_damaged_blocks(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_read_opts_set_hardware_error_retries__doc__[] = 
""
;
static PyObject *
b_read_opts_set_hardware_error_retries(b_read_optsobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_read_opts_methods[] = {
 {"free", (PyCFunction)b_read_opts_free, METH_VARARGS, b_read_opts_free__doc__},
 {"set_raw", (PyCFunction)b_read_opts_set_raw, METH_VARARGS, b_read_opts_set_raw__doc__},
 {"set_c2errors", (PyCFunction)b_read_opts_set_c2errors, METH_VARARGS, b_read_opts_set_c2errors__doc__},
 {"read_subcodes_audio", (PyCFunction)b_read_opts_read_subcodes_audio, METH_VARARGS,   b_read_opts_read_subcodes_audio__doc__},
 {"read_subcodes_data", (PyCFunction)b_read_opts_read_subcodes_data, METH_VARARGS, b_read_opts_read_subcodes_data__doc__},
 {"set_hardware_error_recovery", (PyCFunction)b_read_opts_set_hardware_error_recovery, METH_VARARGS,   b_read_opts_set_hardware_error_recovery__doc__},
 {"report_recovered_errors",    (PyCFunction)b_read_opts_report_recovered_errors,   METH_VARARGS,   b_read_opts_report_recovered_errors__doc__},
 {"transfer_damaged_blocks", (PyCFunction)b_read_opts_transfer_damaged_blocks, METH_VARARGS,   b_read_opts_transfer_damaged_blocks__doc__},
 {"set_hardware_error_retries", (PyCFunction)b_read_opts_set_hardware_error_retries, METH_VARARGS, b_read_opts_set_hardware_error_retries__doc__},
 {NULL,      NULL}  
};

static PyObject *
b_read_opts_getattr(b_read_optsobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_read_opts_methods, (PyObject *)self, name);
}

static b_read_optsobject *
newb_read_optsobject()
{
    b_read_optsobject *self;
    
    self = PyObject_NEW(b_read_optsobject, &B_read_optstype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_read_opts_dealloc(b_read_optsobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_read_optstype__doc__[] = 
""
;
static PyTypeObject B_read_optstype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.read_opts",        /*tp_name*/
    sizeof(b_read_optsobject),  /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    
    /* methods */
    (destructor)b_read_opts_dealloc,    /*tp_dealloc*/
    (printfunc)0,                       /*tp_print*/
    (getattrfunc)b_read_opts_getattr,   /*tp_getattr*/
    (setattrfunc)0,                     /*tp_setattr*/
    (cmpfunc)0,                         /*tp_compare*/
    (reprfunc)0,                        /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    (hashfunc)0,                        /*tp_hash*/
    (ternaryfunc)0,                     /*tp_call*/
    (reprfunc)0,                        /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_read_optstype__doc__              /* Documentation string */
};
/* END burn_read_opts */


/* BEGIN burn_disc */
static char b_disc_free__doc__[] = 
""
;
static PyObject *
b_disc_free(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_disc_write__doc__[] = 
""
;
static PyObject *
b_disc_write(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_disc_add_session__doc__[] = 
""
;
static PyObject *
b_disc_add_session(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_disc_remove_session__doc__[] = 
""
;
static PyObject *
b_disc_remove_session(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_disc_get_sessions__doc__[] = 
""
;
static PyObject *
b_disc_get_sessions(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_disc_get_sectors__doc__[] = 
""
;
static PyObject *
b_disc_get_sectors(b_discobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_disc_methods[] = {
 {"free", (PyCFunction)b_disc_free, METH_VARARGS, b_disc_free__doc__},
 {"write", (PyCFunction)b_disc_write, METH_VARARGS, b_disc_write__doc__},
 {"add_session", (PyCFunction)b_disc_add_session, METH_VARARGS, b_disc_add_session__doc__},
 {"remove_session", (PyCFunction)b_disc_remove_session, METH_VARARGS, b_disc_remove_session__doc__},
 {"get_sessions", (PyCFunction)b_disc_get_sessions, METH_VARARGS, b_disc_get_sessions__doc__},
 {"get_sectors", (PyCFunction)b_disc_get_sectors, METH_VARARGS, b_disc_get_sectors__doc__},
 {NULL,      NULL}
};

static PyObject *
b_disc_getattr(b_discobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_disc_methods, (PyObject *)self, name);
}

static b_discobject *
newb_discobject()
{
    b_discobject *self;
    
    self = PyObject_NEW(b_discobject, &B_disctype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_disc_dealloc(b_discobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_disctype__doc__[] = 
""
;
static PyTypeObject B_disctype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                      /*ob_size*/
    "burn.disc",            /*tp_name*/
    sizeof(b_discobject),   /*tp_basicsize*/
    0,                      /*tp_itemsize*/

    /* methods */
    (destructor)b_disc_dealloc,     /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_disc_getattr,    /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_disctype__doc__               /* Documentation string */
};
/* END burn_disc */


/* BEGIN burn_session */
static char b_session_free__doc__[] = 
""
;
static PyObject *
b_session_free(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_add_track__doc__[] = 
""
;
static PyObject *
b_session_add_track(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_remove_track__doc__[] = 
""
;
static PyObject *
b_session_remove_track(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_hide_first_track__doc__[] = 
""
;
static PyObject *
b_session_hide_first_track(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_get_leadout_entry__doc__[] = 
""
;
static PyObject *
b_session_get_leadout_entry(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_get_tracks__doc__[] = 
""
;
static PyObject *
b_session_get_tracks(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_get_sectors__doc__[] = 
""
;
static PyObject *
b_session_get_sectors(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_session_get_hidefirst__doc__[] = 
""
;
static PyObject *
b_session_get_hidefirst(b_sessionobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_session_methods[] = {
 {"free", (PyCFunction)b_session_free, METH_VARARGS, b_session_free__doc__},
 {"add_track", (PyCFunction)b_session_add_track, METH_VARARGS, b_session_add_track__doc__},
 {"remove_track", (PyCFunction)b_session_remove_track, METH_VARARGS, b_session_remove_track__doc__},
 {"hide_first_track", (PyCFunction)b_session_hide_first_track, METH_VARARGS, b_session_hide_first_track__doc__},
 {"get_leadout_entry", (PyCFunction)b_session_get_leadout_entry, METH_VARARGS, b_session_get_leadout_entry__doc__},
 {"get_tracks", (PyCFunction)b_session_get_tracks, METH_VARARGS, b_session_get_tracks__doc__},
 {"get_sectors", (PyCFunction)b_session_get_sectors, METH_VARARGS, b_session_get_sectors__doc__},
 {"get_hidefirst", (PyCFunction)b_session_get_hidefirst, METH_VARARGS, b_session_get_hidefirst__doc__},
 {NULL,      NULL} 
};

static PyObject *
b_session_getattr(b_sessionobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_session_methods, (PyObject *)self, name);
}

static b_sessionobject *
newb_sessionobject()
{
    b_sessionobject *self;
    
    self = PyObject_NEW(b_sessionobject, &B_sessiontype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_session_dealloc(b_sessionobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_sessiontype__doc__[] = 
""
;
static PyTypeObject B_sessiontype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /*ob_size*/
    "burn.session",             /*tp_name*/
    sizeof(b_sessionobject),    /*tp_basicsize*/
    0,                          /*tp_itemsize*/

    /* methods */
    (destructor)b_session_dealloc,  /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_session_getattr, /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_sessiontype__doc__            /* Documentation string */
};
/* END burn_session */


/* BEGIN burn_track */
static char b_track_free__doc__[] = 
""
;
static PyObject *
b_track_free(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static char b_track_define_data__doc__[] = 
""
;
static PyObject *
b_track_define_data(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_track_set_isrc__doc__[] = 
""
;
static PyObject *
b_track_set_isrc(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_track_clear_isrc__doc__[] = 
""
;
static PyObject *
b_track_clear_isrc(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static char b_track_set_source__doc__[] = 
""
;
static PyObject *
b_track_set_source(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static char b_track_get_sectors__doc__[] = 
""
;
static PyObject *
b_track_get_sectors(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_track_get_entry__doc__[] = 
""
;
static PyObject *
b_track_get_entry(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_track_get_mode__doc__[] = 
""
;
static PyObject *
b_track_get_mode(b_trackobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef b_track_methods[] = {
 {"free", (PyCFunction)b_track_free, METH_VARARGS, b_track_free__doc__},
 {"define_data", (PyCFunction)b_track_define_data, METH_VARARGS, b_track_define_data__doc__},
 {"set_isrc", (PyCFunction)b_track_set_isrc, METH_VARARGS, b_track_set_isrc__doc__},
 {"clear_isrc", (PyCFunction)b_track_clear_isrc, METH_VARARGS, b_track_clear_isrc__doc__},
 {"set_source", (PyCFunction)b_track_set_source, METH_VARARGS, b_track_set_source__doc__},
 {"get_sectors", (PyCFunction)b_track_get_sectors, METH_VARARGS, b_track_get_sectors__doc__},
 {"get_entry", (PyCFunction)b_track_get_entry, METH_VARARGS, b_track_get_entry__doc__},
 {"get_mode", (PyCFunction)b_track_get_mode, METH_VARARGS, b_track_get_mode__doc__},
 {NULL,      NULL}
};

static PyObject *
b_track_getattr(b_trackobject *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(b_track_methods, (PyObject *)self, name);
}

static b_trackobject *
newb_trackobject()
{
    b_trackobject *self;
    
    self = PyObject_NEW(b_trackobject, &B_tracktype);
    if (self == NULL)
        return NULL;
    /* XXXX Add your own initializers here */
    return self;
}

static void
b_track_dealloc(b_trackobject *self)
{
    /* XXXX Add your own cleanup code here */
    PyMem_DEL(self);
}

static char B_tracktype__doc__[] = 
""
;
static PyTypeObject B_tracktype = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                      /*ob_size*/
    "burn.track",           /*tp_name*/
    sizeof(b_trackobject),  /*tp_basicsize*/
    0,                      /*tp_itemsize*/

    /* methods */
    (destructor)b_track_dealloc,    /*tp_dealloc*/
    (printfunc)0,                   /*tp_print*/
    (getattrfunc)b_track_getattr,   /*tp_getattr*/
    (setattrfunc)0,                 /*tp_setattr*/
    (cmpfunc)0,                     /*tp_compare*/
    (reprfunc)0,                    /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    (hashfunc)0,                    /*tp_hash*/
    (ternaryfunc)0,                 /*tp_call*/
    (reprfunc)0,                    /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    B_tracktype__doc__              /* Documentation string */
};
/* END burn_track */


/* Main burn Module */
static char b_main_init__doc__[] =
"Initializes the burn module. You _MUST_ call this before you start!"
;
static PyObject *
b_main_init(PyObject *self /* Not used */, PyObject *args)
{
    if (burn_initialize()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return NULL;
}

static char b_main_finish__doc__[] =
"Closes the burn module. You _MUST_ call this when you are done!"
;
static PyObject *
b_main_finish(PyObject *self /* Not used */, PyObject *args)
{
    burn_finish();
    
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_set_verbosity__doc__[] =
""
;
static PyObject *
b_main_set_verbosity(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_preset_device_open__doc__[] =
""
;
static PyObject *
b_main_preset_device_open(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_get_message__doc__[] =
""
;
static PyObject *
b_main_get_message(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_drive_add_whitelist__doc__[] =
""
;
static PyObject *
b_main_drive_add_whitelist(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_drive_clear_whitelist__doc__[] =
""
;
static PyObject *
b_main_drive_clear_whitelist(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_drive_scan__doc__[] =
""
;
static PyObject *
b_main_drive_scan(PyObject *self /* Not used */, PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static char b_main_msf_to_sectors__doc__[] =
"Convert a Minute-Second-Frame (MSF) tuple value to its sector count"
;
static PyObject *
b_main_msf_to_sectors(PyObject *self /* Not used */, PyObject *args)
{
    int sectors, min, sec, frame;
    
    if (!PyArg_ParseTuple(args, "(iii)", &min, &sec, &frame))
        return NULL;

    sectors = burn_msf_to_sectors(min, sec, frame);

    return PyInt_FromLong(sectors);
}

static char b_main_sectors_to_msf__doc__[] =
"Convert a sector count to its Minute-Second-Frame (MSF) tuple value"
;
static PyObject *
b_main_sectors_to_msf(PyObject *self /* Not used */, PyObject *args)
{
    int sectors, min, sec, frame;   

    if (!PyArg_ParseTuple(args, "i", &sectors))
        return NULL;

    burn_sectors_to_msf(sectors, &min, &sec, &frame);

    return PyTuple_Pack(3, PyInt_FromLong(min),
                             PyInt_FromLong(sec), 
                               PyInt_FromLong(frame));
}

static char b_main_msf_to_lba__doc__[] =
"Convert a Minute-Second-Frame (MSF) tuple value to its corresponding LBA"
;
static PyObject *
b_main_msf_to_lba(PyObject *self /* Not used */, PyObject *args)
{
    int lba, min, sec, frame;
    if (!PyArg_ParseTuple(args, "(iii)", &min, &sec, &frame))
        return NULL;

    lba = burn_msf_to_lba(min, sec, frame);

    return PyInt_FromLong(lba);
}

static char b_main_version__doc__[] =
"Return a tuple containing the major, minor and micro versions of libburn"
;
static PyObject *
b_main_version(PyObject *self /* Not used */, PyObject *args)
{
    int maj, min, mic;
    burn_version(&maj, &min, &mic);

    return PyTuple_Pack(3, PyInt_FromLong(maj),
                             PyInt_FromLong(min), 
                               PyInt_FromLong(mic));
}

/* List of methods defined in the module */
static struct PyMethodDef b_main_methods[] = {
 {"init", (PyCFunction)b_main_init, METH_VARARGS, b_main_init__doc__},
 {"finish", (PyCFunction)b_main_finish, METH_VARARGS, b_main_finish__doc__},
 {"set_verbosity", (PyCFunction)b_main_set_verbosity, METH_VARARGS, b_main_set_verbosity__doc__},
 {"preset_device_open", (PyCFunction)b_main_preset_device_open, METH_VARARGS, b_main_preset_device_open__doc__},
 {"get_message", (PyCFunction)b_main_get_message, METH_VARARGS, b_main_get_message__doc__},
 {"drive_add_whitelist", (PyCFunction)b_main_drive_add_whitelist, METH_VARARGS, b_main_drive_add_whitelist__doc__},
 {"drive_clear_whitelist", (PyCFunction)b_main_drive_clear_whitelist, METH_VARARGS, b_main_drive_clear_whitelist__doc__},
 {"drive_scan", (PyCFunction)b_main_drive_scan, METH_VARARGS, b_main_drive_scan__doc__},
 {"msf_to_sectors", (PyCFunction)b_main_msf_to_sectors, METH_VARARGS, b_main_msf_to_sectors__doc__},
 {"sectors_to_msf", (PyCFunction)b_main_sectors_to_msf, METH_VARARGS, b_main_sectors_to_msf__doc__},
 {"msf_to_lba", (PyCFunction)b_main_msf_to_lba, METH_VARARGS, b_main_msf_to_lba__doc__},
 {"version", (PyCFunction)b_main_version, METH_VARARGS, b_main_version__doc__},
 {NULL, (PyCFunction)NULL, 0, NULL}      
};


/* Initialization function for the module (*must* be called initburn) */
static char burn_module_documentation[] = 
""
;
void
initburn()
{
    PyObject *m, *d;

    /* Create the module and add the functions */
    m = Py_InitModule4("burn", b_main_methods,
        burn_module_documentation,
        (PyObject*)NULL,PYTHON_API_VERSION);

    /* Add objects */
    PyModule_AddObject(m, "toc_entry", (PyObject *)&B_toc_entrytype);
    PyModule_AddObject(m, "source", (PyObject *)&B_sourcetype);
    PyModule_AddObject(m, "drive_info", (PyObject *)&B_drive_infotype);
    PyModule_AddObject(m, "drive", (PyObject *)&B_drivetype);
    PyModule_AddObject(m, "message", (PyObject *)&B_messagetype);
    PyModule_AddObject(m, "progress", (PyObject *)&B_progresstype);
    PyModule_AddObject(m, "write_opts", (PyObject *)&B_write_optstype);
    PyModule_AddObject(m, "read_opts", (PyObject *)&B_read_optstype);
    PyModule_AddObject(m, "disc", (PyObject *)&B_disctype);
    PyModule_AddObject(m, "session", (PyObject *)&B_sessiontype);
    PyModule_AddObject(m, "track", (PyObject *)&B_tracktype);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    ErrorObject = PyString_FromString("burn.error");
    PyDict_SetItemString(d, "error", ErrorObject);

    /* XXXX Add constants here */
    
    /* Check for errors */
    if (PyErr_Occurred())
        Py_FatalError("can't initialize module burn");
}

