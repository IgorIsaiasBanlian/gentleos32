#define VESA_WIDTH 800
#define VESA_HEIGHT 600

// Use VGA 640x480x16 planar mode
// Enable if VESA modes are unavailable
#define VGA_MODE_12H 0

// Available themes:
// - GUI_THEME_DEFAULT
// - GUI_THEME_MONO
// - GUI_THEME_NEON
#define GUI_THEME GUI_THEME_DEFAULT

// To set a wallpaper, save an image using GIMP as an ASCII-formatted PPM.
// It must exactly match the screen resolution and only use colors
// from misc/vga-256.gpl palette.
// The wallpaper is only supported when VGA_MODE_12H is not set.
// #define WALLPAPER_PATH "tmp/wall.ppm"

// Available modes for COM1 serial port:
// - UART_MODE_NONE  - Unused
// - UART_MODE_MOUSE - MS mouse driver
// - UART_MODE_DEBUG - Debug output (8N1/9600)
#define UART_MODE UART_MODE_NONE

// Enable to print keyboard debug information
#define DEBUG_KEYBOARD 0
