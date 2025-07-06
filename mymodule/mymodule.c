// Include MicroPython API.
#include "py/runtime.h"
#include <string.h>

static const mp_arg_t greet_allowed_args[] = {
  { MP_QSTR_name, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
};

mp_obj_t greet(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  mp_arg_val_t args[MP_ARRAY_SIZE(greet_allowed_args)];
  mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(greet_allowed_args), greet_allowed_args, args);
  const char *name =  mp_obj_str_get_str(args[0].u_obj);
  char message[50];
  snprintf(message, sizeof(message), "Hello, %s!", name);

  return mp_obj_new_str(mesage, strlen(mesage)));
}

static MP_DEFINE_CONST_FUN_OBJ_KW(greet_obj, 0, greet);
static const mp_rom_map_elem_t mymodule_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mymodule) },
    { MP_ROM_QSTR(MP_QSTR_greet), MP_ROM_PTR(&greet_obj) },
static MP_DEFINE_CONST_DICT(mymodule_globals, mymodule_globals_table);

const mp_obj_module_t my_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mymodule_globals,
};
MP_REGISTER_MODULE(MP_QSTR_mymodule, my_cmodule);
