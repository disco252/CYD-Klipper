#include "lvgl.h"
#include "panel.h"
#include "../ui_utils.h"
#include "../nav_buttons.h"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"
#include "../../conf/global_config.h"
#include "../../core/data_setup.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void quick_macro_bar_init(lv_obj_t* panel)
{
    if (!global_config.quick_macro_bar) {
        return;
    }

    // Check if there are any macros configured
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        return;
    }

    Macros macros = current_printer_get_macros();
    
    if (!macros.success || macros.count == 0) {
        return;
    }

    // Create the bar container at the bottom of the panel
    lv_obj_t* bar = lv_obj_create(panel);
    lv_obj_set_size(bar, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_layout_flex_row(bar, LV_FLEX_ALIGN_START, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    // Get current macro list
    int macro_count = current_printer_get_macros_count();
    if (macro_count == 0) {
        return;
    }

    // Initialize quick macros array if needed
    for (int i = 0; i < 6; i++) {
        if (printer->printer_config->quick_macros[i][0] == '\0') {
            // Copy first available macro as default
            if (i < macros.count && macros.macros[i] != NULL) {
                strncpy(printer->printer_config->quick_macros[i], macros.macros[i], sizeof(printer->printer_config->quick_macros[i]) - 1);
                printer->printer_config->quick_macros[i][sizeof(printer->printer_config->quick_macros[i]) - 1] = '\0';
            }
        }
    }

    // Create macro buttons
    for (int i = 0; i < 6; i++) {
        if (printer->printer_config->quick_macros[i][0] == '\0') {
            continue;
        }

        lv_obj_t* btn = lv_btn_create(bar);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_set_flex_grow(btn, 1);
        
        // Store macro index as user data (safer than storing pointer)
        lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = lv_event_get_target(e);
            int macro_index = (int)(uintptr_t)lv_obj_get_user_data(btn);
            BasePrinter* printer = get_current_printer();
            if (!printer) {
                return;
            }
            if (macro_index >= 0 && macro_index < 6) {
                const char* macro_name = printer->printer_config->quick_macros[macro_index];
                if (macro_name != NULL && macro_name[0] != '\0') {
                    current_printer_execute_macro(macro_name);
                }
            }
        }, LV_EVENT_CLICKED, NULL);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(lbl, lv_obj_get_width(btn) - 10);
        lv_label_set_text(lbl, printer->printer_config->quick_macros[i]);
        lv_obj_center(lbl);
    }

    // Add a "Manage" button at the end
    lv_obj_t* manage_btn = lv_btn_create(bar);
    lv_obj_set_height(manage_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_set_flex_grow(manage_btn, 0);
    lv_obj_set_style_bg_opa(manage_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(manage_btn, 1, 0);
    lv_obj_set_style_border_color(manage_btn, lv_color_hex(0x666666), 0);
    
    lv_obj_add_event_cb(manage_btn, [](lv_event_t* e) {
        nav_buttons_setup_deferred(PANEL_QUICK_MACRO);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* manage_label = lv_label_create(manage_btn);
    lv_label_set_text(manage_label, "Manage");
}
