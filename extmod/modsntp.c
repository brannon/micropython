
#include <stdlib.h>

#include "py/mperrno.h"
#include "py/runtime.h"

#include "netutils.h"

#include "lwip/udp.h"

typedef struct _sntp_callback_context_t {
	struct udp_pcb* udp;
	mp_obj_t addr;
	mp_obj_t callback;
} sntp_callback_context_t;

STATIC void _sntp_udp_incoming(void* arg, struct udp_pcb* upcb, struct pbuf* p, ip_addr_t* addr, u16_t port) {
	sntp_callback_context_t* context = (sntp_callback_context_t*)arg;
	if (context) {
		// TODO: parse pbuf
		mp_call_function_1(context->callback, context->addr);
		free(context);
	}
	pbuf_free(p);
}

STATIC mp_obj_t sntp_gettime(mp_obj_t addr_in, mp_obj_t callback_in) {
    uint8_t ip[NETUTILS_IPV4ADDR_BUFSIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_BIG);

	if (!mp_obj_is_callable(callback_in)) {
		nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "callback parameter is not callable"));
	}

    struct udp_pcb* udp = udp_new();
	if (!udp) {
		nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(MP_ENOMEM)));
	}

	sntp_callback_context_t* context;
	context = malloc(sizeof(sntp_callback_context_t));
	if (!context) {
		nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(MP_ENOMEM)));
	}

	context->udp = udp;
	context->addr = addr_in;
	context->callback = callback_in;

	udp_recv(udp, _sntp_udp_incoming, (void*)context);

	struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, 48, PBUF_RAM);
	ip_addr_t dest;
	err_t err;

	IP4_ADDR(&dest, ip[0], ip[1], ip[2], ip[3]);

	// TODO: fill out pbuf
	((unsigned char*)p->payload)[0] = 0x1b;

	err = udp_sendto(udp, p, &dest, port);
	if (err != ERR_OK) {
		free(context);
		nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(err)));
	}

	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(sntp_gettime_obj, sntp_gettime);

STATIC const mp_map_elem_t mp_module_sntp_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_sntp) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_gettime), (mp_obj_t)&sntp_gettime_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_sntp_globals, mp_module_sntp_globals_table);

const mp_obj_module_t mp_module_sntp = {
	.base = { &mp_type_module },
	.name = MP_QSTR_sntp,
	.globals = (mp_obj_dict_t*)&mp_module_sntp_globals,
};
