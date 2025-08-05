#ifndef EI_CAMERA_H
#define EI_CAMERA_H

#include "edge-impulse-sdk/classifier/ei_classifier_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize camera */
int ei_camera_init(void);

/** Capture an image into internal framebuffer */
int ei_camera_capture(void);

/** Convert the last captured frame into Edge Impulse signal */
int ei_camera_get_signal(ei::signal_t *signal);

/** Release the frame buffer to free PSRAM */
void ei_camera_release_frame(void);

#ifdef __cplusplus
}
#endif

#endif // EI_CAMERA_H
