#include "settings_page.h"



#include "ui_files/ui.h"


#include <stdio.h>
#include "string.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"




lv_obj_t * ui_lblPanelSettings;
void ui_event_swO1(lv_event_t * e);
lv_obj_t * ui_swO1;
lv_obj_t * ui_cbxO1;
lv_obj_t * ui_cbxO2;
void ui_event_swO2(lv_event_t * e);
lv_obj_t * ui_swO2;
void ui_event_swO3(lv_event_t * e);
lv_obj_t * ui_swO3;
lv_obj_t * ui_cbxO3;
void ui_event_swO4(lv_event_t * e);
lv_obj_t * ui_swO4;
lv_obj_t * ui_cbxO4;
void ui_event_swO5(lv_event_t * e);
lv_obj_t * ui_swO5;
lv_obj_t * ui_cbxO5;
void ui_event_swO6(lv_event_t * e);
lv_obj_t * ui_swO6;
lv_obj_t * ui_cbxO6;
void ui_event_swO7(lv_event_t * e);
lv_obj_t * ui_swO7;
lv_obj_t * ui_cbxO7;
void ui_event_swO8(lv_event_t * e);
lv_obj_t * ui_swO8;
lv_obj_t * ui_cbxO8;
void ui_event_swO9(lv_event_t * e);
lv_obj_t * ui_swO9;
lv_obj_t * ui_cbxO9;
lv_obj_t * ui_cbxO10;
void ui_event_swO10(lv_event_t * e);
lv_obj_t * ui_swO10;
void ui_event_swO11(lv_event_t * e);
lv_obj_t * ui_swO11;
lv_obj_t * ui_cbxO11;
void ui_event_swO12(lv_event_t * e);
lv_obj_t * ui_swO12;
lv_obj_t * ui_cbxO12;
void ui_event_swO14(lv_event_t * e);
lv_obj_t * ui_swO14;
lv_obj_t * ui_cbxO13;
lv_obj_t * ui_cbxO14;
void ui_event_swO15(lv_event_t * e);
lv_obj_t * ui_swO15;
lv_obj_t * ui_cbxO15;
void ui_event_swO16(lv_event_t * e);
lv_obj_t * ui_swO16;
lv_obj_t * ui_cbxO16;
void ui_event_swO13(lv_event_t * e);
lv_obj_t * ui_swO13;
lv_obj_t * ui_Checkbox1;
lv_obj_t * ui_Checkbox2;
lv_obj_t * ui_Checkbox3;
lv_obj_t * ui_Checkbox4;
lv_obj_t * ui_Checkbox5;


lv_obj_t * ui_cbRSelect = NULL;
lv_obj_t * ui_btnRSelect = NULL;
lv_obj_t * ui_btnGSelect = NULL;
lv_obj_t * ui_btnBSelect = NULL;
lv_obj_t * ui_cbGSelect = NULL;
lv_obj_t * ui_cbBSelect = NULL;
lv_obj_t * ui_lbRSelect = NULL;
lv_obj_t * ui_lbGSelect = NULL;
lv_obj_t * ui_lbBSelect = NULL;

lv_obj_t * ui_lblSensors;
lv_obj_t * ui_lblDimmableOutputs;
void ui_event_swDim1(lv_event_t * e);
lv_obj_t * ui_swDim1;
lv_obj_t * ui_cbxDim1;
lv_obj_t * ui_cbxDim2;
lv_obj_t * ui_cbxDim3;
lv_obj_t * ui_cbxDim4;
void ui_event_swDim2(lv_event_t * e);
lv_obj_t * ui_swDim2;
void ui_event_swDim3(lv_event_t * e);
lv_obj_t * ui_swDim3;
void ui_event_swDim4(lv_event_t * e);
lv_obj_t * ui_swDim4;
// CUSTOM VARIABLES



void ui_event_swO1(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO1, 255);
    }
}

