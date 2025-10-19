#include <unistd.h>
#include <stdio.h>
#include "argparse.h"
#include "misc_util.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

static const char *const usages[] = {
    "font_gen [options] ttf_filename",
    NULL,
};

static const char *get_filename(const char *fname) {
    const size_t len = strlen(fname);
    int i = (int)len-2;
    while (i >= 0) {
        if (fname[i] == '/') {
            i++;
            const char *nfname = fname + i;
            return nfname;
        }
        i--;
    }
    return fname;
}

static char *filename_with_ext(const char *fname, const char *ext) {
    const size_t len = strlen(fname);
    int i = (int)len-2;
    while (i >= 0) {
        if (fname[i] == '.') {
            char *tfname = (char *)malloc(len+strlen(ext)+1);
            char *nfname = (char *)malloc(len+strlen(ext)+1);
            strcpy(tfname, fname);
            tfname[i] = '\0';
            sprintf(nfname, "%s.%s", tfname, ext);
            free(tfname);
            return nfname;
        }
        i--;
    }
    char *nfname = (char *)malloc(len+strlen(ext)+1);
    sprintf(nfname, "%s.%s", fname, ext);
    return nfname;
}

static char *filename_without_ext(const char *fname) {
    const size_t len = strlen(fname);
    int i = (int)len-2;
    while (i >= 0) {
        if (fname[i] == '.') {
            char *tfname = (char *)malloc(len+1);
            strcpy(tfname, fname);
            tfname[i] = '\0';
            return tfname;
        }
        i--;
    }
    char *nfname = (char *)malloc(len+1);
    sprintf(nfname, "%s", fname);
    return nfname;
}

static float find_scale_for_target_ascent(const stbtt_fontinfo *fontinfo, int target_ascent, float *dst_size) {
    int ascent, descent, line_gap;

    float closest_font_size = 0;
    float closest_diff = 0.0f;
    float last_diff = 0.0f;
    float font_size = (float)target_ascent;

    bool first = true;
    while (font_size < (float)(2*target_ascent)) {
        float scale = stbtt_ScaleForPixelHeight(fontinfo, (float)font_size);
        stbtt_GetFontVMetrics(fontinfo, &ascent, &descent, &line_gap);
        float scaled_ascent = (float)ascent * scale;
        float diff = fabsf((float)target_ascent - scaled_ascent);
        if (first || diff < closest_diff) {
            closest_diff = diff;
            closest_font_size = font_size;
        }
        // break if the diff starts increasing
        if (!first && diff > last_diff) {
            break;
        }
        last_diff = diff;
        font_size+=0.1f;
        first = false;
    }
    *dst_size = closest_font_size;
    return stbtt_ScaleForPixelHeight(fontinfo, (float)closest_font_size);
}

