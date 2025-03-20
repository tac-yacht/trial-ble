// Include MicroPython API.
#include "py/runtime.h"
#include <string.h>
#include "lwip/ip_addr.h"

#define MP_OBJ_NEW_STR(str) mp_obj_new_str(str, sizeof(str) - 1)

static ip_addr_t ipaddr_from_mp_arg(mp_arg_val_t arg) {
	//TODO バリデーション
	const char *ipaddr_str = mp_obj_str_get_str(arg.u_obj);
	ip_addr_t result;
	ipaddr_aton(ipaddr_str, &result);
	
	return result;
}
static mp_obj_t mp_obj_from_ipaddr(ip_addr_t src) {
	const char *ipaddr_str = ipaddr_ntoa(&src);
	return mp_obj_new_str(ipaddr_str, strlen(ipaddr_str));
}

static mp_obj_t begin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_local_ip, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_private_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_endpoint_address, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_public_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_endpoint_port, MP_ARG_INT|MP_ARG_REQUIRED},
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	//TODO バリデーション
	ip_addr_t local_ip = ipaddr_from_mp_arg(args[0]);
	const char *private_key = mp_obj_str_get_str(args[1].u_obj);
	const char *endpoint_address = mp_obj_str_get_str(args[2].u_obj);
	const char *public_key = mp_obj_str_get_str(args[3].u_obj);
	int endpoint_port = args[4].u_int;
	
	mp_obj_dict_t *result = mp_obj_new_dict(0);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("local_ip"), mp_obj_from_ipaddr(local_ip));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("private_key"), mp_obj_new_str(private_key, strlen(private_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("endpoint_address"), mp_obj_new_str(endpoint_address, strlen(endpoint_address)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("public_key"), mp_obj_new_str(public_key, strlen(public_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("endpoint_port"), mp_obj_new_int(endpoint_port));
	return result;
};

static MP_DEFINE_CONST_FUN_OBJ_KW(begin_obj, 0, begin);

// Define all attributes of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t wireguard_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_wireguard) },
	{ MP_ROM_QSTR(MP_QSTR_begin), MP_ROM_PTR(&begin_obj) },
};
static MP_DEFINE_CONST_DICT(wireguard_module_globals, wireguard_module_globals_table);

// Define module object.
const mp_obj_module_t wireguard_cmodule = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&wireguard_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_wireguard, wireguard_cmodule);
