#include "luat_base.h"
#include "luat_sys.h"
#include "luat_mobile.h"


// #define LUAT_LOG_TAG "mobile"

int luat_mobile_get_imei(int sim_id, char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_get_sn(char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_set_sn(char* buff, uint8_t buf_len)
{
    return 0;
}
int luat_mobile_get_muid(char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_get_iccid(int sim_id, char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_get_imsi(int sim_id, char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_get_sim_number(int sim_id, char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_get_sim_id(int *id)
{
    return 0;
}
int luat_mobile_set_sim_id(int id)
{
    return 0;
}
int luat_mobile_set_sim_pin(int id, uint8_t operation, char pin1[9], char pin2[9])
{
    return 0;
}
uint8_t luat_mobile_get_sim_ready(int id)
{
    return 0;
}
void luat_mobile_set_sim_detect_sim0_first(void)
{
    
}
void luat_mobile_set_default_pdn_ipv6(uint8_t onoff)
{

}
void luat_mobile_set_default_pdn_only_ipv6(uint8_t onoff)
{

}
uint8_t luat_mobile_get_default_pdn_ipv6(void)
{

    return 0;
}
int luat_mobile_get_apn(int sim_id, int cid, char* buff, size_t buf_len)
{
    return 0;
}
void luat_mobile_user_ctrl_apn(void)
{   

}
void luat_mobile_user_ctrl_apn_stop(void)
{

}
int luat_mobile_set_apn_base_info(int sim_id, int cid, uint8_t type, uint8_t* apn_name, uint8_t name_len)
{
    return 0;
}

int luat_mobile_active_apn(int sim_id, int cid, uint8_t state)
{
    return 0;
}
int luat_mobile_active_netif(int sim_id, int cid)
{
    return 0;
}
void luat_mobile_user_apn_auto_active(int sim_id, uint8_t cid,
		uint8_t ip_type,
		uint8_t protocol_type,
		uint8_t *apn_name, uint8_t apn_name_len,
		uint8_t *user, uint8_t user_len,
		uint8_t *password, uint8_t password_len)
{

}
int luat_mobile_get_default_apn(int sim_id, char* buff, size_t buf_len)
{
    return 0;
}
int luat_mobile_del_apn(int sim_id, uint8_t cid, uint8_t is_dedicated)
{
    return 0;
}
int luat_mobile_set_flymode(int index, int mode)
{
    return 0;
}
int luat_mobile_get_flymode(int index)
{
    return 0;
}
int luat_mobile_get_local_ip(int sim_id, int cid, ip_addr_t *ip_v4, ip_addr_t *ip_v6)
{
    return 0;
}
int luat_mobile_get_cell_info(luat_mobile_cell_info_t  *info)
{
    return 0;
}
int luat_mobile_get_cell_info_async(uint8_t max_time)
{
    return 0;
}
int luat_mobile_get_cell_info_async_with_sim_id(uint8_t sim_id)
{
    return 0;
}
int luat_mobile_get_last_notify_cell_info(luat_mobile_cell_info_t  *info)
{
    return 0;
}
int luat_mobile_get_last_notify_cell_info_with_sim_id(uint8_t sim_id, luat_mobile_cell_info_t  *info)
{
    return 0;
}
void luat_mobile_print_last_notify_cell_info_with_sim_id(uint8_t sim_id)
{   

}
uint8_t luat_mobile_rssi_to_csq(int8_t rssi)
{
    return 0;
}
int luat_mobile_get_signal_strength_info(luat_mobile_signal_strength_info_t *info)
{   
    return 0;
}
int luat_mobile_get_signal_strength(uint8_t *csq)
{   
    return 0;
}
int luat_mobile_get_last_notify_signal_strength_info(luat_mobile_signal_strength_info_t *info)
{
    return 0;
}
int luat_mobile_get_last_notify_signal_strength(uint8_t *csq)
{
    return 0;
}
int luat_mobile_get_service_cell_identifier(uint32_t *eci)
{
    return 0;
}
int luat_mobile_get_service_tac_or_lac(uint16_t *tac)
{
    return 0;
}

LUAT_MOBILE_SIM_STATUS_E luat_mobile_get_sim_status(void)
{
    return 0;
}

LUAT_MOBILE_REGISTER_STATUS_E luat_mobile_get_register_status(void)
{
    return 0;
}
int luat_mobile_event_register_handler(luat_mobile_event_callback_t callback_fun)
{
    return 0;
}
int luat_mobile_event_deregister_handler(void)
{
    return 0;
}
int luat_mobile_sms_sdk_event_register_handler(luat_mobile_sms_event_callback_t callback_fun)
{
    return 0;
}
int luat_mobile_sms_event_register_handler(luat_mobile_sms_event_callback_t callback_fun)
{
    return 0;
}

#ifndef __USE_SDK_LWIP__
 void net_lwip_check_switch(uint8_t onoff)
{
	
}
#endif
void luat_mobile_set_rrc_auto_release_time(uint8_t s)
{

}
//实验性质API，请勿使用
void luat_mobile_set_auto_rrc(uint8_t s1, uint32_t s2)
{

}
void luat_mobile_set_auto_rrc_default(void)
{
    
}
void luat_mobile_rrc_auto_release_pause(uint8_t onoff)
{

}
void luat_mobile_rrc_release_once(void)
{   

}
int luat_mobile_reset_stack(void)
{
    return 0;
}
void luat_mobile_fatal_error_auto_reset_stack(uint8_t onoff)
{   

}
int luat_mobile_set_period_work(uint32_t get_cell_period, uint32_t check_sim_period, uint8_t search_cell_time)
{
    return 0;
}
int luat_mobile_set_check_sim(uint32_t check_sim_period)
{
    return 0;
}
void luat_mobile_set_check_network_period(uint32_t period)
{
    
}
void luat_mobile_get_ip_data_traffic(uint64_t *uplink, uint64_t *downlink)
{

}
void luat_mobile_clear_ip_data_traffic(uint8_t clear_uplink, uint8_t clear_downlink)
{

}
int luat_mobile_get_support_band(uint8_t *band,  uint8_t *total_num)
{
    return 0;
}
int luat_mobile_get_band(uint8_t *band,  uint8_t *total_num)
{
    return 0;
}
int luat_mobile_set_band(uint8_t *band,  uint8_t total_num)
{
    return 0;
}
int luat_mobile_config(uint8_t item, uint32_t value)
{
    return 0;
}
void luat_mobile_rf_test_mode(uint8_t uart_id, uint8_t on_off)
{   

}
void luat_mobile_rf_test_input(char *data, uint32_t data_len)
{
    
}
uint32_t luat_mobile_sim_write_counter(void)
{
    return 0;
}
int luat_mobile_get_isp_from_plmn(uint16_t mcc, uint8_t mnc)
{
    return 0;
}
int luat_mobile_get_plmn_from_imsi(char *imsi, uint16_t *mcc, uint8_t *mnc)
{
    return 0;
}
void luat_mobile_get_last_call_num(char *buf, uint8_t buf_len)
{

}
int luat_mobile_make_call(uint8_t sim_id, char *number, uint8_t len)
{
    return 0;
}
void luat_mobile_hangup_call(uint8_t sim_id)
{

}
int luat_mobile_answer_call(uint8_t sim_id)
{
    return 0;
}
int luat_mobile_speech_init(uint8_t multimedia_id,void *callback)
{
    return 0;
}
int luat_mobile_speech_upload(uint8_t *data, uint32_t len)
{
    return 0;
}
void luat_mobile_set_sync_time(uint8_t on_off)
{
    
}
uint8_t luat_mobile_get_sync_time(void)
{
    return 0;
}
int luat_mobile_softsim_onoff(uint8_t on_off)
{   
    return 0;
}
int luat_mobile_sim_detect_onoff(uint8_t on_off)
{
    return 0;
}
void luat_mobile_softsim_init_default(void)
{   

}
int luat_mobile_lock_cell(uint32_t op, uint32_t earfcn, uint16_t pci)
{
    return 0;
}
int luat_mobile_get_extern_service_cell_info(luat_mobile_scell_extern_info_t *info)
{
    return 0;
}
void luat_mobile_vsim_user_heartbeat_once(void)
{   

}
uint32_t luat_mobile_get_search_plmn(void)
{
    return 0;
}
void luat_mobile_data_ip_mode(uint8_t on_off)
{

}

void luat_mobile_init_auto_apn_by_plmn(void)
{
    
}
void luat_mobile_init_auto_apn(void)
{

}
void luat_mobile_add_auto_apn_item(uint16_t mcc, uint16_t mnc, uint8_t ip_type, uint8_t protocol, char *name, uint8_t name_len, char *user, uint8_t user_len, char *password, uint8_t password_len, uint8_t task_safe)
{
    
}
int luat_mobile_find_apn_by_mcc_mnc(uint16_t mcc, uint16_t mnc, apn_info_t *apn)
{
    return 0;
}
void luat_mobile_print_apn_by_mcc_mnc(uint16_t mcc, uint16_t mnc)
{   

}
