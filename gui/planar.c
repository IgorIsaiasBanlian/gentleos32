/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: planar.c - Support for VGA planar mode
 */

#include <gui.h>

enum {
    FB_WIDTH = 640,
    FB_HEIGHT = 480,
    FB_PITCH = FB_WIDTH / 8,
    FB_PLANE_SIZE = FB_HEIGHT * FB_PITCH,
};

static uint8_t (*gui_planar_pixels)[FB_PLANE_SIZE];

/*
 * Map of colors to their 4 planar bits.
 * Store 256 values instead of 16 so it can be safely
 * indexed by any byte value without error checking
 */
static uint32_t gui_planar_lut[256];

global void
gui_planar_flush(rect_st rect)
{
    int x0 = (rect.x / 8) * 8;
    int x1 = ((rect.x + rect.width + 7) / 8) * 8;
    int byte_x0 = x0 / 8;
    int byte_count = (x1 - x0) / 8;

    for (int plane = 0; plane < 4; ++plane) {
        krn_vga_set_write_planes(1 << plane);

        for (int y = rect.y; y < rect.y + rect.height; ++y) {
            memcpy(
                gui_fb_vram_surface->pixels + y * gui_fb_vram_surface->pitch + byte_x0,
                gui_planar_pixels[plane] + y * FB_PITCH + byte_x0,
                byte_count
            );
        }
    }
}

global void
gui_planar_draw_rect(rect_st rect, uint8_t color)
{
    int l_x = rect.x;
    int r_x = rect.x + rect.width - 1;

    if (r_x < l_x) {
        return;
    }

    int l_byte = l_x / 8;
    int r_byte = r_x / 8;

    uint8_t l_mask = 0xFF >> (l_x & 7);
    uint8_t r_mask = 0xFF << (7 - (r_x & 7));

    for (int plane = 0; plane < 4; ++plane) {
        uint8_t fill = ((color >> plane) & 1) ? 0xFF : 0x00;
        uint8_t *dst_plane = gui_planar_pixels[plane];

        for (int y = rect.y; y < rect.y + rect.height; ++y) {
            uint8_t *dst_row = dst_plane + y * FB_PITCH;

            if (l_byte == r_byte) {
                uint8_t mask = l_mask & r_mask;
                dst_row[l_byte] = (dst_row[l_byte] & ~mask) | (fill & mask);
                continue;
            }

            dst_row[l_byte] = (dst_row[l_byte] & ~l_mask) | (fill & l_mask);

            if (r_byte > l_byte + 1) {
                memset(dst_row + l_byte + 1, fill, r_byte - l_byte - 1);
            }

            dst_row[r_byte] = (dst_row[r_byte] & ~r_mask) | (fill & r_mask);
        }
    }
}

global void
gui_planar_draw_pattern_abs(rect_st dst_rect, bitmap_st *pattern, uint8_t c1, uint8_t c2)
{
    int plane, y, l_x, r_x, l_byte, r_byte, byte;
    int pat_h, pat_bytes, pat_byte;
    uint8_t mask, l_mask, r_mask, c1_mask, c2_mask;
    uint8_t *dst_plane, *dst_row, val;
    const uint8_t *pat_row;

    /* Pattern bytes must align with plane bytes */
    ASSERT(pattern->size.width % 8 == 0);

    l_x = dst_rect.x;
    r_x = dst_rect.x + dst_rect.width - 1;

    if (r_x < l_x) {
        return;
    }

    l_byte = l_x / 8;
    r_byte = r_x / 8;

    l_mask = 0xFF >> (l_x & 7);
    r_mask = 0xFF << (7 - (r_x & 7));

    pat_bytes = pattern->size.width / 8;
    pat_h = pattern->size.height;

    for (plane = 0; plane < 4; ++plane) {
        c1_mask = ((c1 >> plane) & 1) ? 0xFF : 0x00;
        c2_mask = ((c2 >> plane) & 1) ? 0xFF : 0x00;
        dst_plane = gui_planar_pixels[plane];

        for (y = dst_rect.y; y < dst_rect.y + dst_rect.height; ++y) {
            pat_row = pattern->pixels + (y % pat_h) * pattern->pitch;
            dst_row = dst_plane + y * FB_PITCH;

            pat_byte = l_byte % pat_bytes;
            val = (pat_row[pat_byte] & c1_mask) | (~pat_row[pat_byte] & c2_mask);
            if (l_byte == r_byte) {
                mask = l_mask & r_mask;
                dst_row[l_byte] = (dst_row[l_byte] & ~mask) | (val & mask);
                continue;
            }
            dst_row[l_byte] = (dst_row[l_byte] & ~l_mask) | (val & l_mask);

            for (byte = l_byte + 1; byte < r_byte; ++byte) {
                /* Avoid modulo in the hot path */
                pat_byte = (pat_byte + 1 == pat_bytes) ? 0 : (pat_byte + 1);
                val = (pat_row[pat_byte] & c1_mask) | (~pat_row[pat_byte] & c2_mask);
                dst_row[byte] = val;
            }

            pat_byte = r_byte % pat_bytes;
            val = (pat_row[pat_byte] & c1_mask) | (~pat_row[pat_byte] & c2_mask);
            dst_row[r_byte] = (dst_row[r_byte] & ~r_mask) | (val & r_mask);
        }
    }
}

