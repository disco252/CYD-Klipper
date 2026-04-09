#include "lvgl.h"
#include "panel.h"
#include "../ui_utils.h"
#include "../../conf/global_config.h"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"
#include "../nav_buttons.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void save_quick_macros(lv_event_t* e);
static void load_quick_macros_from_printer();
static void build_macro_options(const Macros& macros, char* options, size_t size);

void quick_macro_settings_panel_init(lv_obj_t* panel)
{
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        lv_obj_t* label = lv_label_create(panel);
        lv_label_set_text(label, "Printer config not available");
        lv_obj_center(label);
        return;
    }
    load_quick_macros_from_printer();

    lv_obj_t* scroll = lv_obj_create(panel);
    lv_obj_set_size(scroll, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_align(scroll, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_border_width(scroll, 0, 0);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(scroll, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(scroll, LV_FLEX_ALIGN_SPACE_BETWEEN, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    // Get current macro list
    Macros macros = current_printer_get_macros();

    if (!macros.success || macros.count == 0) {
        lv_obj_t* label = lv_label_create(scroll);
        lv_label_set_text(label, "No macros available.\nMake sure macros are defined\nin printer config.");
        return;
    }

    char options[2048] = "";
    build_macro_options(macros, options, sizeof(options));

    // Display macro selection for each quick slot
    for (int i = 0; i < 6; i++) {
        if (printer->printer_config->quick_macros[i][0] == '\0' && i < (int)macros.count && macros.macros[i] != NULL) {
            strncpy(printer->printer_config->quick_macros[i], macros.macros[i], sizeof(printer->printer_config->quick_macros[i]) - 1);
            printer->printer_config->quick_macros[i][sizeof(printer->printer_config->quick_macros[i]) - 1] = '\0';
        }

        lv_obj_t* row = lv_create_empty_panel(scroll);
        lv_obj_set_size(row, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_layout_flex_row(row, LV_FLEX_ALIGN_SPACE_AROUND);

        lv_obj_t* label = lv_label_create(row);
        lv_label_set_text_fmt(label, "Quick Slot %d", i + 1);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);

        lv_obj_t* dropdown = lv_dropdown_create(row);
        lv_obj_set_flex_grow(dropdown, 1);
        lv_dropdown_set_options(dropdown, options);
        
        // Set current selection
        for (int j = 0; j < (int)macros.count && printer->printer_config->quick_macros[i][0] != '\0'; j++) {
            if (macros.macros[j] != NULL && strcmp(printer->printer_config->quick_macros[i], macros.macros[j]) == 0) {
                lv_dropdown_set_selected(dropdown, j);
                break;
            }
        }

        lv_obj_add_event_cb(dropdown, [](lv_event_t* e) {
            lv_obj_t* dropdown = lv_event_get_target(e);
            BasePrinter* printer = get_current_printer();
            if (!printer || !printer->printer_config) {
                return;
            }

            int slot = (int)(uintptr_t)lv_event_get_user_data(e);
            if (slot < 0 || slot >= 6) {
                return;
            }

            const char* option = lv_dropdown_get_text(dropdown);
            if (option == NULL) {
                return;
            }

            strncpy(printer->printer_config->quick_macros[slot], option, sizeof(printer->printer_config->quick_macros[slot]) - 1);
            printer->printer_config->quick_macros[slot][sizeof(printer->printer_config->quick_macros[slot]) - 1] = '\0';
        }, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)i);
    }

    // Save button
    lv_obj_t* save_btn = lv_btn_create(scroll);
    lv_obj_set_size(save_btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(save_btn, save_quick_macros, LV_EVENT_CLICKED, NULL);
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save Quick Macros");
    lv_obj_center(save_label);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(scroll);
    lv_obj_set_size(back_btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        nav_buttons_setup_deferred(PANEL_SETTINGS);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_center(back_label);
}

static void save_quick_macros(lv_event_t* e)
{
    (void)e;
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        return;
    }

    write_global_config();
    lv_create_popup_message("Quick macros saved", 1500);
}

static void load_quick_macros_from_printer()
{
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        return;
    }

    Macros macros = current_printer_get_macros();
    if (!macros.success) {
        return;
    }

    for (int i = 0; i < 6; i++) {
        if (printer->printer_config->quick_macros[i][0] == '\0' && i < (int)macros.count && macros.macros[i] != NULL) {
            strncpy(printer->printer_config->quick_macros[i], macros.macros[i], sizeof(printer->printer_config->quick_macros[i]) - 1);
            printer->printer_config->quick_macros[i][sizeof(printer->printer_config->quick_macros[i]) - 1] = '\0';
        }
    }
}

static void build_macro_options(const Macros& macros, char* options, size_t size)
{
    if (options == NULL || size == 0) {
        return;
    }

    options[0] = '\0';

    for (unsigned int i = 0; i < macros.count; i++) {
        if (macros.macros[i] == NULL || macros.macros[i][0] == '\0') {
            continue;
        }

        if (options[0] != '\0') {
            strncat(options, "\n", size - strlen(options) - 1);
        }

        strncat(options, macros.macros[i], size - strlen(options) - 1);
    }
}
