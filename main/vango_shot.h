#pragma once

void vango_shot_note_fb(const void *fb);
void vango_shot_start(void);
/** Soft trigger — UART stdin yoksa bile shot task frame dumper. */
void vango_shot_request(void);
/** Call after LVGL dash is up — ignores early 'shot' that caused assert/reset. */
void vango_shot_set_ready(void);
