#include <string>
extern "C" {
// Include MicroPython API.
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
}

#include "lwip/ip.h"

//TODO 構成変更するとパスがずれるので要検討
#include "WireGuard/src/WireGuard-ESP32.h"

#include "wireguard-mp-wrapper.h"

//utility
/**
 * 汎用関数
 * @brief Python引数から、lwipの型へ変換する
 * 変換失敗時はPythonの例外が投げられる
 * @param arg 変換するPython引数(文字列)
 * @param key_name キーワード名(無ければ空文字を指定)
 */
static ip_addr_t ipaddr_from_mp_arg(mp_arg_val_t arg, const std::string& kw_name) {
	std::string prefix = "";
	if (!kw_name.empty()) {
		prefix = "'" + kw_name + "' ";
	}

	if (arg.u_obj == nullptr || !mp_obj_is_str(arg.u_obj)) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "%sexpected a string IP address", prefix.c_str()));
	}

	const char *ipaddr_str = mp_obj_str_get_str(arg.u_obj);
	ip_addr_t result;
	if(!ipaddr_aton(ipaddr_str, &result)) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "%sinvalid IP [raw value: %s]", prefix.c_str(), ipaddr_str));
	}
	
	return result;
}
static mp_obj_t mp_obj_from_ipaddr(ip_addr_t src) {
	const char *ipaddr_str = ipaddr_ntoa(&src);
	return mp_obj_new_str(ipaddr_str, strlen(ipaddr_str));
}

static bool mp_obj_is_bytes(mp_obj_t o) {
	return mp_obj_is_type(o, &mp_type_bytes);
}
static mp_obj_t a2b_base64(const std::string& s) {
	mp_obj_t s_mp = mp_obj_new_str(s.c_str(), s.length());

	// binasciiモジュールをインポート
	mp_obj_t binascii_module = mp_import_name(MP_QSTR_binascii, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
	mp_obj_t a2b_base64_func = mp_load_attr(binascii_module, MP_QSTR_a2b_base64);
	
	// b64decodeを呼び出し
	return mp_call_function_1(a2b_base64_func, s_mp);
}
static const char* b2a_base64(mp_obj_t s) {
	// binasciiモジュールをインポート
	mp_obj_t binascii_module = mp_import_name(MP_QSTR_binascii, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
	mp_obj_t b2a_base64_func = mp_load_attr(binascii_module, MP_QSTR_a2b_base64);
	
	// b64decodeを呼び出し
	mp_obj_t result = mp_call_function_1(b2a_base64_func, s);
	return mp_call_function_0(mp_load_attr(result, MP_QSTR_decode));
}

static const char* key_from_mp_arg(mp_arg_val_t arg, const std::string& kw_name) {
	std::string prefix = "";
	if (!kw_name.empty()) {
		prefix = "'" + kw_name + "' ";
	}

	if (arg.u_obj == nullptr || !mp_obj_is_str_or_bytes(arg.u_obj)) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "%sexpected a string or bytes", prefix.c_str()));
	}

	const char *raw = mp_obj_is_str(arg.u_obj)? mp_obj_str_get_str(arg.u_obj) : b2a_base64(arg.u_obj);

	mp_obj_t decode_result = mp_obj_is_str(arg.u_obj)? a2b_base64(std::string(raw)) : arg.u_obj;
	const size_t key_length = 32;
	const size_t result_length = mp_obj_get_int(mp_obj_len(decode_result));
	if(result_length != key_length) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "%sexpected key length is %d bytes. [actual: %d]", prefix.c_str(), key_length, result_length));
	}

	return raw;
}

//helper
/**
 * begin専用
 */
static ip_addr_t get_ip(mp_arg_val_t *args, int index) {
	return ipaddr_from_mp_arg(args[index], std::string(qstr_str(begin_allowed_args[index].qst)));
}

static const char* get_key(mp_arg_val_t *args, int index) {
	return key_from_mp_arg(args[index], std::string(qstr_str(begin_allowed_args[index].qst)));
}


static WireGuard* instance = nullptr;
void init() {
	if(instance) return;
	instance = new WireGuard();
}

void destroy() {
	if(!instance) return;

	instance->end();
	delete instance;
	instance = nullptr;
}


extern "C" {

mp_obj_t begin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	init();

	mp_arg_val_t args[MP_ARRAY_SIZE(begin_allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(begin_allowed_args), begin_allowed_args, args);

	//TODO バリデーション
	ip_addr_t ipaddr = get_ip(args, 0);
	ip_addr_t netmask = get_ip(args, 1);
	ip_addr_t gateway = get_ip(args, 2);
	const char *private_key = get_key(args, 3);
	const char *remote_peer_address = mp_obj_str_get_str(args[4].u_obj);
	const char *remote_peer_public_key = get_key(args, 5);
	int remote_peer_port = args[6].u_int;

	mp_obj_dict_t *result = (mp_obj_dict_t *)mp_obj_new_dict(0);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("local_ip"), mp_obj_from_ipaddr(ipaddr));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("netmask"), mp_obj_from_ipaddr(netmask));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("geteway"), mp_obj_from_ipaddr(gateway));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("private_key"), mp_obj_new_str(private_key, strlen(private_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_address"), mp_obj_new_str(remote_peer_address, strlen(remote_peer_address)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_public_key"), mp_obj_new_str(remote_peer_public_key, strlen(remote_peer_public_key)));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("remote_peer_port"), mp_obj_new_int(remote_peer_port));
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("memo"), MP_OBJ_NEW_STR("キーのバリデーション追加中"));

	if(args[7].u_bool) { //dry_run=Trueの時
		//接続処理せずに終える
		return result;
	}

	bool wg_result = instance->begin(ipaddr, netmask, gateway, private_key, remote_peer_address, remote_peer_public_key, remote_peer_port);
	mp_obj_dict_store(result, MP_OBJ_NEW_STR("result"), mp_obj_new_bool(wg_result));

	return result;
}

mp_obj_t end() {
	destroy();
	return mp_const_none;
}

mp_obj_t is_initialized() {
	if(!instance) return mp_obj_new_bool(false);
	return mp_obj_new_bool(instance->is_initialized());
}

}