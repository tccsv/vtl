#define _POSIX_C_SOURCE 199309L
#include <VTL_vencode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W 1280
#define H 720
#define FRAMES 100

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static uint8_t* generate_frame(int frame_idx) {
    size_t sz = W * H * 3 / 2;
    uint8_t* data = (uint8_t*)malloc(sz);
    memset(data, 0, W * H);         
    memset(data + W * H, 128, sz - W * H); 
    for (int i = 0; i < 200; i++) data[(frame_idx * 10 + i) % (W * H)] = 255;
    return data;
}

void run_encoding_demo(const char* name, VTL_vencode_Api api, VTL_vencode_Flags flags, int threads) {
    printf("\n--- Starting Demo: %s (Threads: %d) ---\n", name, threads);

    VTL_vencode_Config cfg = { .log_level = 0, .thread_count = threads, .flags = flags };
    VTL_vencode_Init(&cfg);

    char filename[64];
    snprintf(filename, sizeof(filename), "demo_%s.mp4", name);

    VTL_vencode_Context* ctx = VTL_vencode_Open(VTL_vencode_codec_kH264, api, filename);
    if (!ctx) {
        printf("Failed to open encoder for %s\n", name);
        VTL_vencode_Deinit();
        return;
    }

    double start = get_time_ms();

    for (int i = 0; i < FRAMES; i++) {
        uint8_t* raw = generate_frame(i);
        VTL_vencode_EncodeFrame(ctx, raw, W * H * 3 / 2, W, H, VTL_vencode_pixfmt_kYUV420P);
        free(raw);
        if (i % 25 == 0) printf("Encoded frame %d/%d...\n", i, FRAMES);
    }

    VTL_vencode_Close(ctx);
    
    double end = get_time_ms();
    
    if (api == VTL_vencode_api_kCPU) {
        double duration = (end - start);
        printf(">>> CPU Encoding Time: %.2f milliseconds (%.2f FPS)\n", duration, FRAMES / duration);
    }

    VTL_vencode_Deinit();
    printf("Demo '%s' finished. Output: %s\n", name, filename);
}

int main(int argc, char** argv) {
    int threads = 0;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }

    printf("VTL libvencode Performance & Multi-threading Demo\n");
    printf("==============================================\n");
    printf("Resolution: %dx%d, Frames: %d\n", W, H, FRAMES);

    /* 1 - Чистый CPU (с замером времени) */
    run_encoding_demo("CPU_Bench", VTL_vencode_api_kCPU, VTL_VENCODE_FLAG_NONE, threads);

    /* 2 - Telegram Optimal (720p + Bitrate limit) */
    run_encoding_demo("TG_Optimal", VTL_vencode_api_kCPU, VTL_VENCODE_FLAG_TG_OPTIMAL, threads);

    /* 3 - Vulkan (если доступно) */
    run_encoding_demo("Vulkan_Only", VTL_vencode_api_kVulkan, VTL_VENCODE_FLAG_NONE, threads);

    return 0;
}
