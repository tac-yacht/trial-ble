
#ifdef __cplusplus
extern "C" {
#endif

#include "py/runtime.h"
#include "py/obj.h"

#define MP_OBJ_NEW_STR(str) mp_obj_new_str(str, sizeof(str) - 1)

mp_obj_t begin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
static const MP_DEFINE_STR_OBJ(subnet_default, "255.255.255.255");
static const MP_DEFINE_STR_OBJ(gateway_default, "0.0.0.0");
const mp_arg_t begin_allowed_args[] = {
	{MP_QSTR_local_ip, MP_ARG_OBJ|MP_ARG_REQUIRED},
	{MP_QSTR_subnet, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(&subnet_default)}},
	{MP_QSTR_gateway, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(&gateway_default)}},
	{MP_QSTR_private_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
	{MP_QSTR_remote_peer_address, MP_ARG_OBJ|MP_ARG_REQUIRED},
	{MP_QSTR_remote_peer_public_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
	{MP_QSTR_remote_peer_port, MP_ARG_INT|MP_ARG_REQUIRED},
};

/*None*/
mp_obj_t end();

/*bool*/
mp_obj_t is_initialized();

#ifdef __cplusplus
}
#endif