global void
gui_planar_draw_surface(int dst_x, int dst_y, surface_st *src, rect_st src_rect)
{
    if (src_rect.width <= 0 || src_rect.height <= 0) {
        return;
    }

    uint8_t (*dst)[FB_PLANE_SIZE] = (uint8_t (*)[FB_PLANE_SIZE])gui_planar_pixels;

    int dst_l_x = dst_x;
    int dst_r_x = dst_x + src_rect.width - 1;

    int dst_l_byte = dst_l_x / 8;
    int dst_r_byte = dst_r_x / 8;

    int dst_l_full_byte = (dst_l_x + 7) / 8;
    int dst_r_full_byte = (dst_r_x + 1) / 8;

    uint8_t dst_l_mask = 0xFF >> (dst_l_x & 7);
    uint8_t dst_r_mask = 0xFF << (7 - (dst_r_x & 7));

    for (int row = 0; row < src_rect.height; ++row) {
        uint8_t *src_row = src->pixels + (src_rect.y + row) * src->pitch + src_rect.x;
        int dst_row_ofs = (dst_y + row) * FB_PITCH;

        if (dst_l_byte == dst_r_byte) {
            uint8_t dst_mask = dst_l_mask & dst_r_mask;
            int dst_ofs = dst_row_ofs + dst_l_byte;
            uint32_t acc = 0;

            for (int bit = 0; bit < 8; ++bit) {
                int src_x = dst_l_byte * 8 - dst_x + bit;

                acc <<= 1;

                if (src_x >= 0 && src_x < src_rect.width) {
                    acc |= gui_planar_lut[src_row[src_x]];
                }
            }

            for (int i = 0; i < 4; ++i) {
                dst[i][dst_ofs] = (dst[i][dst_ofs] & ~dst_mask) | ((acc >> (i * 8)) & 0xFF);
            }

            continue;
        }

        if (dst_l_byte < dst_l_full_byte) {
            int dst_ofs = dst_row_ofs + dst_l_byte;
            uint32_t acc = 0;

            for (int bit = 0; bit < 8; ++bit) {
                int src_x = dst_l_byte * 8 - dst_x + bit;

                acc <<= 1;

                if (src_x >= 0) {
                    acc |= gui_planar_lut[src_row[src_x]];
                }
            }

            for (int i = 0; i < 4; ++i) {
                dst[i][dst_ofs] = (dst[i][dst_ofs] & ~dst_l_mask) | ((acc >> (i * 8)) & 0xFF);
            }
        }

        for (int x = dst_l_full_byte; x < dst_r_full_byte; ++x) {
            uint8_t *sp = src_row + (x * 8 - dst_x);
            int dst_ofs = dst_row_ofs + x;
            uint32_t acc;

            /* Loops are unrolled for performance in this most busy section */

            acc = gui_planar_lut[sp[0]];
            acc = (acc << 1) | gui_planar_lut[sp[1]];
            acc = (acc << 1) | gui_planar_lut[sp[2]];
            acc = (acc << 1) | gui_planar_lut[sp[3]];
            acc = (acc << 1) | gui_planar_lut[sp[4]];
            acc = (acc << 1) | gui_planar_lut[sp[5]];
            acc = (acc << 1) | gui_planar_lut[sp[6]];
            acc = (acc << 1) | gui_planar_lut[sp[7]];

            dst[0][dst_ofs] = acc & 0xFF;
            dst[1][dst_ofs] = (acc >> 8) & 0xFF;
            dst[2][dst_ofs] = (acc >> 16) & 0xFF;
            dst[3][dst_ofs] = acc >> 24;
        }

        if (dst_r_byte >= dst_r_full_byte) {
            int dst_ofs = dst_row_ofs + dst_r_byte;
            uint32_t acc = 0;

            for (int bit = 0; bit < 8; ++bit) {
                int src_x = dst_r_byte * 8 - dst_x + bit;

                acc <<= 1;

                if (src_x < src_rect.width) {
                    acc |= gui_planar_lut[src_row[src_x]];
                }
            }

            for (int i = 0; i < 4; ++i) {
                dst[i][dst_ofs] = (dst[i][dst_ofs] & ~dst_r_mask) | ((acc >> (i * 8)) & 0xFF);
            }
        }
    }
}

