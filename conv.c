#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <math.h>

png_color       mask_col = {85, 85, 85};    // mask_col => mask = 0
int             dark_idx;

int mapidx(int idx) {
    if (!idx)
        return dark_idx;
    else if (idx == dark_idx)
        return 0;

    return idx;
}    

float limcol(float v) {
    if (v > 15.0f)
        v = 15.0f;

    return v;
}

int main(void) {
    FILE    *f = fopen("tiles.png", "rb");
    if (!f) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    unsigned char   header[2];
    size_t in = fread(header, 1, 2, f);
    if (png_sig_cmp(header, 0, 2) || in != 2)
        exit(EXIT_FAILURE);

    png_structp png_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
            NULL, NULL);
    if (!png_p)
        exit(EXIT_FAILURE);

    png_infop info_p = png_create_info_struct(png_p);
    if (!info_p) {
        png_destroy_read_struct(&png_p, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    png_infop end_info_p = png_create_info_struct(png_p);
    if (!end_info_p) {
        png_destroy_read_struct(&png_p, &info_p, NULL);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_p))) {
        png_destroy_read_struct(&png_p, &info_p, &end_info_p);
        exit(EXIT_FAILURE);
    }

    png_init_io(png_p, f);
    png_set_sig_bytes(png_p, 2);
    png_read_png(png_p, info_p, PNG_TRANSFORM_PACKING, NULL);

    png_uint_32 w, h;
    png_int_32  d, c;
    png_get_IHDR(png_p, info_p, &w, &h, &d, &c, NULL, NULL, NULL);

    if (w & 0xf) {
        printf("Image width must be multiple of 16\n");
        png_destroy_read_struct(&png_p, &info_p, &end_info_p);
        exit(EXIT_FAILURE);
    }

    if (c != PNG_COLOR_TYPE_PALETTE) {
        printf("Image must contain a palette\n");
        png_destroy_read_struct(&png_p, &info_p, &end_info_p);
        exit(EXIT_FAILURE);
    }

    png_colorp  pal_p;
    png_int_32  pal_size;
    png_get_PLTE(png_p, info_p, &pal_p, &pal_size);
    printf("pal_size: %d\n", pal_size);

    switch (pal_size) {
        case 16:
            d = 4;
            break;
        case 4:
            d = 2;
            break;
        case 2:
            d = 1;
            break;
        default:
            printf("Palette size must be 16, 4, or 2\n");
            png_destroy_read_struct(&png_p, &info_p, &end_info_p);
            exit(EXIT_FAILURE);
    }
    printf("w: %u, h: %u, d: %d, c: %d\n", w, h, d, c);

    FILE    *o = fopen("colors.pal", "w");
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    int min_sat = 255*255*3 + 1;
    for (int i = 0; i < pal_size; i++) {
        int sat;
        png_byte    r, g, b;
        r = pal_p[i].red;
        g = pal_p[i].green;
        b = pal_p[i].blue;
        sat = r*r + g*g + b*b;

        if (sat < min_sat) {
            dark_idx = i;
            min_sat = sat;
        }
    }
    printf("original darkest color index: %d\n", dark_idx);

    png_color   temp;
    temp = pal_p[0];
    pal_p[0] = pal_p[dark_idx];
    pal_p[dark_idx] = temp;

    int mask_idx = -1;
    for (int i = 0; i < pal_size; i++) {
        png_byte    r, g, b;
        r = pal_p[i].red;
        g = pal_p[i].green;
        b = pal_p[i].blue;

        if (r == mask_col.red && g == mask_col.green && b == mask_col.blue)
            mask_idx = i;

        printf("%2d: r: %-3d  g: %-3d  b: %-3d\n", i, r, g, b);

        unsigned char   buffer[2];
        buffer[0] = (int)limcol(roundf(r/255.0f*15.0f));
        buffer[1] = (int)limcol(roundf(b/255.0f*15.0f));
        buffer[1] |= (int)limcol(roundf(g/255.0f*15.0f)) << 4;
        fwrite(buffer, 1, 2, o);
    }
    fclose(o);

    printf("mask color index: %d\n", mask_idx);

    o = fopen("tileset.raw", "w");
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    png_bytep   *row_pointers_p;
    row_pointers_p = png_get_rows(png_p, info_p);
    for (int i = 0; i < h; i++) {
        unsigned char   buffer;

        for (int l = 0; l < d; l++)
            for (int j = 0; j < w; j += 8) {
                buffer = 0;
                for (int k = 0; k < 8; k++)
                    if (mapidx(*(row_pointers_p[i] + j + k)) & (1 << l))
                        buffer |= 1 << (7 - k);
                
                fwrite(&buffer, 1, 1, o);
            }
    }

    // Add mask
    if (mask_idx >= 0)
        for (int i = 0; i < h; i++) {
            unsigned char   buffer;

            for (int l = 0; l < d; l++)
                for (int j = 0; j < w; j += 8) {
                    buffer = 0;
                    for (int k = 0; k < 8; k++)
                        if (mapidx(*(row_pointers_p[i] + j + k)) != mask_idx)
                                buffer |= 1 << (7 - k);
                        
                    fwrite(&buffer, 1, 1, o);
                }
        }
    else
        printf("Mask data is omitted\n");

    fclose(o);
    exit(EXIT_SUCCESS);
}
