#include <VTL/media_container/img/VTL_img_core.h>
#include <VTL/media_container/img/VTL_img_filters.h>
#include <VTL/media_container/img/VTL_img_utils.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    const char* input_path  = "./test_images/test2-1.jpg";
    const char* output_path = "./test_images/test2-1_output.jpg";

    if (!VTL_img_CheckFileExists(input_path)) {
        printf("File not found: %s\n", input_path);
        return 1;
    }

    if (!VTL_img_IsFormatSupported(input_path)) {
        printf("Unsupported format: %s\n", input_path);
        return 1;
    }

    printf("Input format: %s\n",
           VTL_img_GetFormatDescription(input_path));

    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        printf("Failed to init context\n");
        return 1;
    }

    if (VTL_img_LoadImage(input_path, ctx) != 0) {
        printf("Failed to load image\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    printf("Image loaded: %dx%d\n",
           ctx->current_frame->width,
           ctx->current_frame->height);

    const VTL_ImageFilter* filter = &VTL_img_filter_grayscale;


    printf("Applying filter: %s\n", filter->name);

    if (VTL_img_ApplyFilter(ctx, filter) != 0) {
        printf("Failed to apply filter\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    if (VTL_img_SaveImage(output_path, ctx) != 0) {
        printf("Failed to save image\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    printf("Saved to: %s\n", output_path);

    VTL_img_context_Cleanup(ctx);

    printf("Parallel example\n");

    const char* input_paths[] = {
            "./test_images/2-1.jpg",
            "./test_images/2-2.jpg",
            "./test_images/2-3.jpg"
    };

    const char* output_paths[] = {
            "./test_images/2-1_out.png",
            "./test_images/2-2_out.png",
            "./test_images/2-3_out.png"
    };


    const VTL_ImageFilter* filters[] = {
            &VTL_img_filter_grayscale,
            &VTL_img_filter_sepia,
            &VTL_img_filter_gaussian_blur
    };

    int count = 3;

    printf("Num of images %d \n", count);

    VTL_AppResult res = VTL_img_ProcessBatch(input_paths, filters, output_paths, count);

    if (res == VTL_res_kOk) {
        printf("Success\n");
    } else {
        printf("Error\n");
    }

    return 0;
}