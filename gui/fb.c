// --------------------------------------------------------------------------------------
// Copyright (c) 2025-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: fb.c - Framebuffer routines
// --------------------------------------------------------------------------------------

#include <gui.h>

static surface_st _gui_fb_vram_surface = { 0 };
global surface_st *gui_fb_vram_surface = &_gui_fb_vram_surface;
global uint8_t gui_fb_bpp;

#if !VGA_MODE_12H
static uint8_t gui_fb_pixels[GUI_WIDTH * GUI_HEIGHT] __attribute__((aligned(16)));
static surface_st gui_fb_surface = {
    .size = {
        .width = GUI_WIDTH,
        .height = GUI_HEIGHT,
    },
    .pitch = GUI_WIDTH,
    .pixels = gui_fb_pixels,
};
#endif

static rect_st dirty_rect = { 0 };

global void
gui_fb_draw_start(void)
{
}

global void
gui_fb_draw_end(void)
{
}

global void
gui_fb_mark_dirty(rect_st rect)
{
    static rect_st screen_rect = { .width = GUI_WIDTH, .height = GUI_HEIGHT };
    dirty_rect = gui_rect_clip(gui_rect_enclose(dirty_rect, rect), screen_rect);
}

global void
gui_fb_draw_rect(rect_st rect, uint8_t color)
{
#if VGA_MODE_12H
    gui_planar_draw_rect(rect, color);
#else
    gui_surface_draw_rect(&gui_fb_surface, rect, color);
#endif

    gui_fb_mark_dirty(rect);
}

global void
gui_fb_draw_bitmap(rect_st rect, bitmap_st *bitmap)
{
#if VGA_MODE_12H
    (void)rect;
    (void)bitmap;
#else
    ASSERT(bitmap->size.width == GUI_WIDTH && bitmap->size.height == GUI_HEIGHT);

    for (uint16_t i = 0; i < rect.height; ++i) {
        size_t fb_off = (rect.y + i) * gui_fb_surface.pitch + rect.x;
        size_t bm_off = (rect.y + i) * bitmap->pitch + rect.x;
        memcpy(&gui_fb_surface.pixels[fb_off], &bitmap->pixels[bm_off], rect.width);
    }

    gui_fb_mark_dirty(rect);
#endif
}

global void
gui_fb_draw_pattern(rect_st rect, bitmap_st *pattern, uint8_t c1, uint8_t c2)
{
#if VGA_MODE_12H
    gui_planar_draw_pattern_abs(rect, pattern, c1, c2);
#else
    gui_surface_draw_pattern_abs(&gui_fb_surface, rect, pattern, c1, c2);
#endif

    gui_fb_mark_dirty(rect);
}

global void
gui_fb_draw_surface(int dst_x, int dst_y, surface_st *src_sf, rect_st src_rect)
{
#if VGA_MODE_12H
    gui_planar_draw_surface(dst_x, dst_y, src_sf, src_rect);
#else
    gui_surface_copy(&gui_fb_surface, dst_x, dst_y, src_sf, src_rect);
#endif

    gui_fb_mark_dirty(gui_rect_make(dst_x, dst_y, src_rect.width, src_rect.height));
}

global void
gui_fb_draw_outline(rect_st rect)
{
#if VGA_MODE_12H
    gui_planar_xor_corners(rect);
#else
    gui_surface_draw_border(gui_fb_vram_surface, rect, COLOR_BORDER);
#endif
}

global void
gui_fb_flush(void)
{
    if (gui_rect_is_empty(dirty_rect)) {
        return;
    }

    gui_drag_clear_outline();

    rect_st rect = dirty_rect;
    dirty_rect = (rect_st) { 0 };

#if VGA_MODE_12H
    gui_planar_flush(rect);
#else
    gui_surface_copy(gui_fb_vram_surface, rect.x, rect.y, &gui_fb_surface, rect);
#endif

    gui_pointer_draw();
    gui_drag_draw_outline();
}

global void
gui_fb_init(void)
{
    gui_fb_vram_surface->size.width = GUI_WIDTH;
    gui_fb_vram_surface->size.height = GUI_HEIGHT;

#if VGA_MODE_12H
    gui_fb_vram_surface->pitch = GUI_WIDTH / 8;
    gui_fb_vram_surface->pixels = (uint8_t *)0xA0000;
    gui_fb_bpp = 4;
#else
    gui_fb_vram_surface->pitch = krn_core_mboot_info->fb_pitch;
    gui_fb_vram_surface->pixels = krn_core_mboot_info->fb_addr;
    gui_fb_bpp = krn_core_mboot_info->fb_bpp;
#endif
}
