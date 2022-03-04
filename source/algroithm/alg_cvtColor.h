#ifndef _ALG_H_
#define _ALG_H_

namespace alg_cv {
    enum color_space_e {
        YUV422_YUYV_2_RGB = 0,
        YUV422_UYVY_2_RGB = 1,
    };
    void ALG_cvtColor(unsigned char* src, unsigned char* out, int w, int h, color_space_e color_space);
}


#endif
