#pragma once

/** Geliştirme hattı (klasör adı ile uyumlu). */
#define HEXNET_DISP_DEV_LINE "v0-50"

/**
 * Makro sürüm (çizgi). VERSION.json makro ile uyumlu tutun.
 * CMake project(VERSION "0.50.0") → esp_app_desc.version
 */
#define HEXNET_DISP_RELEASE "v0.50"

/** Mikro sürüm — her anlamlı derleme/değişiklik (takip). */
#define HEXNET_DISP_MICRO "v0.50-0009"

/** Telemetri / UI — çalışan partition veya RELEASE. */
const char *hexnet_firmware_version_string(void);
const char *hexnet_firmware_micro_string(void);
