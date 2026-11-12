file(GLOB VANGO_PANEL_ICON_SRCS "${CMAKE_CURRENT_LIST_DIR}/icons/panel/*.c")
list(APPEND VANGO_PANEL_ICON_SRCS "${CMAKE_CURRENT_LIST_DIR}/icons/vango_panel_icon_lookup.c")

idf_component_register(

    SRCS "waveshare_rgb_lcd_port.c" "main.c" "lvgl_port.c"
        "../components/ui_files/ui.c"
        "display_manager.c"
        "hexnet_version.c"
        "data_manager.c"
        "settings_page.c"
        "ui_vango_dash.c"
        "ui_vango_settings.c"
        "ui_vango_rgb.c"
        "ui_vango_panel_catalog.c"
        "vango_shot.c"
        ${VANGO_PANEL_ICON_SRCS}
        "fonts/vango_font_10.c"
        "fonts/vango_font_12.c"
        "fonts/vango_font_14.c"
        "fonts/vango_font_16.c"
        "fonts/vango_font_18.c"
        "fonts/vango_font_20.c"
        "fonts/vango_font_26.c"
        "icons/vango_icon_light.c"
        "icons/vango_icon_power.c"
        "icons/vango_icon_water.c"
        "icons/vango_icon_thermo.c"
        "icons/vango_icon_sensor.c"
        "icons/vango_icon_settings.c"
        "icons/vango_icon_battery.c"
        "icons/vango_icon_logo.c"
        "icons/vango_wallpaper_logo.c"
        "icons/vango_icon_io_chip.c"
        "icons/vango_icon_drop_clean.c"
        "icons/vango_icon_drop_waste.c"
        "icons/vango_icon_drop_grey.c"
        "icons/vango_icon_gear.c"
        "icons/vango_icon_gear_metal.c"
        "icons/vango_nav_motor.c"
        "icons/vango_nav_home.c"
        "icons/vango_nav_light.c"
        "icons/vango_nav_rgb.c"
        "icons/vango_nav_power.c"
        "icons/vango_nav_water.c"
        "icons/vango_nav_thermo.c"
        "icons/vango_nav_sensor.c"
        "icons/vango_nav_settings.c"
        "icons/vango_glow_soft.c"
    "../components/ui_files/ui_scrWallpaper.c"
    "../components/ui_files/ui_scrTheme.c"
    "../components/ui_files/ui_scrSettings.c"
    "../components/ui_files/ui_scrRGB.c"
    "../components/ui_files/ui_scrPanelSettings.c"
    "../components/ui_files/ui_scrPages.c"
    "../components/ui_files/ui_scrMain.c"
    "../components/ui_files/ui_scrInit.c"
    "../components/ui_files/ui_scrDimsandSensorsandWaters.c"
    "../components/ui_files/ui_scrCalibration.c"
    "../components/ui_files/ui_img_weatherforecast2_png.c"
    "../components/ui_files/ui_img_water_png.c"
    "../components/ui_files/ui_img_waterlevel_png.c"
    # legacy SquareLine wallpaper logo replaced by icons/vango_wallpaper_logo.c
    "../components/ui_files/ui_img_usb_png.c"
    "../components/ui_files/ui_img_tv_png.c"
    "../components/ui_files/ui_img_toilet_png.c"
    "../components/ui_files/ui_img_theme_png.c"
    "../components/ui_files/ui_img_tempicon_png.c"
    "../components/ui_files/ui_img_snotconnected_png.c"
    "../components/ui_files/ui_img_settingsicon_png.c"
    "../components/ui_files/ui_img_sensors_png.c"
    "../components/ui_files/ui_img_sconnected_png.c"
    "../components/ui_files/ui_img_rgbcontrol_png.c"
    "../components/ui_files/ui_img_refrigerator_png.c"
    "../components/ui_files/ui_img_readinglamp_png.c"
    "../components/ui_files/ui_img_oven_png.c"
    "../components/ui_files/ui_img_outlet_png.c"
    "../components/ui_files/ui_img_outcontrols_png.c"
    "../components/ui_files/ui_img_logo3_png.c"
    "../components/ui_files/ui_img_lamp_png.c"
    "../components/ui_files/ui_img_heater_png.c"
    "../components/ui_files/ui_img_connectioncontrol_png.c"
    "../components/ui_files/ui_img_caravan_logo_png.c"
    "../components/ui_files/ui_img_button_up_png.c"
    "../components/ui_files/ui_img_button_down_png.c"
    "../components/ui_files/ui_img_btnsettings2_png.c"
    "../components/ui_files/ui_img_btnotconnected_png.c"
    "../components/ui_files/ui_img_btconnected_png.c"
    "../components/ui_files/ui_img_arcdesign_png.c"
    "../components/ui_files/ui_img_arcdesign2_png.c"
    "../components/ui_files/ui_img_ac_png.c"
    "../components/ui_files/ui_img_1542361832.c"
    "../components/ui_files/ui_img_1096609443.c"
    "../components/ui_files/ui_helpers.c"
    "../components/ui_files/ui_events.c"
    "../components/ui_files/ui_comp_hook.c"
    "../components/ui_files/ui.c"

    
        INCLUDE_DIRS "."
        "fonts"
        "icons"
        "../components/hexnet/inc"
        "../components/ui_files"
        "../components/freemodbus/freemodbus/common/include"

)
# Kaynak degistiyse mikro++ (idf.py build / flash her seferinde)
add_custom_target(hexnet_bump_if_changed
    COMMAND "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/Bump-IfChanged.ps1"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Hexnet: bump mikro if sources changed"
    VERBATIM
)
add_dependencies(${COMPONENT_LIB} hexnet_bump_if_changed)

# Set compile options for lvgl component after component registration
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)

target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
# LVGL UI heap -> SPIRAM (keeps internal DRAM for BLE / IWDT)
target_compile_options(${lvgl_lib} PRIVATE -include "${CMAKE_CURRENT_LIST_DIR}/lv_mem_spiram_force.h")
target_link_libraries(${lvgl_lib} PRIVATE idf::heap)