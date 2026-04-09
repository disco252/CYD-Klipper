#include "lvgl.h"

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void settings_panel_init(lv_obj_t* panel);
void temp_panel_init(lv_obj_t* panel);
void files_panel_init(lv_obj_t* panel);
void move_panel_init(lv_obj_t* panel);
void progress_panel_init(lv_obj_t* panel);
void macros_panel_init(lv_obj_t* panel);
void stats_panel_init(lv_obj_t* panel);
void printer_panel_init(lv_obj_t* panel);
void error_panel_init(lv_obj_t* panel);
void connecting_panel_init(lv_obj_t* panel);

void history_panel_init(lv_obj_t* panel);
void jog_panel_init(lv_obj_t* panel);

void settings_section_device(lv_obj_t* panel);
void jog_panel_back(lv_event_t* e);
void quick_macro_bar_init(lv_obj_t* panel);
void camera_panel_init(lv_obj_t* panel);
void quick_macro_settings_panel_init(lv_obj_t* panel);