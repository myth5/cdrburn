#include "disc.h"
#include "drive.h"
#include "drive_info.h"
#include "message.h"
#include "progress.h"
#include "read_opts.h"
#include "session.h"
#include "source.h"
#include "toc_entry.h"
#include "track.h"
#include "write_opts.h"

static PyObject* ErrorObject;

static PyObject* Burn_Initialize(PyObject* self, PyObject* args)
{
    if (burn_initialize()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return NULL; 
}

static PyObject* Burn_Finish(PyObject* self, PyObject* args)
{
    burn_finish();
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Burn_Set_Verbosity(PyObject* self, PyObject* args)
{
    int level;
    
    if (!PyArg_ParseTuple(args, "i", &level))
        return NULL;
    burn_set_verbosity(level);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Burn_Preset_Device_Open(PyObject* self, PyObject* args)
{
    int ex, bl, abort;

    if (!PyArg_ParseTuples(args, "iii", &ex, &bl, &abort))
        return NULL;
    burn_preset_device_open(ex, bl, abort);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Burn_Drive_Scan_And_Grab(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Add_Whitelist(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Clear_Whitelist(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Scan(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Info_Forget(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Info_Free(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Is_Enumerable(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Convert_FS_Address(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Convert_SCSI_Address(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_Drive_Obtain_SCSI_Address(PyObject* self, PyObject* args)
{

}

static PyObject* Burn_MSF_To_Sectors(PyObject* self, PyObject* args)
{
    int sectors, min, sec, frame;
    
    if (!PyArg_ParseTuple(args, "(iii)", &min, &sec, &frame))
        return NULL;
    sectors = burn_msf_to_sectors(min, sec, frame);
    return PyInt_FromLong(sectors);
}

static PyObject* Burn_Sectors_To_MSF(PyObject* self, PyObject* args)
{
    int sectors, min, sec, frame;   

    if (!PyArg_ParseTuple(args, "i", &sectors))
        return NULL;
    burn_sectors_to_msf(sectors, &min, &sec, &frame);
    return PyTuple_Pack(3, PyInt_FromLong(min),
                             PyInt_FromLong(sec), 
                               PyInt_FromLong(frame));
}

static PyObject* Burn_MSF_To_LBA(PyObject* self, PyObject* args)
{
    int lba, min, sec, frame;

    if (!PyArg_ParseTuple(args, "(iii)", &min, &sec, &frame))
        return NULL;
    lba = burn_msf_to_lba(min, sec, frame);
    return PyInt_FromLong(lba);
}

static PyObject* Burn_LBA_To_MSF(PyObject* self, PyObject* args)
{
    int lba, min, sec, frame;

    if (!PyArg_ParseTuple(args, "i", &lba))
        return NULL;
    burn_lba_to_msf(lba, &min, &sec, &frame);
    return PyTuple_Pack(3, PyInt_FromLong(min),
                             PyInt_FromLong(sec),
                               PyInt_FromLong(frame));
}

static PyObject* Burn_Version(PyObject* self, PyObject* args)
{
    int maj, min, mic;

    burn_version(&maj, &min, &mic);
    return PyTuple_Pack(3, PyInt_FromLong(maj),
                             PyInt_FromLong(min), 
                               PyInt_FromLong(mic)); 
}


static PyMethodDef Burn_Methods[] = {
    {"init", (PyCFunction)Burn_Initialize, METH_VARARGS,
        PyDoc_STR("Initializes the libBurn library for use.")},
    {"finish", (PyCFunction)Burn_Finish, METH_VARARGS,
        PyDoc_STR("Shuts down the library.")},
    {"set_verbosity", (PyCFunction)Burn_Set_Verbosity, METH_VARARGS,
        PyDoc_STR("Sets the verbosity level of the library.")},
    {"preset_device_open", (PyCFunction)Burn_Preset_Device_Open, METH_VARARGS,
        PyDoc_STR("Sets parameters for behaviour on opening devices.")},
    {"drive_scan_and_grab", (PyCFunction)Burn_Drive_Scan_And_Grab, METH_VARARGS,
        PyDoc_STR("Acquires a drive with a known persistent address.")},
    {"drive_add_whitelist", (PyCFunction)Burn_Drive_Add_Whitelist, METH_VARARGS,
        PyDoc_STR("Adds a device to the list of permissible drives.")},
    {"drive_clear_whitelist", (PyCFunction)Burn_Drive_Clear_Whitelist,
        METH_VARARGS,
        PyDoc_STR("Removes all drives from the whitelist.")},
    {"drive_scan", (PyCFunction)Burn_Drive_Scan, METH_VARARGS,
        PyDoc_STR("Scans for and returns drive_info objects.")},
    {"drive_info_forget", (PyCFunction)Burn_Drive_Info_Forget, METH_VARARGS,
        PyDoc_STR("Releases memory and frees a drive_info object.")},
    {"drive_info_free", (PyCFunction)Burn_Drive_Info_Free, METH_VARARGS,
        PyDoc_STR("Frees a set of drive_info objects as returned by drive_scan.")},
    {"drive_is_enumerable", (PyCFunction)Burn_Drive_Is_Enumerable,
        METH_VARARGS,
        PyDoc_STR("Evaluates whether the given address is a possible persistent drive address.")},
    {"drive_convert_fs_address", (PyCFunction)Burn_Drive_Convert_FS_Address,
        METH_VARARGS,
        PyDoc_STR("Converts a given filesystem address to a persistent drive address.")},
    {"drive_convert_scsi_address", (PyCFunction)Burn_Drive_Convert_SCSI_Address,
        METH_VARARGS,
        PyDoc_STR("Converts the given SCSI address (bus, channel, target, lun) into a persistent drive address.")},
    {"drive_obtain_scsi_address", (PyCFunction)Burn_Drive_Obtain_SCSI_Address,
        METH_VARARGS,
        PyDoc_STR("Obtains (host, channel, target, lun) from the path.")},
    {"msf_to_sectors", (PyCFunction)Burn_MSF_To_Sectors, METH_VARARGS,
        PyDoc_STR("Converts a (minute, second, frame) value to sector count.")},
    {"sectors_to_msf", (PyCFunction)Burn_Sectors_To_MSF, METH_VARARGS,
        PyDoc_STR("Converts sector count to a (minute, second, frame) value.")},
    {"msf_to_lba", (PyCFunction)Burn_MSF_To_LBA, METH_VARARGS,
        PyDoc_STR("Converts a (minute, second, frame) value to an LBA.")},
    {"lba_to_msf", (PyCFunction)Burn_LBA_To_MSF, METH_VARARGS,
        PyDoc_STR("Converts an LBA to a (minute, second, frame) value.")},
    {"version", (PyCFunction)Burn_Version, METH_VARARGS,
        PyDoc_STR("Returns the library's (major, minor, micro) versions.")},
    {NULL, NULL}
};

static char Burn_Doc[] =
PyDoc_STR("Really stupid bindings to the awesome libBurn library.");

void initburn()
{
    PyObject *module, *dict;
    
    module = Py_InitModule3("burn", Burn_Methods, Burn_Doc);

    Py_INCREF(&DiscType);
    PyModule_AddObject(module, "disc", (PyObject*) &DiscType);

    Py_INCREF(&DriveType);
    PyModule_AddObject(module, "drive", (PyObject*) &DriveType);

    Py_INCREF(&DriveInfoType);
    PyModule_AddObject(module, "drive_info", (PyObject*) &DriveInfoType);

    Py_INCREF(&MessageType);
    PyModule_AddObject(module, "message", (PyObject*) &MessageType);

    Py_INCREF(&ProgressType);
    PyModule_AddObject(module, "progress", (PyObject*) &ProgressType);

    Py_INCREF(&ReadOptsType);
    PyModule_AddObject(module, "read_opts", (PyObject*) &ReadOptsType);

    Py_INCREF(&SessionType);
    PyModule_AddObject(module, "session", (PyObject*) &SessionType);

    Py_INCREF(&SourceType);
    PyModule_AddObject(module, "source", (PyObject*) &SourceType);

    Py_INCREF(&TOCEntryType);
    PyModule_AddObject(module, "toc_entry", (PyObject*) &TOCEntryType);

    Py_INCREF(&TrackType);
    PyModule_AddObject(module, "track", (PyObject*) &TrackType);

    Py_INCREF(&WriteOptsType);
    PyModule_AddObject(module, "write_opts", (PyObject*) &WriteOptsType);

    dict = PyModule_GetDict(module);
    ErrorObject = PyString_FromString("burn.error");
    PyDict_SetItemString(dict, "error", ErrorObject);

    if (PyErr_Occurred())
        Py_FatalError("can't initialize module burn");
    /*
    if (!(dict = PyModule_GetDict(module))
            || !(tmp = PyString_FromString("0.1"))) {
        if (PyErr_Occurred()) {
            PyErr_SetString(PyExc_ImportError, "pyburn: init failed");
        }
    }
    PyDict_SetItemString(dict, "Error", Error);
    PyDict_SetItemString(dict, "version", tmp);
    TODO: Set Constants and enums in disc and DEBUG this!
    */
}
        
