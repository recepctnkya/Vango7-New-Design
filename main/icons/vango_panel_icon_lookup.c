#include "vango_panel_icons.h"
#include <string.h>

const lv_img_dsc_t *vango_panel_icon_by_key(const char *key)
{
    if (!key || !key[0]) {
        return NULL;
    }
    if (strcmp(key, "ac_kapat") == 0) {
        return &vango_panel_ac_kapat;
    }
    if (strcmp(key, "acil_durum") == 0) {
        return &vango_panel_acil_durum;
    }
    if (strcmp(key, "aku") == 0) {
        return &vango_panel_aku;
    }
    if (strcmp(key, "alarm") == 0) {
        return &vango_panel_alarm;
    }
    if (strcmp(key, "ambiyans_aydinlatma") == 0) {
        return &vango_panel_ambiyans_aydinlatma;
    }
    if (strcmp(key, "ayna_aydinlatma") == 0) {
        return &vango_panel_ayna_aydinlatma;
    }
    if (strcmp(key, "banyo") == 0) {
        return &vango_panel_banyo;
    }
    if (strcmp(key, "banyo_aydinlatma") == 0) {
        return &vango_panel_banyo_aydinlatma;
    }
    if (strcmp(key, "bluetooth") == 0) {
        return &vango_panel_bluetooth;
    }
    if (strcmp(key, "bulasik_makinesi") == 0) {
        return &vango_panel_bulasik_makinesi;
    }
    if (strcmp(key, "buzdolabi") == 0) {
        return &vango_panel_buzdolabi;
    }
    if (strcmp(key, "camasir_makinesi") == 0) {
        return &vango_panel_camasir_makinesi;
    }
    if (strcmp(key, "co_dedektoru") == 0) {
        return &vango_panel_co_dedektoru;
    }
    if (strcmp(key, "davlumbaaz") == 0) {
        return &vango_panel_davlumbaaz;
    }
    if (strcmp(key, "dis_aydinlatma") == 0) {
        return &vango_panel_dis_aydinlatma;
    }
    if (strcmp(key, "dolap") == 0) {
        return &vango_panel_dolap;
    }
    if (strcmp(key, "dolap_alti_aydinlatma") == 0) {
        return &vango_panel_dolap_alti_aydinlatma;
    }
    if (strcmp(key, "dolap_ici_aydinlatma") == 0) {
        return &vango_panel_dolap_ici_aydinlatma;
    }
    if (strcmp(key, "dondurucu") == 0) {
        return &vango_panel_dondurucu;
    }
    if (strcmp(key, "duman_dedektoru") == 0) {
        return &vango_panel_duman_dedektoru;
    }
    if (strcmp(key, "dus") == 0) {
        return &vango_panel_dus;
    }
    if (strcmp(key, "duvar_aydinlatma") == 0) {
        return &vango_panel_duvar_aydinlatma;
    }
    if (strcmp(key, "elektrikli_isitici") == 0) {
        return &vango_panel_elektrikli_isitici;
    }
    if (strcmp(key, "fan") == 0) {
        return &vango_panel_fan;
    }
    if (strcmp(key, "firin") == 0) {
        return &vango_panel_firin;
    }
    if (strcmp(key, "gaz_dedektoru") == 0) {
        return &vango_panel_gaz_dedektoru;
    }
    if (strcmp(key, "giris_aydinlatma") == 0) {
        return &vango_panel_giris_aydinlatma;
    }
    if (strcmp(key, "gps") == 0) {
        return &vango_panel_gps;
    }
    if (strcmp(key, "gri_su_tanki") == 0) {
        return &vango_panel_gri_su_tanki;
    }
    if (strcmp(key, "gunes_paneli") == 0) {
        return &vango_panel_gunes_paneli;
    }
    if (strcmp(key, "hareket_sensoru") == 0) {
        return &vango_panel_hareket_sensoru;
    }
    if (strcmp(key, "hava_cikisi") == 0) {
        return &vango_panel_hava_cikisi;
    }
    if (strcmp(key, "havalandirma") == 0) {
        return &vango_panel_havalandirma;
    }
    if (strcmp(key, "inverter") == 0) {
        return &vango_panel_inverter;
    }
    if (strcmp(key, "isitma") == 0) {
        return &vango_panel_isitma;
    }
    if (strcmp(key, "jenerator") == 0) {
        return &vango_panel_jenerator;
    }
    if (strcmp(key, "kamera") == 0) {
        return &vango_panel_kamera;
    }
    if (strcmp(key, "kapi") == 0) {
        return &vango_panel_kapi;
    }
    if (strcmp(key, "kapi_sensoru") == 0) {
        return &vango_panel_kapi_sensoru;
    }
    if (strcmp(key, "kilit") == 0) {
        return &vango_panel_kilit;
    }
    if (strcmp(key, "klima") == 0) {
        return &vango_panel_klima;
    }
    if (strcmp(key, "klozet") == 0) {
        return &vango_panel_klozet;
    }
    if (strcmp(key, "koltuk") == 0) {
        return &vango_panel_koltuk;
    }
    if (strcmp(key, "kompresor") == 0) {
        return &vango_panel_kompresor;
    }
    if (strcmp(key, "kurutma_makinesi") == 0) {
        return &vango_panel_kurutma_makinesi;
    }
    if (strcmp(key, "led_serit") == 0) {
        return &vango_panel_led_serit;
    }
    if (strcmp(key, "masa") == 0) {
        return &vango_panel_masa;
    }
    if (strcmp(key, "merdiven_aydinlatma") == 0) {
        return &vango_panel_merdiven_aydinlatma;
    }
    if (strcmp(key, "mikrodalga") == 0) {
        return &vango_panel_mikrodalga;
    }
    if (strcmp(key, "musluk") == 0) {
        return &vango_panel_musluk;
    }
    if (strcmp(key, "mutfak") == 0) {
        return &vango_panel_mutfak;
    }
    if (strcmp(key, "mutfak_aydinlatma") == 0) {
        return &vango_panel_mutfak_aydinlatma;
    }
    if (strcmp(key, "nem") == 0) {
        return &vango_panel_nem;
    }
    if (strcmp(key, "ocak") == 0) {
        return &vango_panel_ocak;
    }
    if (strcmp(key, "okuma_lambasi") == 0) {
        return &vango_panel_okuma_lambasi;
    }
    if (strcmp(key, "oturma_alani") == 0) {
        return &vango_panel_oturma_alani;
    }
    if (strcmp(key, "pencere") == 0) {
        return &vango_panel_pencere;
    }
    if (strcmp(key, "perde") == 0) {
        return &vango_panel_perde;
    }
    if (strcmp(key, "pis_su_tanki") == 0) {
        return &vango_panel_pis_su_tanki;
    }
    if (strcmp(key, "priz_220v") == 0) {
        return &vango_panel_priz_220v;
    }
    if (strcmp(key, "rgb_aydinlatma") == 0) {
        return &vango_panel_rgb_aydinlatma;
    }
    if (strcmp(key, "sac_kurutma") == 0) {
        return &vango_panel_sac_kurutma;
    }
    if (strcmp(key, "sarj_cihazi") == 0) {
        return &vango_panel_sarj_cihazi;
    }
    if (strcmp(key, "sarkit_aydinlatma") == 0) {
        return &vango_panel_sarkit_aydinlatma;
    }
    if (strcmp(key, "sicak_su") == 0) {
        return &vango_panel_sicak_su;
    }
    if (strcmp(key, "sicaklik") == 0) {
        return &vango_panel_sicaklik;
    }
    if (strcmp(key, "sogutma") == 0) {
        return &vango_panel_sogutma;
    }
    if (strcmp(key, "spot_aydinlatma") == 0) {
        return &vango_panel_spot_aydinlatma;
    }
    if (strcmp(key, "su_cikisi") == 0) {
        return &vango_panel_su_cikisi;
    }
    if (strcmp(key, "su_filtresi") == 0) {
        return &vango_panel_su_filtresi;
    }
    if (strcmp(key, "su_girisi") == 0) {
        return &vango_panel_su_girisi;
    }
    if (strcmp(key, "su_pompasi") == 0) {
        return &vango_panel_su_pompasi;
    }
    if (strcmp(key, "su_pompasi_elektrik") == 0) {
        return &vango_panel_su_pompasi_elektrik;
    }
    if (strcmp(key, "su_seviye") == 0) {
        return &vango_panel_su_seviye;
    }
    if (strcmp(key, "tavan_aydinlatma") == 0) {
        return &vango_panel_tavan_aydinlatma;
    }
    if (strcmp(key, "tavan_havalandirma_aydinlatma") == 0) {
        return &vango_panel_tavan_havalandirma_aydinlatma;
    }
    if (strcmp(key, "temiz_su_tanki") == 0) {
        return &vango_panel_temiz_su_tanki;
    }
    if (strcmp(key, "tente_alti_aydinlatma") == 0) {
        return &vango_panel_tente_alti_aydinlatma;
    }
    if (strcmp(key, "tv") == 0) {
        return &vango_panel_tv;
    }
    if (strcmp(key, "usb_priz") == 0) {
        return &vango_panel_usb_priz;
    }
    if (strcmp(key, "uzaktan_kumanda") == 0) {
        return &vango_panel_uzaktan_kumanda;
    }
    if (strcmp(key, "wifi") == 0) {
        return &vango_panel_wifi;
    }
    if (strcmp(key, "yangin_alarmi") == 0) {
        return &vango_panel_yangin_alarmi;
    }
    if (strcmp(key, "yatak_basi") == 0) {
        return &vango_panel_yatak_basi;
    }
    if (strcmp(key, "yatak_odasi") == 0) {
        return &vango_panel_yatak_odasi;
    }
    if (strcmp(key, "zemin_aydinlatma") == 0) {
        return &vango_panel_zemin_aydinlatma;
    }
    return NULL;
}

