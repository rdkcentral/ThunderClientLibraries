/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TextureLoader.h"
#include <png.h>
#include <cstdio>
#include <cstring>

namespace Thunder {
namespace Compositor {
namespace Texture {

static void PngErrorFn(png_structp, png_const_charp msg) {
    fprintf(stderr, "[TextureLoader] PNG error: %s\n", msg);
}
static void PngWarnFn(png_structp, png_const_charp msg) {
    fprintf(stderr, "[TextureLoader] PNG warning: %s\n", msg);
}

PixelData LoadPNG(const std::string& filename) {
    PixelData result = {0, 0, 4, {}};

    // PNG loading implementation follows the transformation pipeline described in
    // the official libpng manual: http://www.libpng.org/pub/png/libpng-manual.txt
    // Section III.2 "Reading PNG files" - we normalize all input formats to RGBA8
    // for consistent texture handling across the compositor system.

    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) return result;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, PngErrorFn, PngWarnFn);
    if (!png) { fclose(fp); return result; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return result; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return result;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    result.width  = png_get_image_width(png, info);
    result.height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth  = png_get_bit_depth(png, info);

    // Apply libpng transformations to normalize any input PNG format to RGBA8 output.
    // This transformation chain handles: 16-bit depths, palette images, grayscale formats,
    // transparency chunks, and ensures consistent 4-byte-per-pixel RGBA layout.
    // Each transformation is applied conditionally based on the input format detected above.
    // See libpng-manual.txt Section III.2 for the standard transformation sequence.
    
    // Reduce 16-bit samples to 8-bit for embedded device memory efficiency
    if (bit_depth == 16) png_set_strip_16(png);
    
    // Expand palette-based images to full RGB for uniform processing
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    
    // Normalize low-bit-depth grayscale (1,2,4-bit) to 8-bit
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    
    // Convert transparency chunks (tRNS) to full alpha channel
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    
    // Add opaque alpha channel (0xFF) to RGB/Gray/Palette images without transparency
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    
    // Convert grayscale to RGB (grayscale and grayscale+alpha both handled)
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    result.data.resize(result.width * result.height * 4);
    result.bytes_per_pixel = 4;

    // Row pointers point direct naar buffer (rechtop!)
    png_bytep* row_pointers = new png_bytep[result.height];
    for (unsigned int y = 0; y < result.height; ++y) {
        row_pointers[y] = result.data.data() + y * result.width * 4;
    }

    png_read_image(png, row_pointers);
    delete[] row_pointers;

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    return result;
}

} // namespace Texture
} // namespace Compositor
} // namespace Thunder
