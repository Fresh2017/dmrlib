#include <Python.h>
#include <talloc.h>
#include <dmr/log.h>
#include <dmr/thread.h>
#include <dmr/type.h>
#include <dmr/packet.h>
#include "module.h"

#define trace(fmt, ...) dmr_log_trace("noisebridge: %s[%d]: " #fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define debug(fmt, ...) dmr_log_debug("noisebridge: %s[%d]: " #fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define error(fmt, ...) dmr_log_error("noisebridge: %s[%d]: " #fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define Pyx_BLOCK_THREADS    {PyGILState_STATE __gstate = PyGILState_Ensure();
#define Pyx_UNBLOCK_THREADS   PyGILState_Release(__gstate);}

dmr_thread_local_setup(PyThreadState *, local_thread_state) /* macro */

struct py_funcdef {
	PyObject	*module;
	PyObject	*function;
	char const	*module_name;
	char const	*function_name;
};

typedef struct {
	PyThreadState *main_thread_state;
	struct py_funcdef
		start,
		close,
		route,
		proto_rx,
		proto_tx;
} mod_python_t;

/* Our single instance of the noisebridge module */
static PyObject *noisebridge_module = NULL;

/* Log wrapper */
static PyObject *mod_dmrlog(PyObject *module, PyObject *args)
{
	int priority;
	char *msg;
	DMR_UNUSED(module);

	if (!PyArg_ParseTuple(args, "is", &priority, &msg)) {
		return NULL;
	}

	dmr_log_message(priority, msg);

	Py_INCREF(Py_None);
	return Py_None;
}

static struct {
	char const *name;
	int 	   value;
} module_constants[] = {
#define C(x) { #x, DMR_LOG_ ## x }
	C(PRIORITY_TRACE),
	C(PRIORITY_DEBUG),
	C(PRIORITY_INFO),
	C(PRIORITY_WARN),
	C(PRIORITY_ERROR),
	C(PRIORITY_CRITICAL),
#undef C
#define C(x) { #x, DMR_ ## x }
	C(TS1),
	C(TS2),
	C(DATA_TYPE_VOICE_PI),
	C(DATA_TYPE_VOICE_LC),
	C(DATA_TYPE_TERMINATOR_WITH_LC),
	C(DATA_TYPE_CSBK),
	C(DATA_TYPE_MBC),
	C(DATA_TYPE_MBCC),
	C(DATA_TYPE_DATA),
	C(DATA_TYPE_RATE12_DATA),
	C(DATA_TYPE_RATE34_DATA),
	C(DATA_TYPE_IDLE),
	C(DATA_TYPE_VOICE_SYNC),
	C(DATA_TYPE_VOICE),
#undef C
	{NULL, 0}
};

static PyMethodDef module_methods[] = {
	{
		"dmrlog", &mod_dmrlog, METH_VARARGS,
		"noisebridge.dmrlog(priority, msg)\n\n" \
		"Print a message using the dmrlib log system."
	},
	{NULL, NULL, 0, NULL} /* sentinel */
};

typedef struct {
	PyObject_HEAD
	dmr_packet_t *packet;
} Packet;

static void Packet_dealloc(Packet *self)
{
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *Packet_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	Packet *self = (Packet *)type->tp_alloc(type, 0);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", NULL))
		return NULL;

	if (self != NULL) {
		self->packet = talloc_zero(NULL, dmr_packet_t);
		if (self->packet == NULL) {
			return NULL;
		}
	}

	return (PyObject *)self;
}

static PyObject *Packet_get_ts(Packet *self, void *closure)
{
	DMR_UNUSED(closure);
	return Py_BuildValue("i", self->packet->ts);
}

static int Packet_set_ts(Packet *self, PyObject *value, void *closure)
{
	long ts = PyInt_AsLong(value);
	DMR_UNUSED(closure);

	if (ts < 0 || ts > 2) {
		PyErr_SetString(PyExc_ValueError, "timeslot must be in range 0-2");
		return -1;
	}
	self->packet->ts = (dmr_ts_t)ts;
	return 0;
}

