#include "ei_camera.h"
#include "esp_camera.h"
#include "Arduino.h"

// --- ESP-EYE Pin Map (Fixed) ---
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     4
#define SIOD_GPIO_NUM    18
#define SIOC_GPIO_NUM    23

#define Y9_GPIO_NUM      36
#define Y8_GPIO_NUM      37
#define Y7_GPIO_NUM      38
#define Y6_GPIO_NUM      39
#define Y5_GPIO_NUM      35
#define Y4_GPIO_NUM      14
#define Y3_GPIO_NUM      13
#define Y2_GPIO_NUM      34

#define VSYNC_GPIO_NUM    5
#define HREF_GPIO_NUM    27
#define PCLK_GPIO_NUM    25

static camera_fb_t *fb = NULL;  
static float image_buffer[96 * 96];  // ~37 KB float buffer

int ei_camera_init(void) {
    Serial.println("🚀 Initializing camera...");
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0 = 34;
    config.pin_d1 = 13;
    config.pin_d2 = 14;
    config.pin_d3 = 35;
    config.pin_d4 = 39;
    config.pin_d5 = 38;
    config.pin_d6 = 37;
    config.pin_d7 = 36;
    config.pin_xclk = 4;
    config.pin_pclk = 25;
    config.pin_vsync = 5;
    config.pin_href = 27;
    config.pin_sscb_sda = 18;
    config.pin_sscb_scl = 23;
    config.pin_pwdn = -1;
    config.pin_reset = -1;

    config.xclk_freq_hz = 10000000;         // Lower clock → less DMA stress
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size   = FRAMESIZE_QQVGA;  // 160x120
    config.jpeg_quality = 12;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.fb_count     = 2;                // Double buffering
    config.grab_mode    = CAMERA_GRAB_LATEST;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("❌ Camera init failed with error 0x%x\n", err);
        return 1;
    }

    Serial.println("📷 Camera initialized successfully.");
    return 0;
}

int ei_camera_capture(void) {
    if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ Camera capture failed");
        return 1;
    }

    Serial.printf("✅ Frame captured: %dx%d\n", fb->width, fb->height);
    return 0;
}

int ei_camera_get_signal(ei::signal_t *signal) {
    if (!fb) {
        Serial.println("⚠️ No frame buffer available");
        return 1;
    }

    // Downsample to 96x96 and normalize to 0-1
    int src_w = fb->width;
    int src_h = fb->height;

    for (int y = 0; y < 96; y++) {
        for (int x = 0; x < 96; x++) {
            int srcX = x * src_w / 96;
            int srcY = y * src_h / 96;
            uint8_t pixel = fb->buf[srcY * src_w + srcX];
            image_buffer[y * 96 + x] = pixel / 255.0f;
        }
    }

    // Prepare signal
    signal->total_length = 96 * 96;
    signal->get_data = [](size_t offset, size_t length, float *out_ptr) -> int {
        memcpy(out_ptr, image_buffer + offset, length * sizeof(float));
        return 0;
    };

    // Immediately release frame to avoid DMA overflow
    ei_camera_release_frame();

    return 0;
}

void ei_camera_release_frame(void) {
    if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
        Serial.println("🗑️ Frame buffer released");
    }
}
