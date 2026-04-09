#pragma once

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../conf/global_config.h"
#include "klipper/klipper_printer_integration.hpp"
#include "core/current_printer.h"
#include <SPIFFS.h>
#include <LittleFS.h>

#define CAMERA_IMAGE_CACHE "/camera_preview.jpg"
#define CAMERA_IMAGE_CACHE_SIZE 200000  // Maximum image size to cache

// Camera stream management
class CameraManager {
public:
    CameraManager() : last_fetch(0), fetch_interval(3000) {}

    bool fetch_camera_preview(HTTPClient& client, const char* filename, int timeout = 5000) {
        if (!global_config.camera_enabled) {
            return false;
        }

        // Get printer configuration from current printer
        KlipperPrinter* printer = (KlipperPrinter*)get_current_printer();
        PrinterConfiguration* printer_config = printer->printer_config;
        
        if (printer_config == NULL) {
            return false;
        }

        // Try to get thumbnail first, then full image
        String url = "/server/files/gcodes/" + String(filename) + "/thumbnail";
        client.begin("http://" + String(printer_config->printer_host) + ":" + 
                     String(printer_config->klipper_port) + url);
        
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
        
        if (printer_config->auth_configured) {
            client.addHeader("X-Api-Key", printer_config->printer_auth);
        }

        int http_code = client.GET();
        if (http_code != 200) {
            return false;
        }

        // Read image data
        size_t len = client.getSize();
        if (len > CAMERA_IMAGE_CACHE_SIZE) {
            len = CAMERA_IMAGE_CACHE_SIZE;  // Limit cache size
        }

        unsigned char* data = (unsigned char*)malloc(len + 1);
        if (!data) {
            return false;
        }

        if (len != client.getStream().readBytes((char*)data, len)) {
            free(data);
            return false;
        }

        // Save to SPIFFS
        File f = SPIFFS.open(CAMERA_IMAGE_CACHE, FILE_WRITE);
        if (f) {
            f.write(data, len);
            f.close();
        }

        free(data);
        return true;
    }

    bool get_cached_image(unsigned char*& data, size_t& len) {
        File f = SPIFFS.open(CAMERA_IMAGE_CACHE, FILE_READ);
        if (!f) {
            return false;
        }

        len = f.size();
        if (len > CAMERA_IMAGE_CACHE_SIZE) {
            len = CAMERA_IMAGE_CACHE_SIZE;
        }

        data = (unsigned char*)malloc(len + 1);
        if (!data) {
            f.close();
            return false;
        }

        if (len != f.read((uint8_t*)data, len)) {
            free(data);
            f.close();
            return false;
        }

        f.close();
        return true;
    }

    void cleanup() {
        File f = SPIFFS.open(CAMERA_IMAGE_CACHE, FILE_READ);
        if (f) {
            f.close();
            SPIFFS.remove(CAMERA_IMAGE_CACHE);
        }
    }

    void cleanup_camera_image() {
        cleanup();
    }

private:
    unsigned long last_fetch;
    int fetch_interval;
};

CameraManager& get_camera_manager();

// Snapshot functionality
bool take_camera_snapshot(const char* filename);