static PyGetSetDef Packet_getset[] = {
	/*
	dmr_ts_t        ts;
    dmr_id_t        src_id;
    dmr_id_t        dst_id;
    dmr_id_t        repeater_id;
    dmr_call_type_t call_type;
    dmr_slot_type_t slot_type;
    uint8_t         payload[DMR_PAYLOAD_BYTES];
    */
	{"ts", (getter)Packet_get_ts, (setter)Packet_set_ts, "Time slot", NULL},
	{NULL, NULL, NULL, NULL, NULL} /* sentinel */
};

static PyMethodDef Packet_methods[] = {
    { NULL }  /* Sentinel */
};

static PyTypeObject PacketType = {
	PyObject_HEAD_INIT(NULL)
	0,                          /* ob_size*/
    "noisebridge.Packet",       /* tp_name*/
    sizeof(Packet),    	   	    /* tp_basicsize*/
    0,                          /* tp_itemsize*/
    (destructor)Packet_dealloc, /* tp_dealloc*/
    0,                          /* tp_print*/
    0,                          /* tp_getattr*/
    0,                          /* tp_setattr*/
    0,                          /* tp_compare*/
    0,                          /* tp_repr*/
    0,                          /* tp_as_number*/
    0,                          /* tp_as_sequence*/
    0,                          /* tp_as_mapping*/
    0,                          /* tp_hash */
    0,                          /* tp_call*/
    0,                          /* tp_str*/
    0,                          /* tp_getattro*/
    0,                          /* tp_setattro*/
    0,                          /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,         /* tp_flags*/
    "DMR Packet",               /* tp_doc */
    0, 						    /* tp_traverse */
    0, 						    /* tp_clear */
    0, 						    /* tp_richcompare */
    0, 						    /* tp_weaklistoffset */
    0, 						    /* tp_iter */
    0, 						    /* tp_iternext */
    Packet_methods,			    /* tp_methods */
    0, 						    /* tp_members */
    Packet_getset, 			    /* tp_getset */
};

static void mod_error(void)
{
	PyObject *pType = NULL, *pValue = NULL, *pTraceback = NULL, *pStr1 = NULL, *pStr2 = NULL;
	
	/* This will be called with the GIL lock held */
	PyErr_Fetch(&pType, &pValue, &pTraceback);
	if (!pType || !pValue)
		goto failed;
	if (((pStr1 = PyObject_Str(pType)) == NULL) ||
	    ((pStr2 = PyObject_Str(pValue)) == NULL))
		goto failed;

	error("%s (%s)", PyString_AsString(pStr1), PyString_AsString(pStr2));

failed:
	Py_XDECREF(pStr1);
	Py_XDECREF(pStr2);
	Py_XDECREF(pType);
	Py_XDECREF(pValue);
	Py_XDECREF(pTraceback);
}

static void python_cleanup(void *arg)
{
	PyThreadState *my_thread_state = arg;
	PyEval_AcquireLock();
	PyThreadState_Swap(NULL);	/* Not entirely sure this is needed */
	PyThreadState_Clear(my_thread_state);
	PyThreadState_Delete(my_thread_state);
	PyEval_ReleaseLock();
}

static int python_thread_state_init(mod_python_t *mod, PyGILState_STATE gstate, PyThreadState *prev)
{
	int ret = 0;
	PyThreadState *my_thread_state;
	DMR_UNUSED(prev);  // TODO(pd0mz): Why is this required on newer gcc?

	my_thread_state = dmr_thread_local_init(local_thread_state, python_cleanup);
	if (my_thread_state == NULL) {
		my_thread_state = PyThreadState_New(mod->main_thread_state->interp);
		debug("new thread state %p", my_thread_state);
		if (my_thread_state == NULL) {
			debug("thread state initialize failed");
			PyGILState_Release(gstate);
			return -1;
		}
		ret = dmr_thread_local_set(local_thread_state, my_thread_state);
		if (ret != 0) {
			debug("failed storing PyThreadState: %s", strerror(ret));
			PyThreadState_Clear(my_thread_state);
			PyThreadState_Delete(my_thread_state);
			PyGILState_Release(gstate);
			return -1;
		}
	}
	debug("using thread state %p", my_thread_state);
		prev = PyThreadState_Swap(my_thread_state);
	return ret;
}