void ui_event_swO2(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO2, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO3(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO3, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO4(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO4, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO5(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO5, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO6(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO6, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO5, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO7(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO7, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO6, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO8(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO8, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO7, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO9(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO9, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO8, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO10(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO10, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO9, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO11(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO11, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO10, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO12(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO12, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO11, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO13(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO13, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO12, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO14(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO14, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO13, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO15(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO16, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO16, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO15, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO14, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swO16(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO16, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxO16, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_opacity_set(ui_cbxO16, 255);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swO15, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}





void ui_event_swDim1(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
}

void ui_event_swDim2(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim1, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swDim3(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim2, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
}

void ui_event_swDim4(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_cbxDim4, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_REMOVE);
    }

    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED)) {
        _ui_flag_modify(ui_swDim3, LV_OBJ_FLAG_CLICKABLE, _UI_MODIFY_FLAG_ADD);
    }

}


void ui_event_ButtonSelectR(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_CLICKED) {
        sendCanFrameRGB_R(e);
    }
}

void ui_event_ButtonSelectG(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_CLICKED) {
        sendCanFrameRGB_G(e);
    }
}

void ui_event_ButtonSelectB(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_CLICKED) {
        sendCanFrameRGB_B(e);
    }
}


void ui_scrPanelSettings_IO_Dim_init(void)
{

    ui_lblPanelSettings = lv_label_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_lblPanelSettings, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_lblPanelSettings, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_lblPanelSettings, -16);
    lv_obj_set_y(ui_lblPanelSettings, -216);
    lv_obj_set_align(ui_lblPanelSettings, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lblPanelSettings, "PANEL SETTINGS");
    lv_obj_set_style_text_font(ui_lblPanelSettings, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_swO1 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO1, 73);
    lv_obj_set_height(ui_swO1, 44);
    lv_obj_set_x(ui_swO1, -330);
    lv_obj_set_y(ui_swO1, -165);
    lv_obj_set_align(ui_swO1, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_swO1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO1, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO1, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO1, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO1, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO1 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO1,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO1, 140);
    lv_obj_set_height(ui_cbxO1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO1, -220);
    lv_obj_set_y(ui_cbxO1, -165);
    lv_obj_set_align(ui_cbxO1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO1, LV_OBJ_FLAG_CLICKABLE);      /// Flags

    lv_obj_set_style_opa(lv_dropdown_get_list(ui_cbxO1), 255,  LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_cbxO2 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO2,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO2, 140);
    lv_obj_set_height(ui_cbxO2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO2, -219);
    lv_obj_set_y(ui_cbxO2, -117);
    lv_obj_set_align(ui_cbxO2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO2, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO2 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO2, 73);
    lv_obj_set_height(ui_swO2, 44);
    lv_obj_set_x(ui_swO2, -332);
    lv_obj_set_y(ui_swO2, -117);
    lv_obj_set_align(ui_swO2, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO2, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO2, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO2, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO2, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO2, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_swO3 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO3, 73);
    lv_obj_set_height(ui_swO3, 44);
    lv_obj_set_x(ui_swO3, -331);
    lv_obj_set_y(ui_swO3, -70);
    lv_obj_set_align(ui_swO3, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO3, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO3, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO3, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO3, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO3, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO3, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO3 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO3,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO3, 140);
    lv_obj_set_height(ui_cbxO3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO3, -222);
    lv_obj_set_y(ui_cbxO3, -70);
    lv_obj_set_align(ui_cbxO3, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO3, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO4 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO4, 73);
    lv_obj_set_height(ui_swO4, 44);
    lv_obj_set_x(ui_swO4, -332);
    lv_obj_set_y(ui_swO4, -22);
    lv_obj_set_align(ui_swO4, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO4, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO4, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO4, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO4, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO4, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO4, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO4 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO4,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO4, 140);
    lv_obj_set_height(ui_cbxO4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO4, -220);
    lv_obj_set_y(ui_cbxO4, -21);
    lv_obj_set_align(ui_cbxO4, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO4, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO4, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO5 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO5, 73);
    lv_obj_set_height(ui_swO5, 44);
    lv_obj_set_x(ui_swO5, -334);
    lv_obj_set_y(ui_swO5, 25);
    lv_obj_set_align(ui_swO5, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO5, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO5, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO5, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO5, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO5, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO5, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO5 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO5,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO5, 140);
    lv_obj_set_height(ui_cbxO5, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO5, -222);
    lv_obj_set_y(ui_cbxO5, 24);
    lv_obj_set_align(ui_cbxO5, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO5, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO5, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO6 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO6, 73);
    lv_obj_set_height(ui_swO6, 44);
    lv_obj_set_x(ui_swO6, -334);
    lv_obj_set_y(ui_swO6, 71);
    lv_obj_set_align(ui_swO6, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO6, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO6, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO6, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO6, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO6, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO6, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO6 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO6,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO6, 140);
    lv_obj_set_height(ui_cbxO6, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO6, -221);
    lv_obj_set_y(ui_cbxO6, 73);
    lv_obj_set_align(ui_cbxO6, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO6, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO6, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO7 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO7, 73);
    lv_obj_set_height(ui_swO7, 44);
    lv_obj_set_x(ui_swO7, -334);
    lv_obj_set_y(ui_swO7, 120);
    lv_obj_set_align(ui_swO7, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO7, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO7, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO7, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO7, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO7, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO7, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO7 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO7,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO7, 140);
    lv_obj_set_height(ui_cbxO7, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO7, -220);
    lv_obj_set_y(ui_cbxO7, 122);
    lv_obj_set_align(ui_cbxO7, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO7, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO7, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO8 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO8, 73);
    lv_obj_set_height(ui_swO8, 44);
    lv_obj_set_x(ui_swO8, -334);
    lv_obj_set_y(ui_swO8, 166);
    lv_obj_set_align(ui_swO8, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO8, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO8, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO8, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO8, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO8, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO8, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO8 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO8,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO8, 140);
    lv_obj_set_height(ui_cbxO8, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO8, -219);
    lv_obj_set_y(ui_cbxO8, 166);
    lv_obj_set_align(ui_cbxO8, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO8, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO8, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO9 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO9, 73);
    lv_obj_set_height(ui_swO9, 44);
    lv_obj_set_x(ui_swO9, -75);
    lv_obj_set_y(ui_swO9, -162);
    lv_obj_set_align(ui_swO9, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO9, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO9, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO9, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO9, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO9, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO9, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO9 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO9,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO9, 140);
    lv_obj_set_height(ui_cbxO9, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO9, 38);
    lv_obj_set_y(ui_cbxO9, -162);
    lv_obj_set_align(ui_cbxO9, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO9, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO9, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_cbxO10 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO10,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO10, 140);
    lv_obj_set_height(ui_cbxO10, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO10, 39);
    lv_obj_set_y(ui_cbxO10, -116);
    lv_obj_set_align(ui_cbxO10, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO10, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO10, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO10 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO10, 73);
    lv_obj_set_height(ui_swO10, 44);
    lv_obj_set_x(ui_swO10, -76);
    lv_obj_set_y(ui_swO10, -114);
    lv_obj_set_align(ui_swO10, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO10, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO10, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO10, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO10, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO10, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO10, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_swO11 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO11, 73);
    lv_obj_set_height(ui_swO11, 44);
    lv_obj_set_x(ui_swO11, -77);
    lv_obj_set_y(ui_swO11, -66);
    lv_obj_set_align(ui_swO11, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO11, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO11, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO11, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO11, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO11, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO11, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO11 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO11,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO11, 140);
    lv_obj_set_height(ui_cbxO11, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO11, 37);
    lv_obj_set_y(ui_cbxO11, -69);
    lv_obj_set_align(ui_cbxO11, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO11, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO11, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO12 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO12, 73);
    lv_obj_set_height(ui_swO12, 44);
    lv_obj_set_x(ui_swO12, -76);
    lv_obj_set_y(ui_swO12, -19);
    lv_obj_set_align(ui_swO12, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO12, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO12, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO12, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO12, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO12, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO12, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO12 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO12,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO12, 140);
    lv_obj_set_height(ui_cbxO12, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO12, 37);
    lv_obj_set_y(ui_cbxO12, -22);
    lv_obj_set_align(ui_cbxO12, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO12, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO12, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO14 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO14, 73);
    lv_obj_set_height(ui_swO14, 44);
    lv_obj_set_x(ui_swO14, -75);
    lv_obj_set_y(ui_swO14, 76);
    lv_obj_set_align(ui_swO14, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO14, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO14, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO14, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO14, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO14, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO14, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO13 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO13,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO13, 140);
    lv_obj_set_height(ui_cbxO13, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO13, 36);
    lv_obj_set_y(ui_cbxO13, 28);
    lv_obj_set_align(ui_cbxO13, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO13, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO13, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_cbxO14 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO14,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO14, 140);
    lv_obj_set_height(ui_cbxO14, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO14, 37);
    lv_obj_set_y(ui_cbxO14, 76);
    lv_obj_set_align(ui_cbxO14, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO14, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO14, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO15 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO15, 73);
    lv_obj_set_height(ui_swO15, 44);
    lv_obj_set_x(ui_swO15, -74);
    lv_obj_set_y(ui_swO15, 124);
    lv_obj_set_align(ui_swO15, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO15, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO15, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO15, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO15, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO15, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO15, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO15 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO15,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO15, 140);
    lv_obj_set_height(ui_cbxO15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO15, 38);
    lv_obj_set_y(ui_cbxO15, 122);
    lv_obj_set_align(ui_cbxO15, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO15, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO15, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO16 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO16, 73);
    lv_obj_set_height(ui_swO16, 44);
    lv_obj_set_x(ui_swO16, -74);
    lv_obj_set_y(ui_swO16, 171);
    lv_obj_set_align(ui_swO16, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_swO16, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO16, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO16, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO16, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO16, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxO16 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxO16,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_obj_set_width(ui_cbxO16, 140);
    lv_obj_set_height(ui_cbxO16, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxO16, 37);
    lv_obj_set_y(ui_cbxO16, 169);
    lv_obj_set_align(ui_cbxO16, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxO16, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxO16, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swO13 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swO13, 73);
    lv_obj_set_height(ui_swO13, 44);
    lv_obj_set_x(ui_swO13, -75);
    lv_obj_set_y(ui_swO13, 26);
    lv_obj_set_align(ui_swO13, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swO13, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swO13, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO13, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swO13, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swO13, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swO13, 10, LV_PART_KNOB | LV_STATE_DEFAULT);




    ui_cbRSelect = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbRSelect, "R\nG\nB");
    lv_obj_set_width(ui_cbRSelect, 69);
    lv_obj_set_height(ui_cbRSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbRSelect, 310);
    lv_obj_set_y(ui_cbRSelect, -205);
    lv_obj_set_align(ui_cbRSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbRSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags


    ui_cbGSelect = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbGSelect, "R\nG\nB");
    lv_obj_set_width(ui_cbGSelect, 69);
    lv_obj_set_height(ui_cbGSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbGSelect, 310);
    lv_obj_set_y(ui_cbGSelect, -148);
    lv_obj_set_align(ui_cbGSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbGSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags

    ui_cbBSelect = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbBSelect, "R\nG\nB");
    lv_obj_set_width(ui_cbBSelect, 69);
    lv_obj_set_height(ui_cbBSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbBSelect, 310);
    lv_obj_set_y(ui_cbBSelect, -90);
    lv_obj_set_align(ui_cbBSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbBSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags

     ui_btnRSelect = lv_btn_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_btnRSelect, 85);
    lv_obj_set_height(ui_btnRSelect, 50);
    lv_obj_set_x(ui_btnRSelect, 213);
    lv_obj_set_y(ui_btnRSelect, -208);
    lv_obj_set_align(ui_btnRSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_btnRSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_btnRSelect, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_lbRSelect = lv_label_create(ui_btnRSelect);
    lv_obj_set_width(ui_lbRSelect, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_lbRSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_lbRSelect, -2);
    lv_obj_set_y(ui_lbRSelect, -1);
    lv_obj_set_align(ui_lbRSelect, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lbRSelect, "SET R");

    ui_btnGSelect = lv_btn_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_btnGSelect, 85);
    lv_obj_set_height(ui_btnGSelect, 50);
    lv_obj_set_x(ui_btnGSelect, 213);
    lv_obj_set_y(ui_btnGSelect, -148);
    lv_obj_set_align(ui_btnGSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_btnGSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_btnGSelect, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_lbGSelect = lv_label_create(ui_btnGSelect);
    lv_obj_set_width(ui_lbGSelect, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_lbGSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_lbGSelect, -2);
    lv_obj_set_y(ui_lbGSelect, -1);
    lv_obj_set_align(ui_lbGSelect, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lbGSelect, "SET G");

    ui_btnBSelect = lv_btn_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_btnBSelect, 85);
    lv_obj_set_height(ui_btnBSelect, 50);
    lv_obj_set_x(ui_btnBSelect, 213);
    lv_obj_set_y(ui_btnBSelect, -90);
    lv_obj_set_align(ui_btnBSelect, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_btnBSelect, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_btnBSelect, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_lbBSelect = lv_label_create(ui_btnBSelect);
    lv_obj_set_width(ui_lbBSelect, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_lbBSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_lbBSelect, -2);
    lv_obj_set_y(ui_lbBSelect, -1);
    lv_obj_set_align(ui_lbBSelect, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lbBSelect, "SET B");


    lv_obj_add_event_cb(ui_btnRSelect, ui_event_ButtonSelectR, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_btnGSelect, ui_event_ButtonSelectG, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_btnBSelect, ui_event_ButtonSelectB, LV_EVENT_ALL, NULL);

    ui_lblDimmableOutputs = lv_label_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_lblDimmableOutputs, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_lblDimmableOutputs, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_lblDimmableOutputs, 241);
    lv_obj_set_y(ui_lblDimmableOutputs, -20);
    lv_obj_set_align(ui_lblDimmableOutputs, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lblDimmableOutputs, "Dimmable Outputs");
    lv_obj_set_style_text_font(ui_lblDimmableOutputs, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_swDim1 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swDim1, 73);
    lv_obj_set_height(ui_swDim1, 44);
    lv_obj_set_x(ui_swDim1, 163);
    lv_obj_set_y(ui_swDim1, 19);
    lv_obj_set_align(ui_swDim1, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_swDim1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim1, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swDim1, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swDim1, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim1, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_cbxDim1 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxDim1, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_obj_set_width(ui_cbxDim1, 140);
    lv_obj_set_height(ui_cbxDim1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxDim1, 279);
    lv_obj_set_y(ui_cbxDim1, 20);
    lv_obj_set_align(ui_cbxDim1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxDim1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxDim1, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_cbxDim2 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxDim2, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_obj_set_width(ui_cbxDim2, 140);
    lv_obj_set_height(ui_cbxDim2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxDim2, 279);
    lv_obj_set_y(ui_cbxDim2, 67);
    lv_obj_set_align(ui_cbxDim2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxDim2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxDim2, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_cbxDim3 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxDim3, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_obj_set_width(ui_cbxDim3, 140);
    lv_obj_set_height(ui_cbxDim3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxDim3, 279);
    lv_obj_set_y(ui_cbxDim3, 115);
    lv_obj_set_align(ui_cbxDim3, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxDim3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxDim3, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_cbxDim4 = lv_dropdown_create(ui_scrPanelSettings);
    lv_dropdown_set_options(ui_cbxDim4, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_obj_set_width(ui_cbxDim4, 140);
    lv_obj_set_height(ui_cbxDim4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_cbxDim4, 280);
    lv_obj_set_y(ui_cbxDim4, 164);
    lv_obj_set_align(ui_cbxDim4, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cbxDim4, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_cbxDim4, LV_OBJ_FLAG_CLICKABLE);      /// Flags



    ui_swDim2 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swDim2, 73);
    lv_obj_set_height(ui_swDim2, 44);
    lv_obj_set_x(ui_swDim2, 165);
    lv_obj_set_y(ui_swDim2, 69);
    lv_obj_set_align(ui_swDim2, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swDim2, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swDim2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim2, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swDim2, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swDim2, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim2, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_swDim3 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swDim3, 73);
    lv_obj_set_height(ui_swDim3, 44);
    lv_obj_set_x(ui_swDim3, 165);
    lv_obj_set_y(ui_swDim3, 119);
    lv_obj_set_align(ui_swDim3, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swDim3, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swDim3, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim3, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swDim3, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swDim3, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim3, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_swDim4 = lv_switch_create(ui_scrPanelSettings);
    lv_obj_set_width(ui_swDim4, 73);
    lv_obj_set_height(ui_swDim4, 44);
    lv_obj_set_x(ui_swDim4, 165);
    lv_obj_set_y(ui_swDim4, 167);
    lv_obj_set_align(ui_swDim4, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_swDim4, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_swDim4, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim4, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_swDim4, lv_color_hex(0xE6E2E6), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_swDim4, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_swDim4, 10, LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_swO1, ui_event_swO1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO2, ui_event_swO2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO3, ui_event_swO3, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO4, ui_event_swO4, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO5, ui_event_swO5, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO6, ui_event_swO6, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO7, ui_event_swO7, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO8, ui_event_swO8, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO9, ui_event_swO9, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO10, ui_event_swO10, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO11, ui_event_swO11, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO12, ui_event_swO12, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO14, ui_event_swO14, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO15, ui_event_swO15, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO16, ui_event_swO16, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swO13, ui_event_swO13, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swDim1, ui_event_swDim1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swDim2, ui_event_swDim2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swDim3, ui_event_swDim3, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_swDim4, ui_event_swDim4, LV_EVENT_ALL, NULL);

}


void set_language_for_dropdowns()
{
    lv_dropdown_set_options(ui_cbxO1,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO2,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO3,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO4,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO5,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO6,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO7,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO8,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO9,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO10,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO11,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO12,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO13,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO14,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO15,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");
    lv_dropdown_set_options(ui_cbxO16,
                            "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nAC\nUSB\nREGRIGE.\nWATER P.\nOUTLET\nOVEN\nTV\nEX. LIGHT\nEX. OUTLE\nHEATER\nSPOT\nREADING L.");

    lv_dropdown_set_options(ui_cbxDim1, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_dropdown_set_options(ui_cbxDim2, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_dropdown_set_options(ui_cbxDim3, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT");
    lv_dropdown_set_options(ui_cbxDim4, "LAMP\nTOILET\nKITCHEN\nBEDROOM\nCORRIDOR\nSTEP\nEX. LIGHT\nSPOT"); 
}

//convert dropdown options to Turkish
void set_language_for_dropdowns_TR()
{
    //"LAMBA", "TUVALET", "MUTFAK", "YATAK OD.", "KORIDOR", "MERDIVEN", "KLIMA", "USB", "BUZDOLABI", "SU POMPASI", "PRIZ", "FIRIN", "TV", "DIS. LAMBA", "DIS. PRIZ", "ISI TICICI", "SPOT", "OKUMA L."
    lv_dropdown_set_options(ui_cbxO1,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO2,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO3,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO4,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO5,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO6,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO7,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO8,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO9,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO10,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO11,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO12,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO13,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO14,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO15,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");
    lv_dropdown_set_options(ui_cbxO16,
                            "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nKLIMA\nUSB\nBUZDOLABI\nSU POMPASI\nPRIZ\nFIRIN\nTV\nDIS. LAMBA\nDIS. PRIZ\nISI TICICI\nSPOT\nOKUMA L.");

    //"LAMBA", "TUVALET", "MUTFAK", "YATAK OD.", "KORIDOR", "MERDIVEN", "DIS. LAMBA", "SPOT"
    lv_dropdown_set_options(ui_cbxDim1, "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nDIS. LAMBA\nSPOT");
    lv_dropdown_set_options(ui_cbxDim2, "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nDIS. LAMBA\nSPOT");
    lv_dropdown_set_options(ui_cbxDim3, "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nDIS. LAMBA\nSPOT");
    lv_dropdown_set_options(ui_cbxDim4, "LAMBA\nTUVALET\nMUTFAK\nYATAK OD.\nKORIDOR\nMERDIVEN\nDIS. LAMBA\nSPOT");
}