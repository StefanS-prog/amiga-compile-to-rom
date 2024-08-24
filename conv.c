#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <math.h>

png_color mask_col = { 85, 85, 85 };    // mask_col => mask = 0
int dark_idx;

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

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("File name expected\n");
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    unsigned char header[2];
    size_t in = fread(header, 1, 2, f);
    if (png_sig_cmp(header, 0, 2) || in != 2)
        exit(EXIT_FAILURE);

    png_structp png_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
            NULL, NULL);
    if (!png_p)
        exit(EXIT_FAILURE);

    png_infop info_p = png_create_info_struct(png_p);
    if (!info_p)
        exit(EXIT_FAILURE);

    png_infop end_info_p = png_create_info_struct(png_p);
    if (!end_info_p)
        exit(EXIT_FAILURE);

    if (setjmp(png_jmpbuf(png_p)))
        exit(EXIT_FAILURE);

    png_init_io(png_p, f);
    png_set_sig_bytes(png_p, 2);
    png_read_png(png_p, info_p, PNG_TRANSFORM_PACKING, NULL);

    png_uint_32 w, h;
    png_int_32 d, c;
    png_get_IHDR(png_p, info_p, &w, &h, &d, &c, NULL, NULL, NULL);

    if (w & 0xf) {
        printf("Image width must be multiple of 16\n");
        exit(EXIT_FAILURE);
    }

    if (c != PNG_COLOR_TYPE_PALETTE) {
        printf("Image must contain a palette\n");
        exit(EXIT_FAILURE);
    }

    png_bytep trans_ent_p;
    png_int_32 trans_size;
    png_color_16p trans_col_p;
    png_get_tRNS(png_p, info_p, &trans_ent_p, &trans_size, &trans_col_p); 

    printf("Number of transparency values: %d\n", trans_size);

    png_colorp pal_p;
    png_int_32 pal_size;
    png_get_PLTE(png_p, info_p, &pal_p, &pal_size);

    printf("Palette size: %d\n", pal_size);

    if (trans_size && trans_size != pal_size) {
        printf("Transparency values not for whole palette defined\n");
        exit(EXIT_FAILURE);
    }

    if (pal_size > 16) {
        pal_size = 16;
        printf("Limiting palette size to 16\n");
    }

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
            printf("Final palette size must be 16, 4, or 2\n");
            exit(EXIT_FAILURE);
    }

    printf("w: %u, h: %u, d: %d, c: %d\n", w, h, d, c);

    FILE *o = fopen("colors.pal", "w");
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    int min_sat = 255*255*3 + 1;
    for (int i = 0; i < pal_size; i++) {
        int sat;
        png_byte r, g, b;

        r = pal_p[i].red;
        g = pal_p[i].green;
        b = pal_p[i].blue;
        sat = r*r + g*g + b*b;

        if (sat < min_sat) {
            dark_idx = i;
            min_sat = sat;
        }
    }

    printf("Original darkest color index: %d\n", dark_idx);

    png_color temp_col = pal_p[0];
    pal_p[0] = pal_p[dark_idx];
    pal_p[dark_idx] = temp_col;

    int trans_idx = -1;
    if (trans_size) {
        png_byte temp_a = trans_ent_p[0];
        trans_ent_p[0] = trans_ent_p[dark_idx];
        trans_ent_p[dark_idx] = temp_a;

        for (int i = 0; i < trans_size; i++)
            if (!trans_ent_p[i]) {
                trans_idx = i;
                break;
            }

        printf("Transparent color index: %d\n", trans_idx);
    }

    int mask_idx = -1;
    for (int i = 0; i < pal_size; i++) {
        png_byte r, g, b;
        r = pal_p[i].red;
        g = pal_p[i].green;
        b = pal_p[i].blue;

        if (r == mask_col.red && g == mask_col.green && b == mask_col.blue)
            mask_idx = i;

        printf("%2d: r: %-3d  g: %-3d  b: %-3d", i, r, g, b);
        if (trans_size)
            printf(" a: %-3d\n", trans_ent_p[i]);
        else
            printf("\n");

        unsigned char buffer[2];
        buffer[0] = (int)limcol(roundf(r/255.0f*15.0f));
        buffer[1] = (int)limcol(roundf(b/255.0f*15.0f));
        buffer[1] |= (int)limcol(roundf(g/255.0f*15.0f)) << 4;
        if (fwrite(buffer, 1, 2, o) < 2) {
            printf("Write error\n");
            exit(EXIT_FAILURE);
        }
    }

    fclose(o);

    printf("Mask color index: %d\n", mask_idx);

    o = fopen("tileset.raw", "w");
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    png_bytep *row_pointers_p = png_get_rows(png_p, info_p);
    for (int i = 0; i < h; i++) {
        unsigned char buffer;

        for (int l = 0; l < d; l++)
            for (int j = 0; j < w; j += 8) {
                buffer = 0;
                for (int k = 0; k < 8; k++)
                    if (mapidx(*(row_pointers_p[i] + j + k)) & (1 << l))
                        buffer |= 1 << (7 - k);
                
                if (fwrite(&buffer, 1, 1, o) < 1) {
                    printf("Write error\n");
                    exit(EXIT_FAILURE);
                }
            }
    }

    // Add mask
    if (mask_idx >= 0 || trans_idx >= 0)
        for (int i = 0; i < h; i++) {
            unsigned char buffer;
            int pixel_idx;

            for (int l = 0; l < d; l++)
                for (int j = 0; j < w; j += 8) {
                    buffer = 0;
                    for (int k = 0; k < 8; k++) {
                        pixel_idx = mapidx(*(row_pointers_p[i] + j + k));
                        if (pixel_idx != mask_idx && pixel_idx != trans_idx)
                                buffer |= 1 << (7 - k);
                    }
                        
                    if (fwrite(&buffer, 1, 1, o) < 1) {
                        printf("Write error\n");
                        exit(EXIT_FAILURE);
                    }
                }
        }
    else
        printf("Mask data is omitted\n");

    fclose(o);
    exit(EXIT_SUCCESS);
}
