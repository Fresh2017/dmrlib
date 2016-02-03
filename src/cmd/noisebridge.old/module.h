#ifndef _NOISEBRIDGE_MODULE_H
#define _NOISEBRIDGE_MODULE_H

typedef enum {
	MOD_START = 0, 				//!< Module starting
	MOD_CLOSE, 					//!< Module stopping
	MOD_ROUTE, 					//!< Repeater route request
	MOD_PROTO_RX, 				//!< Protocol packet received
	MOD_PROTO_TX, 				//!< Protocol packet send
	MOD_COUNT
} mod_func_t;

typedef enum {
	MOD_RETURN_PASS = 0,
	MOD_RETURN_FAIL,
	MOD_RETURN_PERMIT,
	MOD_RETURN_REJECT
} mod_return_t;

typedef int (*mod_init_t)(void *instance);
typedef int (*mod_kill_t)(void *instance);
typedef mod_return_t (*mod_meth_t)(void *instance, void *ptr, ...);

#define MOD_TYPE_THREAD_SAFE 	(0 << 0)
#define MOD_TYPE_THREAD_UNSAFE 	(1 << 0)
#define MOD_TYPE_REHASH_SAFE 	(1 << 1)

typedef struct module_s {
	const char *name;
	int 	   type;
	size_t 	   size;
	mod_init_t init;
	mod_kill_t kill;
	mod_meth_t methods[MOD_COUNT];
} module_t;

#endif // _NOISEBRIDGE_MODULE_H