#include "py/runtime.h"
#include "py/obj.h"

#include "wireguard-mp-wrapper.h"

static MP_DEFINE_CONST_FUN_OBJ_KW(begin_obj, 0, begin);
static MP_DEFINE_CONST_FUN_OBJ_0(end_obj, end);
static MP_DEFINE_CONST_FUN_OBJ_0(is_initialized_obj, is_initialized);


// Define all attributes of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t wireguard_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_wireguard) },
	{ MP_ROM_QSTR(MP_QSTR_begin), MP_ROM_PTR(&begin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_end), MP_ROM_PTR(&end_obj) },
	{ MP_ROM_QSTR(MP_QSTR_is_initialized), MP_ROM_PTR(&is_initialized_obj) },
};
static MP_DEFINE_CONST_DICT(wireguard_module_globals, wireguard_module_globals_table);

// Define module object.
const mp_obj_module_t wireguard_cmodule = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&wireguard_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_wireguard, wireguard_cmodule);
