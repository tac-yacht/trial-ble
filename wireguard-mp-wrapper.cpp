// Include MicroPython API.
#include "py/runtime.h"
#include "py/obj.h"

#include "lwip/ip.h"

#include "WireGuard-ESP32.h"
#include "Wireguardd-mp-wrapper.h"

static WireGuard instance;

#define MP_OBJ_NEW_STR(str) mp_obj_new_str(str, sizeof(str) - 1)

//utility
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

extern "C" {

mp_obj_t begin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_local_ip, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_subnet, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}}, //あとで直接指定する方法を調べる
		{MP_QSTR_gateway, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
		{MP_QSTR_private_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_remote_peer_address, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_remote_peer_public_key, MP_ARG_OBJ|MP_ARG_REQUIRED},
		{MP_QSTR_remote_peer_port, MP_ARG_INT|MP_ARG_REQUIRED},
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	//TODO バリデーション
	ip_addr_t ipaddr = ipaddr_from_mp_arg(args[0]);
	ip_addr_t netmask = ipaddr_from_mp_arg(args[1]);
	ip_addr_t gateway = ipaddr_from_mp_arg(args[2]);
	const char *private_key = mp_obj_str_get_str(args[3].u_obj);
	const char *remote_peer_address = mp_obj_str_get_str(args[4].u_obj);
	const char *remote_peer_public_key = mp_obj_str_get_str(args[5].u_obj);
	int remote_peer_port = args[6].u_int;

	mp_obj_dict_t *result = mp_obj_new_dict(0);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("local_ip"), mp_obj_from_ipaddr(ipaddr));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("netmask"), mp_obj_from_ipaddr(netmask));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("geteway"), mp_obj_from_ipaddr(gateway));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("private_key"), mp_obj_new_str(private_key, strlen(private_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_address"), mp_obj_new_str(remote_peer_address, strlen(remote_peer_address)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_public_key"), mp_obj_new_str(remote_peer_public_key, strlen(remote_peer_public_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_port"), mp_obj_new_int(remote_peer_port));

	bool wg_result = instance.begin(ipaddr, netmask, gateway, private_key, remote_peer_address, remote_peer_public_key, remote_peer_port);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("result"), mp_obj_new_bool(wg_result));
	return result;
}

mp_obj_t end() {
	instance.end();
	return mp_const_none;
}
mp_obj_t is_initialized() {
	return mp_obj_new_bool(instance.is_initialized()));
}

}