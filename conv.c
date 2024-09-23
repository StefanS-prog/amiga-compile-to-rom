#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <math.h>

int dark_idx;

int mapidx(int idx) {
    if (!idx)
        return dark_idx;
    else if (idx == dark_idx)
        return 0;
    else
        return idx;
}    

float limcol(float v) {
    if (v > 15.0f)
        v = 15.0f;

    return v;
}

int main(int argc, char **argv) {
    int i, mr, mg, mb;

    mr = -1;
    if (argc == 6) {
        if (argv[1][0] != '-' || argv[1][1] != 't' || argv[1][2] != '\0')
            goto syn_error;

        if (sscanf(argv[2], "%3d,%3d,%3d", &mr, &mg, &mb)<3 || mr<0 || mg<0 || mb<0)
            goto syn_error;

        argv += 2;
    } else if (argc != 4) {
syn_error:
        fprintf(stderr, "Syntax: conv [-t r,g,b] input_image output_palette output_image\n");
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen(argv[1], "r"); // open PNG input image
    if (!f) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    unsigned char header[8];
    size_t in = fread(header, 1, 8, f);
    if (in != 8 || png_sig_cmp(header, 0, 8))
        exit(EXIT_FAILURE);

    png_structp png_p = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_p)
        exit(EXIT_FAILURE);

    png_infop info_p = png_create_info_struct(png_p);
    if (!info_p)
        exit(EXIT_FAILURE);

    if (setjmp(png_jmpbuf(png_p)))
        exit(EXIT_FAILURE);

    png_init_io(png_p, f);
    png_set_sig_bytes(png_p, 8);
    png_read_png(png_p, info_p, PNG_TRANSFORM_PACKING, NULL);

    png_uint_32 w, h;
    png_int_32 d, c;
    if (!png_get_IHDR(png_p, info_p, &w, &h, &d, &c, NULL, NULL, NULL)) {
        fprintf(stderr, "Image header could not be read\n");
        exit(EXIT_FAILURE);
    }

    if (c != PNG_COLOR_TYPE_PALETTE) {
        fprintf(stderr, "Image must contain a palette\n");
        exit(EXIT_FAILURE);
    }

    png_byte alphas[256] = { [0 ... 255] = 255};
    png_bytep trans_ent_p;
    int trans_size; // read transparency chunk
    if (!(png_get_tRNS(png_p, info_p, &trans_ent_p, &trans_size, NULL) & PNG_INFO_tRNS && trans_ent_p && trans_size > 0))
        trans_size = 0;

    for (i = 0; i < trans_size; i++)
        alphas[i] = trans_ent_p[i];

    png_colorp pal_p;
    int pal_size; // read palette chunk
    if (!(png_get_PLTE(png_p, info_p, &pal_p, &pal_size) & PNG_INFO_PLTE && pal_size > 0 && pal_p)) {
        fprintf(stderr, "Palette could not be read\n");
        exit(EXIT_FAILURE);
    } 

    fprintf(stderr, "Found palette of size %d with %d transparency value(s)\n", pal_size, trans_size);

    if (pal_size > 16) {
        pal_size = 16;
        fprintf(stderr, "Limiting palette size of 256 to 16\n");
    }

    switch (pal_size) { // compute color depth
        case 256:
            d = 8;
            break;

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
            fprintf(stderr, "Illegal palette size\n");
            exit(EXIT_FAILURE);
    }

    png_textp text_p;
    int text_size; // read text chunks
    if (png_get_text(png_p, info_p, &text_p, &text_size) && text_p && text_size > 0) {
        FILE *t = fopen("tiletypes.h", "w");
        if (!t) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < text_size; i++)
            if (fprintf(t, "#define %s\n", (char *)text_p[i].text) < 0) {
                fprintf(stderr, "Tile types write error\n");
                exit(EXIT_FAILURE);
            }

        fclose(t);
    }

    FILE *o = fopen(argv[2], "w"); // open palette file for output
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    long sat;
    long min_sat = 255*255*3 + 1;
    for (i = 0; i < pal_size; i++) { // search for darkest color
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

    png_color temp_col = pal_p[0]; // swap darkest color to index zero
    pal_p[0] = pal_p[dark_idx];
    pal_p[dark_idx] = temp_col;

    png_byte temp_a = alphas[0];
    alphas[0] = alphas[dark_idx];
    alphas[dark_idx] = temp_a;

    int trans_idx = -1;
    for (i = 0; i < trans_size; i++) // check for color with zero alpha
        if (!alphas[i]) {
            trans_idx = i;
            break;
        }

    if (trans_idx >= 0)
        fprintf(stderr, "Treating index %d as transparent\n", trans_idx);

    int mask_idx = -1; // color mask = 0 for transparency => background visible
    for (i = 0; i < pal_size; i++) {
        png_byte r, g, b;
        r = pal_p[i].red;
        g = pal_p[i].green;
        b = pal_p[i].blue;

        if (r == mr && g == mg && b == mb)
            mask_idx = i;

        fprintf(stderr, "| %2d: r: %-3d  g: %-3d  b: %-3d  a: %-3d\n", i, r, g, b, alphas[i]);

        unsigned char buffer[2];
        buffer[0] = (int)limcol(roundf(r/255.0f*15.0f));
        buffer[1] = (int)limcol(roundf(b/255.0f*15.0f));
        buffer[1] |= (int)limcol(roundf(g/255.0f*15.0f)) << 4;
        if (fwrite(buffer, 1, 2, o) < 2) {
            fprintf(stderr, "Palette write error\n");
            exit(EXIT_FAILURE);
        }
    }

    fclose(o);
    if (mask_idx >= 0)
        fprintf(stderr, "Treating index %d as transparent\n", mask_idx);

    o = fopen(argv[3], "w"); // open image file for output of raw data
    if (!o) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    png_bytep *row_pointers_p = png_get_rows(png_p, info_p);
    for (i = 0; i < h; i++) {
        unsigned char buffer;

        for (int l = 0; l < d; l++)
            for (int j = 0; j < w; j += 8) {
                buffer = 0;
                for (int k = 0; k < 8; k++)
                    if (mapidx(*(row_pointers_p[i] + j + k)) & (1 << l))
                        buffer |= 1 << (7 - k);
                
                if (fwrite(&buffer, 1, 1, o) < 1) {
                    fprintf(stderr, "Image write error\n");
                    exit(EXIT_FAILURE);
                }
            }
    }

    if (mask_idx >= 0 || trans_idx >= 0) // Add mask
        for (i = 0; i < h; i++) {
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
                        fprintf(stderr, "Image write error\n");
                        exit(EXIT_FAILURE);
                    }
                }
        }
    else
        fprintf(stderr, "No mask data for transparency has been written\n");

    fclose(o);
    exit(EXIT_SUCCESS);
}