static int run_python(mod_python_t *mod, PyObject *pFunc, PyObject *pArgs, const char *function_name, bool worker)
{
	int ret = MOD_RETURN_PASS;
	PyGILState_STATE gstate;
	PyThreadState *prev_thread_state = NULL;
	PyObject *pRet;

	// Return OK if there's nothing to do.
	if (pFunc == NULL)
		return ret;

	memset(&gstate, 0, sizeof(gstate));
	gstate = PyGILState_Ensure();
	if (worker) {
		ret = python_thread_state_init(mod, gstate, prev_thread_state);
		if (ret != 0) {
			return MOD_RETURN_FAIL;
		}
	}

	if (pArgs == NULL) {
		Py_INCREF(Py_None);
		pArgs = Py_None;
	}

	pRet = PyObject_CallFunctionObjArgs(pFunc, pArgs, NULL);
	if (pRet == NULL) {
		// Function call failed (raised an exception)
		ret = MOD_RETURN_FAIL;
		goto run_python_finish;
	}

	/* The function can return serveral types */
	if (PyInt_CheckExact(pRet)) {
		// Module returned an integer value
		ret = PyInt_AsLong(pRet);
	} else if (pRet == Py_None) {
		// Module returned None
		ret = MOD_RETURN_PASS;
	} else {
		// Module returned an unsupported value
		error("%s: did not return an int or None", function_name);
		ret = MOD_RETURN_FAIL;
		goto run_python_finish;
	}

run_python_finish:
	Py_XDECREF(pArgs);
	Py_XDECREF(pRet);

	if (worker) {
		PyThreadState_Swap(prev_thread_state);
	}
	PyGILState_Release(gstate);

	return ret;
}

static int mod_init_module(void *instance)
{
	mod_python_t *mod = (mod_python_t *)instance;
	int i;

	dmr_log_trace("noisebridge: mod_python: init");

	if (noisebridge_module != NULL) {
		dmr_log_debug("noisebridge: mod_python: already initialized");
		return 0;
	}

	Py_SetProgramName("noisebridge");
	Py_InitializeEx(0);
	PyEval_InitThreads();
	mod->main_thread_state = PyThreadState_Get();

	if ((noisebridge_module = Py_InitModule3("noisebridge", module_methods, "mod_python module")) == NULL) {
		goto failed;
	}

	// Export module constants
	for (i = 0; module_constants[i].name != NULL; i++) {
		if ((PyModule_AddIntConstant(noisebridge_module, module_constants[i].name, module_constants[i].value)) < 0) {
			goto failed;
		}
	}

	// Export object types
	PacketType.tp_new = Packet_new;
	if (PyType_Ready(&PacketType) < 0)
		goto failed;

	Py_INCREF(&PacketType);
	PyModule_AddObject(noisebridge_module, "Packet", (PyObject *)&PacketType);

	PyThreadState_Swap(NULL);	/* We have to swap out the current thread else we get deadlocks */
	PyEval_ReleaseLock();		/* Drop lock grabbed by InitThreads */
	dmr_log_debug("noisebridge: mod_python: init done");
	return 0;

failed:
	if (noisebridge_module != NULL) {
		Py_XDECREF(noisebridge_module);
	}

	PyEval_ReleaseLock();
	Pyx_BLOCK_THREADS
	mod_error();
	Pyx_UNBLOCK_THREADS

	noisebridge_module = NULL;
	Py_Finalize();
	return -1;
}

static mod_return_t mod_close(void *instance, void *ptr, ...)
{
	int ret = 0;
	mod_python_t *mod = (mod_python_t *)instance;
	DMR_UNUSED(ptr);
	
	if (mod == NULL)
		return -1;

	return ret;
}

static mod_return_t mod_start(void *instance, void *ptr, ...)
{
	int ret = 0;
	mod_python_t *mod = (mod_python_t *)instance;
	DMR_UNUSED(ptr);

	if (mod == NULL)
		return -1;

	return ret;
}

static mod_return_t mod_route(void *instance, void *ptr, ...)
{
	mod_python_t *mod = (mod_python_t *)instance;
	dmr_packet_t *packet = (dmr_packet_t *)ptr;
	if (mod == NULL || packet == NULL)
		return -1;

	return 0;
}

