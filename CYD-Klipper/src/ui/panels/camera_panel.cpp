#include "lvgl.h"
#include "panel.h"
#include "../ui_utils.h"
#include "../nav_buttons.h"
#include "../../core/camera_integration.hpp"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"
#include "../../conf/global_config.h"
#include <stdio.h>
#include <SPIFFS.h>
#include <HTTPClient.h>

#define CAMERA_UPDATE_INTERVAL 3000  // Refresh camera image every 3 seconds

static lv_obj_t* camera_img = NULL;
static lv_obj_t* snapshot_btn = NULL;
static lv_obj_t* refresh_label = NULL;
static unsigned long last_refresh = 0;

static void update_camera_preview(lv_event_t* e);

static void cleanup_camera_image()
{
    // Free any remaining image data buffer
    // This should be called when the panel is destroyed
    // Note: This is a simplified cleanup - in production, track the buffer separately
}

void display_camera_image(lv_obj_t* parent, const char* filename)
{
    if (!global_config.camera_enabled) {
        return;
    }

    // Load image from cache
    size_t len;
    unsigned char* img_data;
    
    if (!get_camera_manager().get_cached_image(img_data, len)) {
        // Try to fetch new image
        HTTPClient client;
        BasePrinter* printer = get_current_printer();
        if (!printer || !printer->printer_config) {
            return;
        }
        KlipperPrinter* klipper_printer = static_cast<KlipperPrinter*>(printer);
        get_camera_manager().fetch_camera_preview(client, filename);
        
        // Reload from cache
        if (!get_camera_manager().get_cached_image(img_data, len)) {
            // No image available
            lv_obj_t* label = lv_label_create(parent);
            lv_label_set_text(label, "Camera not available\nor no print selected");
            lv_obj_center(label);
            return;
        }
    }

    // Create image object
    if (camera_img) {
        lv_obj_del(camera_img);
    }
    
    camera_img = lv_img_create(parent);
    lv_img_set_src(camera_img, img_data);
    lv_obj_set_size(camera_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(camera_img);

    // Note: Do NOT free(img_data) here - LVGL needs the buffer to display the image
    // The buffer will be freed when the image object is deleted or when the display updates
}

static void update_camera_preview(lv_event_t* e)
{
    if (!global_config.camera_enabled) {
        return;
    }

    PrinterData* data = get_current_printer_data();
    if (!data) {
        return;
    }
    const char* filename = data->print_filename;
    if (!filename || strlen(filename) == 0) {
        return;
    }

    // Check if we need to refresh
    unsigned long now = millis();
    if (now - last_refresh < CAMERA_UPDATE_INTERVAL) {
        return;
    }
    last_refresh = now;

    // Fetch new image
    HTTPClient client;
    BasePrinter* printer = get_current_printer();
    if (!printer || !printer->printer_config) {
        return;
    }
    KlipperPrinter* klipper_printer = static_cast<KlipperPrinter*>(printer);
    get_camera_manager().fetch_camera_preview(client, filename);

    // Update display
    if (camera_img) {
        lv_obj_del(camera_img);
    }

    // Reload and display
    size_t len;
    unsigned char* img_data;
    
    if (!get_camera_manager().get_cached_image(img_data, len)) {
        // Image fetch failed - show error message
        camera_img = lv_img_create(lv_scr_act());
        lv_obj_t* label = lv_label_create(camera_img);
        lv_label_set_text(label, "Camera error\nCheck connection");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_center(label);
        lv_obj_set_size(camera_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_center(camera_img);
        return;
    }

    camera_img = lv_img_create(lv_scr_act());
    lv_img_set_src(camera_img, img_data);
    lv_obj_set_size(camera_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(camera_img);

    // Note: Do NOT free(img_data) here - LVGL needs the buffer to display the image
    // The buffer will be freed when the image object is deleted or when the display updates
}
void camera_panel_init(lv_obj_t* panel)
{
    if (!global_config.camera_enabled) {
        lv_obj_t* label = lv_label_create(panel);
        lv_label_set_text(label, "Camera feature\nnot enabled");
        lv_obj_center(label);
        return;
    }

    PrinterData* data = get_current_printer_data();
    if (!data) {
        lv_obj_t* label = lv_label_create(panel);
        lv_label_set_text(label, "Printer not initialized");
        lv_obj_center(label);
        return;
    }
    const char* filename = data->print_filename;
    if (!filename || strlen(filename) == 0) {
        lv_obj_t* label = lv_label_create(panel);
        lv_label_set_text(label, "Select a print\nto view camera");
        lv_obj_center(label);
        
        // Add navigation button to files panel
        lv_obj_t* nav_btn = lv_btn_create(panel);
        lv_obj_set_size(nav_btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, 
                    CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_align(nav_btn, LV_ALIGN_BOTTOM_MID, 0, -CYD_SCREEN_GAP_PX);
        lv_obj_add_event_cb(nav_btn, [](lv_event_t* e) {
            nav_buttons_setup_deferred(PANEL_FILES);
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_t* nav_label = lv_label_create(nav_btn);
        lv_label_set_text(nav_label, "Browse Files");
        lv_obj_center(nav_label);
        return;
    }

    // Create camera display container
    lv_obj_t* container = lv_obj_create(panel);
    lv_obj_set_size(container, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_set_style_pad_all(container, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(container, LV_FLEX_ALIGN_SPACE_BETWEEN, 0, 0);

    // Top: Camera image area
    lv_obj_t* img_container = lv_obj_create(container);
    lv_obj_set_size(img_container, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, 
                    CYD_SCREEN_PANEL_HEIGHT_PX * 0.6);
    lv_layout_flex_column(img_container, LV_FLEX_ALIGN_CENTER);
    
    display_camera_image(img_container, filename);

    // Bottom: Controls
    lv_obj_t* controls = lv_create_empty_panel(container);
    lv_obj_set_size(controls, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, 
                    CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 2);
    lv_layout_flex_row(controls, LV_FLEX_ALIGN_SPACE_AROUND);

    // Snapshot button
    snapshot_btn = lv_btn_create(controls);
    lv_obj_set_height(snapshot_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_set_flex_grow(snapshot_btn, 1);
    lv_obj_add_event_cb(snapshot_btn, [](lv_event_t* e) {
        PrinterData* data = get_current_printer_data();
        if (!data) {
            return;
        }
        const char* filename = data->print_filename;
        if (filename && strlen(filename) > 0) {
            take_camera_snapshot(filename);
            lv_obj_t* btn = lv_event_get_target(e);
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, "Saved!");
            lv_obj_center(label);
            lv_obj_invalidate(btn);
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* snap_label = lv_label_create(snapshot_btn);
    lv_label_set_text(snap_label, LV_SYMBOL_IMAGE " Snapshot");
    lv_obj_center(snap_label);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(controls);
    lv_obj_set_height(back_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_set_flex_grow(back_btn, 1);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        nav_buttons_setup_deferred(PANEL_FILES);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_center(back_label);

    // Subscribe to printer data updates
    lv_obj_add_event_cb(panel, update_camera_preview, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, panel, NULL);
}
