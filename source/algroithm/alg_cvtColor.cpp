#include "alg_cvtColor.h"
#include "stdint.h"

namespace alg_cv {

#define LIMIT_VAL_HIGH(val, max) (val) > (max) ? (max) : (val)
#define LIMIT_VAL_LOW(val, min) (val) < (min) ? (min) : (val)

void ALG_cvtColor(unsigned char* src, unsigned char* out, int w, int h, color_space_e color_space)
{
    if (!src || !out) {
        return;
    }
    int16_t y1, u, y2, v;

    if (color_space == YUV422_YUYV_2_RGB || color_space == YUV422_UYVY_2_RGB) {
        for (int index = 0; index < w * h / 2; ++index) {
            if (color_space == YUV422_YUYV_2_RGB) {
                y1 = src[index * 4 + 0];
                u  = src[index * 4 + 1];
                y2 = src[index * 4 + 2];
                v  = src[index * 4 + 3];
            } else if (color_space == YUV422_UYVY_2_RGB) {
                u  = src[index * 4 + 0];
                y1 = src[index * 4 + 1];
                v  = src[index * 4 + 2];
                y2 = src[index * 4 + 3];
            }

            unsigned char& o_r1 = out[index * 6 + 0];
            unsigned char& o_g1 = out[index * 6 + 1];
            unsigned char& o_b1 = out[index * 6 + 2];

            float r = 1.164 * (y1 - 16) + 1.596 * (v - 128);
            float g = 1.164 * (y1 - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
            float b = 1.164 * (y1 - 16) + 2.018 * (u -128);

            r = LIMIT_VAL_HIGH(r, 255);
            g = LIMIT_VAL_HIGH(g, 255);
            b = LIMIT_VAL_HIGH(b, 255);
            r = LIMIT_VAL_LOW(r, 0);
            g = LIMIT_VAL_LOW(g, 0);
            b = LIMIT_VAL_LOW(b, 0);

            o_r1 = r;
            o_g1 = g;
            o_b1 = b;


            unsigned char& o_r2 = out[index * 6 + 3];
            unsigned char& o_g2 = out[index * 6 + 4];
            unsigned char& o_b2 = out[index * 6 + 5];

            r = 1.164 * (y2 - 16) + 1.596 * (v - 128);
            g = 1.164 * (y2 - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
            b = 1.164 * (y2 - 16) + 2.018 * (u -128);

            r = LIMIT_VAL_HIGH(r, 255);
            g = LIMIT_VAL_HIGH(g, 255);
            b = LIMIT_VAL_HIGH(b, 255);
            r = LIMIT_VAL_LOW(r, 0);
            g = LIMIT_VAL_LOW(g, 0);
            b = LIMIT_VAL_LOW(b, 0);


            o_r2 = r;
            o_g2 = g;
            o_b2 = b;
        }
    }
}
}