const lv_img_dsc_t *vango_panel_icon_sm_by_key(const char *key)
{
    if (!key || !key[0]) {
        return NULL;
    }
    if (strcmp(key, "ac_kapat") == 0) {
        return &vango_panel_ac_kapat_sm;
    }
    if (strcmp(key, "acil_durum") == 0) {
        return &vango_panel_acil_durum_sm;
    }
    if (strcmp(key, "aku") == 0) {
        return &vango_panel_aku_sm;
    }
    if (strcmp(key, "alarm") == 0) {
        return &vango_panel_alarm_sm;
    }
    if (strcmp(key, "ambiyans_aydinlatma") == 0) {
        return &vango_panel_ambiyans_aydinlatma_sm;
    }
    if (strcmp(key, "ayna_aydinlatma") == 0) {
        return &vango_panel_ayna_aydinlatma_sm;
    }
    if (strcmp(key, "banyo") == 0) {
        return &vango_panel_banyo_sm;
    }
    if (strcmp(key, "banyo_aydinlatma") == 0) {
        return &vango_panel_banyo_aydinlatma_sm;
    }
    if (strcmp(key, "bluetooth") == 0) {
        return &vango_panel_bluetooth_sm;
    }
    if (strcmp(key, "bulasik_makinesi") == 0) {
        return &vango_panel_bulasik_makinesi_sm;
    }
    if (strcmp(key, "buzdolabi") == 0) {
        return &vango_panel_buzdolabi_sm;
    }
    if (strcmp(key, "camasir_makinesi") == 0) {
        return &vango_panel_camasir_makinesi_sm;
    }
    if (strcmp(key, "co_dedektoru") == 0) {
        return &vango_panel_co_dedektoru_sm;
    }
    if (strcmp(key, "davlumbaaz") == 0) {
        return &vango_panel_davlumbaaz_sm;
    }
    if (strcmp(key, "dis_aydinlatma") == 0) {
        return &vango_panel_dis_aydinlatma_sm;
    }
    if (strcmp(key, "dolap") == 0) {
        return &vango_panel_dolap_sm;
    }
    if (strcmp(key, "dolap_alti_aydinlatma") == 0) {
        return &vango_panel_dolap_alti_aydinlatma_sm;
    }
    if (strcmp(key, "dolap_ici_aydinlatma") == 0) {
        return &vango_panel_dolap_ici_aydinlatma_sm;
    }
    if (strcmp(key, "dondurucu") == 0) {
        return &vango_panel_dondurucu_sm;
    }
    if (strcmp(key, "duman_dedektoru") == 0) {
        return &vango_panel_duman_dedektoru_sm;
    }
    if (strcmp(key, "dus") == 0) {
        return &vango_panel_dus_sm;
    }
    if (strcmp(key, "duvar_aydinlatma") == 0) {
        return &vango_panel_duvar_aydinlatma_sm;
    }
    if (strcmp(key, "elektrikli_isitici") == 0) {
        return &vango_panel_elektrikli_isitici_sm;
    }
    if (strcmp(key, "fan") == 0) {
        return &vango_panel_fan_sm;
    }
    if (strcmp(key, "firin") == 0) {
        return &vango_panel_firin_sm;
    }
    if (strcmp(key, "gaz_dedektoru") == 0) {
        return &vango_panel_gaz_dedektoru_sm;
    }
    if (strcmp(key, "giris_aydinlatma") == 0) {
        return &vango_panel_giris_aydinlatma_sm;
    }
    if (strcmp(key, "gps") == 0) {
        return &vango_panel_gps_sm;
    }
    if (strcmp(key, "gri_su_tanki") == 0) {
        return &vango_panel_gri_su_tanki_sm;
    }
    if (strcmp(key, "gunes_paneli") == 0) {
        return &vango_panel_gunes_paneli_sm;
    }
    if (strcmp(key, "hareket_sensoru") == 0) {
        return &vango_panel_hareket_sensoru_sm;
    }
    if (strcmp(key, "hava_cikisi") == 0) {
        return &vango_panel_hava_cikisi_sm;
    }
    if (strcmp(key, "havalandirma") == 0) {
        return &vango_panel_havalandirma_sm;
    }
    if (strcmp(key, "inverter") == 0) {
        return &vango_panel_inverter_sm;
    }
    if (strcmp(key, "isitma") == 0) {
        return &vango_panel_isitma_sm;
    }
    if (strcmp(key, "jenerator") == 0) {
        return &vango_panel_jenerator_sm;
    }
    if (strcmp(key, "kamera") == 0) {
        return &vango_panel_kamera_sm;
    }
    if (strcmp(key, "kapi") == 0) {
        return &vango_panel_kapi_sm;
    }
    if (strcmp(key, "kapi_sensoru") == 0) {
        return &vango_panel_kapi_sensoru_sm;
    }
    if (strcmp(key, "kilit") == 0) {
        return &vango_panel_kilit_sm;
    }
    if (strcmp(key, "klima") == 0) {
        return &vango_panel_klima_sm;
    }
    if (strcmp(key, "klozet") == 0) {
        return &vango_panel_klozet_sm;
    }
    if (strcmp(key, "koltuk") == 0) {
        return &vango_panel_koltuk_sm;
    }
    if (strcmp(key, "kompresor") == 0) {
        return &vango_panel_kompresor_sm;
    }
    if (strcmp(key, "kurutma_makinesi") == 0) {
        return &vango_panel_kurutma_makinesi_sm;
    }
    if (strcmp(key, "led_serit") == 0) {
        return &vango_panel_led_serit_sm;
    }
    if (strcmp(key, "masa") == 0) {
        return &vango_panel_masa_sm;
    }
    if (strcmp(key, "merdiven_aydinlatma") == 0) {
        return &vango_panel_merdiven_aydinlatma_sm;
    }
    if (strcmp(key, "mikrodalga") == 0) {
        return &vango_panel_mikrodalga_sm;
    }
    if (strcmp(key, "musluk") == 0) {
        return &vango_panel_musluk_sm;
    }
    if (strcmp(key, "mutfak") == 0) {
        return &vango_panel_mutfak_sm;
    }
    if (strcmp(key, "mutfak_aydinlatma") == 0) {
        return &vango_panel_mutfak_aydinlatma_sm;
    }
    if (strcmp(key, "nem") == 0) {
        return &vango_panel_nem_sm;
    }
    if (strcmp(key, "ocak") == 0) {
        return &vango_panel_ocak_sm;
    }
    if (strcmp(key, "okuma_lambasi") == 0) {
        return &vango_panel_okuma_lambasi_sm;
    }
    if (strcmp(key, "oturma_alani") == 0) {
        return &vango_panel_oturma_alani_sm;
    }
    if (strcmp(key, "pencere") == 0) {
        return &vango_panel_pencere_sm;
    }
    if (strcmp(key, "perde") == 0) {
        return &vango_panel_perde_sm;
    }
    if (strcmp(key, "pis_su_tanki") == 0) {
        return &vango_panel_pis_su_tanki_sm;
    }
    if (strcmp(key, "priz_220v") == 0) {
        return &vango_panel_priz_220v_sm;
    }
    if (strcmp(key, "rgb_aydinlatma") == 0) {
        return &vango_panel_rgb_aydinlatma_sm;
    }
    if (strcmp(key, "sac_kurutma") == 0) {
        return &vango_panel_sac_kurutma_sm;
    }
    if (strcmp(key, "sarj_cihazi") == 0) {
        return &vango_panel_sarj_cihazi_sm;
    }
    if (strcmp(key, "sarkit_aydinlatma") == 0) {
        return &vango_panel_sarkit_aydinlatma_sm;
    }
    if (strcmp(key, "sicak_su") == 0) {
        return &vango_panel_sicak_su_sm;
    }
    if (strcmp(key, "sicaklik") == 0) {
        return &vango_panel_sicaklik_sm;
    }
    if (strcmp(key, "sogutma") == 0) {
        return &vango_panel_sogutma_sm;
    }
    if (strcmp(key, "spot_aydinlatma") == 0) {
        return &vango_panel_spot_aydinlatma_sm;
    }
    if (strcmp(key, "su_cikisi") == 0) {
        return &vango_panel_su_cikisi_sm;
    }
    if (strcmp(key, "su_filtresi") == 0) {
        return &vango_panel_su_filtresi_sm;
    }
    if (strcmp(key, "su_girisi") == 0) {
        return &vango_panel_su_girisi_sm;
    }
    if (strcmp(key, "su_pompasi") == 0) {
        return &vango_panel_su_pompasi_sm;
    }
    if (strcmp(key, "su_pompasi_elektrik") == 0) {
        return &vango_panel_su_pompasi_elektrik_sm;
    }
    if (strcmp(key, "su_seviye") == 0) {
        return &vango_panel_su_seviye_sm;
    }
    if (strcmp(key, "tavan_aydinlatma") == 0) {
        return &vango_panel_tavan_aydinlatma_sm;
    }
    if (strcmp(key, "tavan_havalandirma_aydinlatma") == 0) {
        return &vango_panel_tavan_havalandirma_aydinlatma_sm;
    }
    if (strcmp(key, "temiz_su_tanki") == 0) {
        return &vango_panel_temiz_su_tanki_sm;
    }
    if (strcmp(key, "tente_alti_aydinlatma") == 0) {
        return &vango_panel_tente_alti_aydinlatma_sm;
    }
    if (strcmp(key, "tv") == 0) {
        return &vango_panel_tv_sm;
    }
    if (strcmp(key, "usb_priz") == 0) {
        return &vango_panel_usb_priz_sm;
    }
    if (strcmp(key, "uzaktan_kumanda") == 0) {
        return &vango_panel_uzaktan_kumanda_sm;
    }
    if (strcmp(key, "wifi") == 0) {
        return &vango_panel_wifi_sm;
    }
    if (strcmp(key, "yangin_alarmi") == 0) {
        return &vango_panel_yangin_alarmi_sm;
    }
    if (strcmp(key, "yatak_basi") == 0) {
        return &vango_panel_yatak_basi_sm;
    }
    if (strcmp(key, "yatak_odasi") == 0) {
        return &vango_panel_yatak_odasi_sm;
    }
    if (strcmp(key, "zemin_aydinlatma") == 0) {
        return &vango_panel_zemin_aydinlatma_sm;
    }
    return NULL;
}
