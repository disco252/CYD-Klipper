#include "camera_integration.hpp"

// Camera stream management - implementation moved to header for inline compilation

CameraManager camera_manager;

CameraManager& get_camera_manager() {
    return camera_manager;
}

// Snapshot functionality
bool take_camera_snapshot(const char* filename) {
    if (!global_config.camera_enabled) {
        return false;
    }

    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer();
    PrinterConfiguration* printer_config = printer->printer_config;
    
    if (printer_config == NULL) {
        return false;
    }
    
    // Send snapshot command to Moonraker
    HTTPClient client;
    String url = "/server/files/gcodes/" + String(filename) + "/snapshot";
    client.begin("http://" + String(printer_config->printer_host) + ":" + 
                 String(printer_config->klipper_port) + url);
    
    if (printer_config->auth_configured) {
        client.addHeader("X-Api-Key", printer_config->printer_auth);
    }
    
    int http_code = client.POST("");
    return http_code == 200;
}
