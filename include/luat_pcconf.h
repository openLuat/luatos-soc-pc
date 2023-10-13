#ifndef LUAT_PCCONF_H
#define LUAT_PCCONF_H

typedef struct luat_pcconf
{
    // MCU
    char   mcu_unique_id[32];
    size_t mcu_unique_id_len;
    size_t mcu_mhz;

    // mobile
    char mobile_imei[16];
    char mobile_muid[20];
    char mobile_imsi[20];
    char mobile_iccid[20];
    char mobile_iccid2[20];
    int  mobile_csq;
}luat_pcconf_t;

void luat_pcconf_init(void);

void luat_pcconf_save(void);

#endif