static mod_return_t mod_proto_rx(void *instance, void *ptr, ...)
{
	mod_python_t *mod = (mod_python_t *)instance;
	dmr_packet_t *packet = (dmr_packet_t *)ptr;
	if (mod == NULL || packet == NULL)
		return -1;

	return 0;
}

static mod_return_t mod_proto_tx(void *instance, void *ptr, ...)
{
	mod_python_t *mod = (mod_python_t *)instance;
	dmr_packet_t *packet = (dmr_packet_t *)ptr;
	if (mod == NULL || packet == NULL)
		return -1;

	PyObject *object = PyObject_CallObject((PyObject *)&PacketType, Py_None);
	if (object == NULL) {
		error("failed to create packet");
		return MOD_RETURN_FAIL;
	}
	Packet *pPacket = (Packet *)object;
	pPacket->packet = packet;
	Py_INCREF(object);

	return run_python(mod, mod->proto_tx.function, object, "proto_tx", true);
}

static void mod_objclear(PyObject **ob)
{
	if (*ob != NULL) {
		Pyx_BLOCK_THREADS
		Py_DECREF(*ob);
		Pyx_UNBLOCK_THREADS
		*ob = NULL;
	}
}

static void mod_funcdef_clear(struct py_funcdef *def)
{
	mod_objclear(&def->function);
	mod_objclear(&def->module);
}

static void mod_funcdefs_clear(mod_python_t *mod)
{
	mod_funcdef_clear(&mod->close);
	mod_funcdef_clear(&mod->start);
	mod_funcdef_clear(&mod->route);
	mod_funcdef_clear(&mod->proto_rx);
	mod_funcdef_clear(&mod->proto_tx);
}

static int mod_funcdef(struct py_funcdef *def)
{
	char const *funcname = "mod_funcdef";
	PyGILState_STATE gstate = PyGILState_Ensure();

	if (def->module_name != NULL && def->function_name != NULL) {
		if ((def->module = PyImport_ImportModule(def->module_name)) == NULL) {
			error("%s: module \"%s\" not found", funcname, def->module_name);
			goto failed;
		}
		if ((def->function = PyObject_GetAttrString(def->module, def->function_name)) == NULL) {
			error("%s: function \"%s.%s\" not found", funcname, def->module_name, def->function_name);
			goto failed;
		}
		if (!PyCallable_Check(def->function)) {
			error("%s: function \"%s.%s\" not callable", funcname, def->module_name, def->function_name);
			goto failed;
		}
	}
	PyGILState_Release(gstate);
	return 0;

failed:
	mod_error();
	error("%s: failed to import python function \"%s.%s\"", funcname, def->module_name, def->function_name);
	Py_XDECREF(def->function);
	def->function = NULL;
	Py_XDECREF(def->module);
	def->module = NULL;
	PyGILState_Release(gstate);
	return -1;
}

static int mod_init(void *instance)
{
	mod_python_t *mod = (mod_python_t *)instance;

	if (mod_init_module(mod) != 0)
		return -1;

#define C(x) if (mod_funcdef(&mod->x) < 0) goto failed
	C(close);
	C(start);
	C(route);
	C(proto_rx);
	C(proto_tx);
#undef C

	return run_python(mod, NULL, mod->start.function, "start", false);

failed:

	Pyx_BLOCK_THREADS
	mod_error();
	Pyx_UNBLOCK_THREADS
	mod_funcdefs_clear(mod);
	return -1;
}

static int mod_kill(void *instance)
{
	mod_python_t *mod = (mod_python_t *)instance;
	int ret = run_python(mod, mod->close.function, NULL, "close", false);

	mod_funcdefs_clear(mod);

	return ret;
}

extern module_t module;
module_t module = {
	.name = "python",
	.type = MOD_TYPE_THREAD_SAFE,
	.size = sizeof(mod_python_t),
	.init = mod_init,
	.kill = mod_kill,
	.methods = {
		[MOD_START]    = mod_start,
		[MOD_CLOSE]    = mod_close,
		[MOD_ROUTE]    = mod_route,
		[MOD_PROTO_RX] = mod_proto_rx,
		[MOD_PROTO_TX] = mod_proto_tx
	}
};