global void
gui_planar_draw_pointer(int dst_x, int dst_y)
{
    bitmap_st *bitmap = &bitmap_pointer;
    uint8_t alpha = bitmap->alpha;

    int bmp_w = bitmap->size.width;
    int bmp_h = bitmap->size.height;

    if (dst_x + bmp_w > FB_WIDTH) {
        bmp_w = FB_WIDTH - dst_x;
    }

    if (dst_y + bmp_h > FB_HEIGHT) {
        bmp_h = FB_HEIGHT - dst_y;
    }

    if (bmp_w <= 0 || bmp_h <= 0) {
        return;
    }

    int dst_l_x = (dst_x / 8) * 8;
    int dst_r_x = ((dst_x + bmp_w + 7) / 8) * 8;

    int dst_l_byte = dst_l_x / 8;
    int dst_byte_count = (dst_r_x - dst_l_x) / 8;

    for (int plane = 0; plane < 4; ++plane) {
        krn_vga_set_write_planes(1 << plane);

        for (int row = 0; row < bmp_h; ++row) {
            int y = dst_y + row;

            uint8_t *dst = gui_fb_vram_surface->pixels + y * FB_PITCH + dst_l_byte;

            for (int byte = 0; byte < dst_byte_count; ++byte) {
                uint8_t val = 0;

                for (int bit = 0; bit < 8; ++bit) {
                    int px_x = dst_l_x + byte * 8 + bit;
                    int bmp_x = px_x - dst_x;

                    if (bmp_x >= 0 && bmp_x < bmp_w &&
                        bitmap->pixels[row * bitmap->pitch + bmp_x] != alpha) {

                        uint8_t c = bitmap->pixels[row * bitmap->pitch + bmp_x];
                        val |= ((c >> plane) & 1) << (7 - bit);
                    } else {
                        int byte_ofs = y * FB_PITCH + px_x / 8;
                        int bit_pos = 7 - (px_x & 7);
                        uint8_t c = (gui_planar_pixels[plane][byte_ofs] >> bit_pos) & 1;
                        val |= c << (7 - bit);
                    }
                }

                dst[byte] = val;
            }
        }
    }
}

/* This must be only be called from gui_planar_xor_corners() */
static void
gui_planar_xor_h_seg(uint8_t *vram, int x, int y, int w)
{
    if (w <= 0) {
        return;
    }

    int l_x = x;
    int r_x = x + w - 1;
    int l_byte = l_x / 8;
    int r_byte = r_x / 8;

    uint8_t l_mask = 0xFF >> (l_x & 7);
    uint8_t r_mask = 0xFF << (7 - (r_x & 7));

    uint8_t *row = vram + y * FB_PITCH;

    if (l_byte == r_byte) {
        krn_vga_set_bit_mask(l_mask & r_mask);
        krn_vga_latch_write(&row[l_byte], 0xFF);
        return;
    }

    krn_vga_set_bit_mask(l_mask);
    krn_vga_latch_write(&row[l_byte], 0xFF);

    if (r_byte > l_byte + 1) {
        krn_vga_set_bit_mask(0xFF);
        for (int b = l_byte + 1; b < r_byte; ++b) {
            krn_vga_latch_write(&row[b], 0xFF);
        }
    }

    krn_vga_set_bit_mask(r_mask);
    krn_vga_latch_write(&row[r_byte], 0xFF);
}

/* This must be only be called from gui_planar_xor_corners() */
static void
gui_planar_xor_v_seg(uint8_t *vram, int x, int y, int h)
{
    if (h <= 0) {
        return;
    }

    int x_byte = x / 8;
    uint8_t mask = 0x80 >> (x & 7);

    krn_vga_set_bit_mask(mask);

    for (int row = 0; row < h; ++row) {
        krn_vga_latch_write(vram + (y + row) * FB_PITCH + x_byte, 0xFF);
    }
}

global void
gui_planar_xor_corners(rect_st rect)
{
    uint8_t *vram = gui_fb_vram_surface->pixels;
    const int arm = 16;

    int l_x = rect.x;
    int t_y = rect.y;
    int r_x = rect.x + rect.width - 1;
    int b_y = rect.y + rect.height - 1;

    krn_vga_set_write_planes(0x0F);
    krn_vga_set_logic_op(0x18);

    /* Top-left */
    gui_planar_xor_h_seg(vram, l_x, t_y, arm);
    gui_planar_xor_v_seg(vram, l_x, t_y + 1, arm - 1);

    /* Top-right */
    gui_planar_xor_h_seg(vram, r_x - arm + 1, t_y, arm);
    gui_planar_xor_v_seg(vram, r_x, t_y + 1, arm - 1);

    /* Bottom-left */
    gui_planar_xor_h_seg(vram, l_x, b_y, arm);
    gui_planar_xor_v_seg(vram, l_x, b_y - arm + 1, arm - 1);

    /* Bottom-right */
    gui_planar_xor_h_seg(vram, r_x - arm + 1, b_y, arm);
    gui_planar_xor_v_seg(vram, r_x, b_y - arm + 1, arm - 1);

    krn_vga_set_logic_op(0x00);
    krn_vga_set_bit_mask(0xFF);
}

global void
gui_planar_init(void)
{
    int c;

    if (!krn_system_info.fb_planar) {
        return;
    }

    gui_planar_pixels = heap_alloc(4 * FB_PLANE_SIZE, "Planar pixels", 1);

    for (c = 0; c < 256; ++c) {
        gui_planar_lut[c] = 0
            | (((c >> 0) & 1) << 0)
            | (((c >> 1) & 1) << 8)
            | (((c >> 2) & 1) << 16)
            | (((c >> 3) & 1) << 24);
    }
}
