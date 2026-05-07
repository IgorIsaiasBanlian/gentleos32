// --------------------------------------------------------------------------------------
// Copyright (c) 2025-2026 luke8086
// Distributed under the terms of GPL-2 License
// --------------------------------------------------------------------------------------
// File: pointer.c - Mouse pointer
// --------------------------------------------------------------------------------------

#include <gui.h>


global bitmap_st bitmap_pointer = {
    .size = { .width = 11, .height = 15 },
    .bpp = 8,
    .pitch = 11,
    .foreground = 0x00,
    .alpha = 0x56,
    .pixels = (uint8_t *)
        "\x00\x56\x56\x56\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x00\x56\x56\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x0f\x00\x56\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x0f\x0f\x00\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x0f\x0f\x0f\x00\x56\x56\x56\x56\x56\x56" \
        "\x00\x0f\x0f\x0f\x0f\x00\x56\x56\x56\x56\x56" \
        "\x00\x0f\x0f\x0f\x0f\x0f\x00\x56\x56\x56\x56" \
        "\x00\x0f\x0f\x0f\x0f\x0f\x0f\x00\x56\x56\x56" \
        "\x00\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x00\x56\x56" \
        "\x00\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x00\x56" \
        "\x00\x0f\x0f\x0f\x00\x00\x00\x00\x00\x00\x00" \
        "\x00\x0f\x0f\x00\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x0f\x00\x56\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x00\x56\x56\x56\x56\x56\x56\x56\x56\x56" \
        "\x00\x56\x56\x56\x56\x56\x56\x56\x56\x56\x56" \
};

static rect_st gui_pointer_rect;

global void
gui_pointer_draw(void)
{
#if VGA_MODE_12H
    gui_planar_draw_pointer(gui_pointer_rect.x, gui_pointer_rect.y);
#else
    gui_surface_draw_bitmap(gui_fb_vram_surface, gui_pointer_rect.x, gui_pointer_rect.y,
        &bitmap_pointer, 0);
#endif
}

global void
gui_pointer_move(uint16_t x, uint16_t y)
{
    gui_fb_mark_dirty(gui_pointer_rect);

    gui_pointer_rect.x = x;
    gui_pointer_rect.y = y;
}

global void
gui_pointer_init(void)
{
    gui_pointer_rect.x = GUI_WIDTH / 2,
    gui_pointer_rect.y = GUI_HEIGHT / 2,
    gui_pointer_rect.width = bitmap_pointer.size.width;
    gui_pointer_rect.height = bitmap_pointer.size.height;
}
