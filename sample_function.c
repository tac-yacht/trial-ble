// Include MicroPython API.
#include "py/runtime.h"
#include <string.h>
#include "WireGuard/src/wireguardif.h" //TODO 構成変更するとずれるので要検討

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

// Wireguard instance
static struct netif wg_netif_struct = {0};
static struct netif *wg_netif = NULL;
static struct netif *previous_default_netif = NULL;
static uint8_t wireguard_peer_index = WIREGUARDIF_INVALID_INDEX;

static mp_obj_t begin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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

	//妥協のデフォルト設定（これなら下のバインドのところでやっても変わらないが、一応バインドとデフォルト設定は別にしておく
    if (args[1].u_obj == MP_OBJ_NULL) { //subnet
        args[1].u_obj = MP_OBJ_NEW_STR("255.255.255.255");
    }
    if (args[2].u_obj == MP_OBJ_NULL) { //gateway
        args[2].u_obj = MP_OBJ_NEW_STR("0.0.0.0");
    }
	//TODO バリデーション
	ip_addr_t ipaddr = ipaddr_from_mp_arg(args[0]);
	ip_addr_t netmask = ipaddr_from_mp_arg(args[1]);
	ip_addr_t geteway = ipaddr_from_mp_arg(args[2]);
	const char *private_key = mp_obj_str_get_str(args[3].u_obj);
	ip_addr_t remote_peer_address = ipaddr_from_mp_arg(args[4]);
	const char *remote_peer_public_key = mp_obj_str_get_str(args[5].u_obj);
	int remote_peer_port = args[6].u_int;

	mp_obj_dict_t *result = mp_obj_new_dict(0);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("local_ip"), mp_obj_from_ipaddr(ipaddr));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("netmask"), mp_obj_from_ipaddr(netmask));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("geteway"), mp_obj_from_ipaddr(geteway));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("private_key"), mp_obj_new_str(private_key, strlen(private_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_address"), mp_obj_from_ipaddr(remote_peer_address));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_public_key"), mp_obj_new_str(remote_peer_public_key, strlen(remote_peer_public_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_port"), mp_obj_new_int(remote_peer_port));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("memo"), MP_OBJ_NEW_STR("デフォルト値設定テスト中"));

	struct wireguardif_init_data wg;
	struct wireguardif_peer peer;

	// Setup the WireGuard device structure
	wg.private_key = private_key;
    wg.listen_port = remote_peer_port;
	
	wg.bind_netif = NULL;

	// Initialise the first WireGuard peer structure
	wireguardif_peer_init(&peer);
	
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
