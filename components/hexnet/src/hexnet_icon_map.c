#include "hexnet_icon_map.h"

#include <string.h>

typedef struct {
    const char *key;
    uint8_t preset;
} hexnet_icon_map_entry_t;

/* VanGO PanelConfigMapper.platformToPreset ile hizali (en sik kullanilanlar). */
static const hexnet_icon_map_entry_t s_map[] = {
    {"tavan_aydinlatma", 1}, {"spot_aydinlatma", 17}, {"led_serit", 1},
    {"rgb_aydinlatma", 1}, {"sarkit_aydinlatma", 1}, {"duvar_aydinlatma", 1},
    {"okuma_lambasi", 18}, {"dolap_alti_aydinlatma", 1}, {"dolap_ici_aydinlatma", 1},
    {"ayna_aydinlatma", 1}, {"dis_aydinlatma", 14}, {"giris_aydinlatma", 1},
    {"merdiven_aydinlatma", 1}, {"zemin_aydinlatma", 1}, {"ambiyans_aydinlatma", 1},
    {"yatak_basi", 4}, {"mutfak_aydinlatma", 3}, {"tavan_havalandirma_aydinlatma", 1},
    {"banyo_aydinlatma", 1}, {"tente_alti_aydinlatma", 14},
    {"priz_220v", 11}, {"usb_priz", 8}, {"inverter", 8}, {"jenerator", 15},
    {"kompresor", 15}, {"elektrikli_isitici", 16}, {"fan", 1}, {"tv", 13},
    {"mikrodalga", 12}, {"sac_kurutma", 15},
    {"temiz_su_tanki", 10}, {"gri_su_tanki", 10}, {"pis_su_tanki", 10},
    {"su_pompasi", 10}, {"su_pompasi_elektrik", 10}, {"su_girisi", 10},
    {"su_cikisi", 10}, {"su_filtresi", 10}, {"musluk", 3}, {"dus", 10},
    {"sicak_su", 10}, {"su_seviye", 10},
    {"yatak_odasi", 4}, {"oturma_alani", 5}, {"mutfak", 3}, {"banyo", 2},
    {"klozet", 2}, {"masa", 5}, {"koltuk", 5}, {"dolap", 5}, {"kapi", 5},
    {"pencere", 5}, {"perde", 5},
    {"klima", 7}, {"isitma", 16}, {"havalandirma", 7}, {"hava_cikisi", 7},
    {"sicaklik", 7}, {"sogutma", 7}, {"nem", 10},
    {"ac_kapat", 1}, {"kilit", 5}, {"alarm", 5}, {"kamera", 13},
    {"buzdolabi", 9}, {"dondurucu", 9}, {"firin", 12}, {"ocak", 12},
    {"lightbulb", 1}, {"plug", 11}, {"toilet", 2}, {"fridge", 9},
    {"heater", 16}, {"ac", 7},
};

uint8_t hexnet_icon_key_to_preset(const char *key)
{
    if (!key || !key[0]) {
        return 1;
    }
    for (size_t i = 0; i < sizeof(s_map) / sizeof(s_map[0]); ++i) {
        if (strcmp(key, s_map[i].key) == 0) {
            return s_map[i].preset;
        }
    }
    return 1;
}
