#include <stdio.h>
#include "VTL/media_container/img/VTL_img_load.h"
#include "VTL/media_container/img/VTL_img_blur.h"
#include "VTL/media_container/img/VTL_img_save.h"
#include <libavutil/pixfmt.h>

int main(void) {
    const char* input_path = "input.png";   // PNG 8-bit grayscale
    const char* output_path = "output.png";
    AVFrame* frame = NULL;
    int width = 0, height = 0;
    enum AVPixelFormat pix_fmt;
    int ret = VTL_img_load(input_path, &frame, &width, &height, &pix_fmt);
    if (ret < 0) {
        printf("[ERROR] Failed to load image: %s\n", input_path);
        return 1;
    }
    printf("[INFO] Loaded image %s: %dx%d, format=%d\n", input_path, width, height, pix_fmt);
    AVFrame* blurred = NULL;
    ret = VTL_img_blur(frame, &blurred, width, height, pix_fmt, NULL);
    if (ret < 0) {
        printf("[ERROR] Blur failed (OpenCL error: %d)\n", ret);
        av_frame_free(&frame);
        return 3;
    }
    ret = VTL_img_save(output_path, blurred);
    if (ret < 0) {
        printf("[ERROR] Failed to save blurred image: %s\n", output_path);
        av_frame_free(&frame);
        av_frame_free(&blurred);
        return 4;
    }
    printf("[OK] Blurred image saved to %s\n", output_path);
    av_frame_free(&frame);
    av_frame_free(&blurred);
    return 0;
}