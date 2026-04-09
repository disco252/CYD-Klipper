#include "panel.h"
#include "../../core/printer_integration.hpp"

void connecting_panel_init(lv_obj_t* panel) 
{
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        lv_obj_t* label = lv_label_create(panel);
        lv_label_set_text(label, "Connecting...");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        return;
    }
    
    lv_obj_t* label = lv_label_create(panel);
    const char* name = (printer->printer_config->printer_name[0] == 0) 
        ? printer->printer_config->printer_host 
        : printer->printer_config->printer_name;
    lv_label_set_text_fmt(label, "Connecting to %s...", name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}