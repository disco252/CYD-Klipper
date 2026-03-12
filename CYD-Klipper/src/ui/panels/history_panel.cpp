#include "panel.h"
#include "../ui_utils.h"
#include "../nav_buttons.h"
#include "../../core/klipper/klipper_printer_integration.hpp"
#include "../../core/semaphore.h"
#include <stdio.h>
#include <string.h>

void history_panel_init(lv_obj_t* panel)
{
    freeze_request_thread();
    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer();
    PrintHistoryResult history = printer->get_print_history();
    unfreeze_request_thread();

    lv_obj_t* scroll = lv_obj_create(panel);
    lv_obj_set_size(scroll, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_align(scroll, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_border_width(scroll, 0, 0);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(scroll, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(scroll);

    if (!history.totals.success)
    {
        lv_obj_t* label = lv_label_create(scroll);
        lv_label_set_text(label, "History unavailable.\nCheck Moonraker config.");
        return;
    }

    // Totals header
    char buf[128];
    unsigned long h = (unsigned long)(history.totals.total_print_time_s / 3600);
    unsigned long m = ((unsigned long)history.totals.total_print_time_s % 3600) / 60;
    sprintf(buf, "Prints: %d   Time: %ldh%ldm   Filament: %.1fm",
        history.totals.total_jobs, h, m,
        history.totals.total_filament_used_mm / 1000.0f);
    lv_obj_t* totals_label = lv_label_create(scroll);
    lv_obj_set_style_text_font(totals_label, &CYD_SCREEN_FONT_SMALL, 0);
    lv_label_set_long_mode(totals_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(totals_label, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 4);
    lv_label_set_text(totals_label, buf);

    // Divider
    lv_obj_t* div = lv_obj_create(scroll);
    lv_obj_set_size(div, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 4, 2);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x555555), 0);
    lv_obj_set_style_pad_all(div, 0, 0);

    if (history.job_count == 0)
    {
        lv_obj_t* label = lv_label_create(scroll);
        lv_label_set_text(label, "No recent jobs.");
        return;
    }

    // Recent jobs
    for (int i = 0; i < history.job_count; i++)
    {
        auto& job = history.jobs[i];
        const char* icon = LV_SYMBOL_OK;
        if (strcmp(job.status, "cancelled") == 0) icon = LV_SYMBOL_STOP;
        else if (strcmp(job.status, "error") == 0) icon = LV_SYMBOL_CLOSE;

        unsigned long jh = job.print_duration_s / 3600;
        unsigned long jm = (job.print_duration_s % 3600) / 60;

        lv_obj_t* row = lv_create_empty_panel(scroll);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_layout_flex_column(row, LV_FLEX_ALIGN_START, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 2);

        lv_obj_t* name = lv_label_create(row);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 4);
        sprintf(buf, "%s %s", icon, job.filename);
        lv_label_set_text(name, buf);

        lv_obj_t* stats = lv_label_create(row);
        lv_obj_set_style_text_font(stats, &CYD_SCREEN_FONT_SMALL, 0);
        sprintf(buf, "%ldh%02ldm   %.2fm filament", jh, jm, job.filament_used_mm / 1000.0f);
        lv_label_set_text(stats, buf);
    }
}
