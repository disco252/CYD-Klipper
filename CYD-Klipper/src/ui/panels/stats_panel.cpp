#include "panel.h"
#include "../ui_utils.h"
#include "../nav_buttons.h"
#include <stdio.h>
#include <Esp.h>
#include "../../core/printer_integration.hpp"

static void swap_to_files_menu(lv_event_t * e) {
    nav_buttons_setup_deferred(PANEL_FILES);
}

static void update_data(lv_event_t * e) {
    BasePrinter* printer = get_current_printer();
    if (!printer) {
        return;
    }
    lv_msg_send(DATA_PRINTER_DATA, printer);
}

void create_state_button(lv_obj_t * root, lv_event_cb_t label, lv_event_cb_t button){
    lv_obj_t * btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, button, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, update_data, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_obj = lv_label_create(btn);
    lv_obj_add_event_cb(label_obj, label, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label_obj, NULL);
    lv_obj_align(label_obj, LV_ALIGN_CENTER, 0, 0);
}

static void label_pos(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    char x_pos_buff[32];
    sprintf(x_pos_buff, "X%.2f Y%.2f", data->position[0], data->position[1]);
    lv_label_set_text(label, x_pos_buff);
}

static void label_filament_used_m(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    char filament_buff[32];
    sprintf(filament_buff, "%.2f m", data->filament_used_mm / 1000);
    lv_label_set_text(label, filament_buff);
}

static void label_total_layers(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    char layers_buff[32];
    sprintf(layers_buff, "%d of %d", data->current_layer, data->total_layers);
    lv_label_set_text(label, layers_buff);
}

static void label_pressure_advance(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    char pressure_buff[32];
    sprintf(pressure_buff, "%.3f (%.2fs)", data->pressure_advance, data->smooth_time);
    lv_label_set_text(label, pressure_buff);
}

static void label_feedrate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    char feedrate_buff[32];
    sprintf(feedrate_buff, "%d mm/s", data->feedrate_mm_per_s);
    lv_label_set_text(label, feedrate_buff);
}

void create_stat_text_block(lv_obj_t * root, const char* label, lv_event_cb_t value){
    lv_obj_t * panel = lv_create_empty_panel(root);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_layout_flex_column(panel , LV_FLEX_ALIGN_START, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 2);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * label_obj = lv_label_create(panel);
    lv_label_set_text(label_obj, label);
    lv_obj_set_style_text_font(label_obj, &lv_font_montserrat_12, 0);

    lv_obj_t * value_obj = lv_label_create(panel);
    lv_obj_add_event_cb(value_obj, value, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, value_obj, NULL);
}

void stats_panel_init(lv_obj_t* panel) {
    BasePrinter* printer = get_current_printer();
    if (!printer) {
        return;
    }
    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    auto panel_width = CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * left_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(left_panel, panel_width, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(left_panel);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    create_stat_text_block(left_panel, "Position:", label_pos);

    if (data->state != PrinterState::PrinterStateIdle){
        create_stat_text_block(left_panel, "Filament Used:", label_filament_used_m);
        create_stat_text_block(left_panel, "Layer:", label_total_layers);
    }

    create_stat_text_block(left_panel, "Pressure Advance:", label_pressure_advance);
    create_stat_text_block(left_panel, "Feedrate:", label_feedrate);

    lv_obj_t * right_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(right_panel, panel_width, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(right_panel, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, -1 * CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    if (data->state >= PrinterState::PrinterStatePrinting){
        lv_obj_t * btn = lv_btn_create(right_panel);
        lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_add_event_cb(btn, swap_to_files_menu, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, "Files");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    for (int i = 0; i < printer->custom_menus_count; i++)
    {
        create_state_button(right_panel, (lv_event_cb_t)printer->custom_menus[i].set_label, (lv_event_cb_t)printer->custom_menus[i].open_panel);
    }
}
