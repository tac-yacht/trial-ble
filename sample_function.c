// Include MicroPython API.
#include "py/runtime.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/ip.h"
#include "lwip/netdb.h"
#include "lwip/err.h"

//TODO 構成変更するとずれるので要検討
#include "WireGuard/src/wireguardif.h"
#include "WireGuard/src/wireguard-platform.h"

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
static bool is_initialized = false;

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
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("memo"), MP_OBJ_NEW_STR("初期設定テスト中"));

	struct wireguardif_init_data wg;
	struct wireguardif_peer peer;

	// Setup the WireGuard device structure
	wg.private_key = private_key;
	wg.listen_port = remote_peer_port;
	
	wg.bind_netif = NULL;

	// Initialise the first WireGuard peer structure
	wireguardif_peer_init(&peer);
	// If we know the endpoint's address can add here
	bool success_get_endpoint_ip = false;
	for(int retry = 0; retry < 5; retry++) {
		ip_addr_t endpoint_ip = IPADDR4_INIT_BYTES(0, 0, 0, 0);
		struct addrinfo *res = NULL;
		struct addrinfo hint;
		memset(&hint, 0, sizeof(hint));
		memset(&endpoint_ip, 0, sizeof(endpoint_ip));
		if( lwip_getaddrinfo(remote_peer_address, NULL, &hint, &res) != 0 ) {
			vTaskDelay(pdMS_TO_TICKS(2000));
			continue;
		}
		success_get_endpoint_ip = true;
		struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
		inet_addr_to_ip4addr(ip_2_ip4(&endpoint_ip), &addr4);
		lwip_freeaddrinfo(res);

		peer.endpoint_ip = endpoint_ip;
		break;
	}
	if( !success_get_endpoint_ip  ) {
		mp_obj_dict_store(result, MP_OBJ_NEW_STR("error_log"), MP_OBJ_NEW_STR("failed to get endpoint ip."));
		return result;
	}
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("endpoint_ip"), mp_obj_from_ipaddr(peer.endpoint_ip));

	// Register the new WireGuard network interface with lwIP
	wg_netif = netif_add(&wg_netif_struct, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gateway), &wg, &wireguardif_init, &ip_input);
	if( wg_netif == NULL ) {
		mp_obj_dict_store(result, MP_OBJ_NEW_STR("error_log"), MP_OBJ_NEW_STR("failed to initialize WG netif."));
		return result;
	}
	// Mark the interface as administratively up, link up flag is set automatically when peer connects
	netif_set_up(wg_netif);

	peer.public_key = remote_peer_public_key;
	peer.preshared_key = NULL;
	// Allow all IPs through tunnel
	{
		ip_addr_t allowed_ip = IPADDR4_INIT_BYTES(0, 0, 0, 0);
		peer.allowed_ip = allowed_ip;
		ip_addr_t allowed_mask = IPADDR4_INIT_BYTES(0, 0, 0, 0);
		peer.allowed_mask = allowed_mask;
	}
	
	peer.endport_port = remote_peer_port;

    // Initialize the platform
    wireguard_platform_init();
	// Register the new WireGuard peer with the netwok interface
	wireguardif_add_peer(wg_netif, &peer, &wireguard_peer_index);
	if ((wireguard_peer_index != WIREGUARDIF_INVALID_INDEX) && !ip_addr_isany(&peer.endpoint_ip)) {
		// Start outbound connection to peer
		mp_obj_dict_store(result, MP_OBJ_NEW_STR("log"), MP_OBJ_NEW_STR("connecting wireguard..."));
		err_t connect_result = wireguardif_connect(wg_netif, wireguard_peer_index);
		const char* connect_result_str = lwip_strerr(connect_result);
		mp_obj_dict_store(result, MP_OBJ_NEW_STR("connect_result"), mp_obj_new_str(connect_result_str, strlen(connect_result_str)));
		// Save the current default interface for restoring when shutting down the WG interface.
		previous_default_netif = netif_default;
		// Set default interface to WG device.
        netif_set_default(wg_netif);
	}

	is_initialized = true;
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("log"), MP_OBJ_NEW_STR("connected!"));
	return result;
};

static MP_DEFINE_CONST_FUN_OBJ_KW(begin_obj, 0, begin);

static mp_obj_t end() {
	if( !is_initialized ) return mp_const_none;

	// Restore the default interface.
	netif_set_default(previous_default_netif);
	previous_default_netif = NULL;
	// Disconnect the WG interface.
	wireguardif_disconnect(wg_netif, wireguard_peer_index);
	// Remove peer from the WG interface
	wireguardif_remove_peer(wg_netif, wireguard_peer_index);
	wireguard_peer_index = WIREGUARDIF_INVALID_INDEX;
	// Shutdown the wireguard interface.
	wireguardif_shutdown(wg_netif);
	// Remove the WG interface;
	netif_remove(wg_netif);
	wg_netif = NULL;

	is_initialized = false;
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(end_obj, end);

// Define all attributes of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t wireguard_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_wireguard) },
	{ MP_ROM_QSTR(MP_QSTR_begin), MP_ROM_PTR(&begin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_end), MP_ROM_PTR(&end_obj) },
};
static MP_DEFINE_CONST_DICT(wireguard_module_globals, wireguard_module_globals_table);

// Define module object.
const mp_obj_module_t wireguard_cmodule = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&wireguard_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_wireguard, wireguard_cmodule);
