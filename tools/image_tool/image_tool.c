#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RGBA_FMT "RGBA8888"
#define MAX_PATH 256

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t* pixels;
} RGBAImage;

static RGBAImage* rgba_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint32_t width, height;
    fread(&width, 4, 1, f);
    fread(&height, 4, 1, f);
    
    uint8_t* pixels = malloc(size - 8);
    fread(pixels, 1, size - 8, f);
    fclose(f);
    
    RGBAImage* img = malloc(sizeof(RGBAImage));
    img->width = width;
    img->height = height;
    img->pixels = pixels;
    return img;
}

static void rgba_save(const char* path, RGBAImage* img) {
    FILE* f = fopen(path, "wb");
    if (!f) return;
    
    fwrite(&img->width, 4, 1, f);
    fwrite(&img->height, 4, 1, f);
    fwrite(img->pixels, 1, img->width * img->height * 4, f);
    fclose(f);
}

static int parse_size(const char* str, uint32_t* out_w, uint32_t* out_h) {
    int w = 0, h = 0;
    if (sscanf(str, "%ux%u", &w, &h) == 2) {
        *out_w = w;
        *out_h = h;
        return 0;
    }
    return -1;
}

static void print_usage(const char* prog) {
    printf("LeonelOS Image Tool\n");
    printf("Usage: %s <command> [options]\n", prog);
    printf("\n");
    printf("Commands:\n");
    printf("  info <file.rgba>    - Display image information\n");
    printf("  resize <file> WxH  - Resize RGBA image\n");
    printf("  convert <in> <out>  - Convert image format\n");
    printf("  validate <file>     - Validate RGBA file\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "info") == 0 && argc >= 3) {
        RGBAImage* img = rgba_load(argv[2]);
        if (!img) {
            fprintf(stderr, "Error: Cannot load %s\n", argv[2]);
            return 1;
        }
        printf("Image: %ux%u RGBA8888 (%zu bytes)\n",
               img->width, img->height,
               (size_t)img->width * img->height * 4);
        free(img->pixels);
        free(img);
        return 0;
    }
    
    if (strcmp(argv[1], "resize") == 0 && argc >= 4) {
        uint32_t w, h;
        if (parse_size(argv[3], &w, &h) != 0) {
            fprintf(stderr, "Error: Invalid size format. Use WxH\n");
            return 1;
        }
        
        RGBAImage* img = rgba_load(argv[2]);
        if (!img) {
            fprintf(stderr, "Error: Cannot load %s\n", argv[2]);
            return 1;
        }
        
        RGBAImage* resized = malloc(sizeof(RGBAImage));
        resized->width = w;
        resized->height = h;
        resized->pixels = malloc(w * h * 4);
        
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                uint32_t src_x = (x * img->width) / w;
                uint32_t src_y = (y * img->height) / h;
                uint32_t src_idx = (src_y * img->width + src_x) * 4;
                uint32_t dst_idx = (y * w + x) * 4;
                resized->pixels[dst_idx + 0] = img->pixels[src_idx + 0];
                resized->pixels[dst_idx + 1] = img->pixels[src_idx + 1];
                resized->pixels[dst_idx + 2] = img->pixels[src_idx + 2];
                resized->pixels[dst_idx + 3] = img->pixels[src_idx + 3];
            }
        }
        
        const char* out = (argc >= 5) ? argv[4] : "output.rgba";
        rgba_save(out, resized);
        printf("Resized to %ux%u -> %s\n", w, h, out);
        
        free(img->pixels);
        free(img);
        free(resized->pixels);
        free(resized);
        return 0;
    }
    
    if (strcmp(argv[1], "validate") == 0 && argc >= 3) {
        RGBAImage* img = rgba_load(argv[2]);
        if (!img) {
            fprintf(stderr, "ERROR: Invalid or missing RGBA file\n");
            return 1;
        }
        printf("VALID: %ux%u RGBA8888\n", img->width, img->height);
        free(img->pixels);
        free(img);
        return 0;
    }
    
    if (strcmp(argv[1], "convert") == 0 && argc >= 4) {
        printf("Converting %s to %s\n", argv[2], argv[3]);
        printf("Output format: RGBA8888\n");
        RGBAImage* img = rgba_load(argv[2]);
        if (!img) {
            fprintf(stderr, "Error: Cannot load %s\n", argv[2]);
            return 1;
        }
        rgba_save(argv[3], img);
        printf("Converted successfully\n");
        free(img->pixels);
        free(img);
        return 0;
    }
    
    print_usage(argv[0]);
    return 1;
}