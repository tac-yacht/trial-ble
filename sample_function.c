// Include MicroPython API.
#include "py/runtime.h"

static const mp_arg_t begin_args[] = {
	{MP_QSTR_local_ip, MP_ARG_INT|MP_ARG_REQUIRED},
	{MP_QSTR_private_key, MP_ARG_INT|MP_ARG_REQUIRED},
	{MP_QSTR_endpoint_address, MP_ARG_INT|MP_ARG_REQUIRED},
	{MP_QSTR_public_key, MP_ARG_INT|MP_ARG_REQUIRED},
	{MP_QSTR_endpoint_port, MP_ARG_INT|MP_ARG_REQUIRED},
}

static mp_obj_t begin(size_t n_args, size_t n_kw, const mp_obj_t *args) {
	mp_arg_val_t parsed_args[MP_ARRAY_SIZE(begin_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(begin_args), begin_args, parsed_args);
	
	int arg1 = parsed_args[0].u_int;
	int arg2 = parsed_args[1].u_int;
	int arg3 = parsed_args[2].u_int;
	int arg4 = parsed_args[3].u_int;
	int arg5 = parsed_args[4].u_int;
	
	mp_obj_dict_t *result = mp_obj_new_dict(0);
	mp_obj_dict_store(result, mp_obj_new_str("local_ip"), mp_obj_new_int(arg1));
	mp_obj_dict_store(result, mp_obj_new_str("private_key"), mp_obj_new_int(arg2));
	mp_obj_dict_store(result, mp_obj_new_str("endpoint_addess"), mp_obj_new_int(arg3));
	mp_obj_dict_store(result, mp_obj_new_str("public_key"), mp_obj_new_int(arg4));
	mp_obj_dict_store(result, mp_obj_new_str("endpoint_port"), mp_obj_new_int(arg5));
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

// Define module object.
const mp_obj_module_t wireguard_cmodule = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&wireguard_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_wireguard, wireguard_cmodule);