int main(int argc, const char **argv) {
    float font_size = 0;
    int ascent_target = 0;
    int image_width = 0;
    int image_height = 0;
    bool output_image = false;
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Options"),
        OPT_FLOAT('s', "size", &font_size, "pixel height of font", NULL, 0, 0),
        OPT_INTEGER('a', "ascent", &ascent_target, "find a font size closest to this ascent", NULL, 0, 0),
        OPT_INTEGER('x', "x", &image_width, "width of the atlas", NULL, 0, 0),
        OPT_INTEGER('y', "y", &image_height, "height of the atlas", NULL, 0, 0),
        OPT_BOOLEAN('i', "image", &output_image, "output png binary data in c file", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nGenerates a font texture and clover font def from a TTF file.", "\n");
    argc = argparse_parse(&argparse, argc, argv);

    if (font_size == 0 && ascent_target == 0) {
        printf("need to specify either font size or acent target\n");
        argparse_usage(&argparse);
        return 1;
    }

    if (image_width == 0) {
        printf("atlas width is zero or not specified\n");
        argparse_usage(&argparse);
        return 1;
    }

    if (image_height == 0) {
        image_height = image_width;
    }

    if (argc == 0) {
        printf("font name not specified\n");
        argparse_usage(&argparse);
        return 1;
    }

    const char *filename = argv[0];
    if (!file_exists(filename)) {
        printf("file does not exist\n");
        return 1;
    }

    unsigned char *data = load_file(filename);
    stbtt_fontinfo fontinfo;
    const int ret = stbtt_InitFont(&fontinfo, data, 0);
    if (!ret) {
        printf("bad font\n");
        return 1;
    }

    const char *just_filename = get_filename(filename);
    char *pngfname = filename_with_ext(just_filename, "png");
    char *cfname = filename_with_ext(just_filename, "c");
    char *varname = filename_without_ext(just_filename);

    FILE *outfile = fopen(cfname, "w");
    if (!outfile) {
        printf("unable to open output file\n");
        return 1;
    }

    float scale;
    if (ascent_target == 0) {
        scale = stbtt_ScaleForPixelHeight(&fontinfo, font_size);
    } else {
        scale = find_scale_for_target_ascent(&fontinfo, ascent_target, &font_size);
    }

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &line_gap);
    //printf("metrics: ascent=%d, descent=%d, line_gap=%d\n", ascent, descent, line_gap);

    int num_chars = 95;
    unsigned char *pixels = (unsigned char *)malloc(image_width * image_height * sizeof(unsigned char));
    stbtt_bakedchar *chardata = (stbtt_bakedchar *)malloc(96 * sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(data, 0, (float)font_size, pixels, image_width, image_height, 32, num_chars, chardata);

    unsigned char *px4 = (unsigned char *)malloc(image_width * image_height * 4 * sizeof(unsigned char));
    int ii = 0;
    for (int i=0; i<(image_width*image_height); i++) {
        px4[ii] = 0xFF;
        px4[ii+1] = 0xFF;
        px4[ii+2] = 0xFF;
        px4[ii+3] = pixels[i];
        ii+=4;
    }
    int out_len;
    unsigned char *out_img = stbi_write_png_to_mem(px4, image_width*4, image_width, image_height, 4, &out_len);
    if (!out_img) {
        printf("error writing png file\n");
        return 1;
    }

    fprintf(outfile, "#include \"clover.h\"\n\n");
    fprintf(outfile, "cl_font_def %s = {\n", varname);
    fprintf(outfile, "\t.font_size = %d,\n", (int)roundf(font_size));
    fprintf(outfile, "\t.ascent = %d,\n", (int)roundf((float)ascent * scale));
    fprintf(outfile, "\t.descent = %d,\n", (int)roundf((float)descent * scale));
    fprintf(outfile, "\t.min_id = 32,\n");
    fprintf(outfile, "\t.num_chars = 95,\n");
    fprintf(outfile, "\t.chars = {\n");

    for (int i=0; i<num_chars; i++) {
        char c = (char)i+32;
        char cs[20];
        if (c == '\'' || c == '\\') {
            sprintf(cs, "'\\%c'", c);
        } else {
            sprintf(cs, "'%c'", c);
        }
        fprintf(outfile, "\t\t{ .c=%s, .x=%d, .y=%d, .w=%d, .h=%d, .xoff=%0.1ff, .yoff=%0.1ff, .xadv=%0.1ff },\n", cs, chardata[i].x0, chardata[i].y0, chardata[i].x1-chardata[i].x0, chardata[i].y1-chardata[i].y0, chardata[i].xoff, chardata[i].yoff, chardata[i].xadvance);
    }
    fprintf(outfile, "\t},\n");

    fprintf(outfile, "\t.kerns = {\n");
    int num_kerns = 0;
    for (int c1=32; c1<(32+95); c1++) {
        for (int c2=32; c2<(32+95); c2++) {
            if (c1 != c2) {
                int kern = stbtt_GetCodepointKernAdvance(&fontinfo, c1, c2);
                if (kern != 0) {
                    num_kerns++;
                    fprintf(outfile, "\t\t{ .first=%d, .second=%d, .amount=%0.4ff },\n", c1, c2, (float)kern*scale);
                }
            }
        }
    }
    fprintf(outfile, "\t},\n");
    fprintf(outfile, "\t.num_kerns=%d,\n", num_kerns);
    fprintf(outfile, "};\n");

    if (output_image) {
        fprintf(outfile, "\n");
        fprintf(outfile, "unsigned char  %s_png[] = {\n", varname);
        for (int i=0; i<out_len; i++) {
            if ((i % 16) == 0) {
                fprintf(outfile, "\t");
            }
            fprintf(outfile, "0x%02x,", out_img[i]);
            if (((i+1) % 16) == 0) {
                fprintf(outfile, "\n");
            } else {
                fprintf(outfile, " ");
            }
        }
        if ((out_len % 16) != 0) {
            fprintf(outfile, "\n");
        }
        fprintf(outfile, "};\n\n");
        fprintf(outfile, "unsigned int %s_png_len = %d;\n", varname, out_len);
    }

    FILE *pngout = fopen(pngfname, "w");
    fwrite(out_img, 1, out_len, pngout);
    fclose(pngout);

    fclose(outfile);
    free(data);
    free(px4);
    free(pixels);
    free(pngfname);
    free(cfname);
    free(varname);
    return 0;
}