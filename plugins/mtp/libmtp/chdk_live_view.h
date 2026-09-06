#ifndef __LIVE_VIEW_H
#define __LIVE_VIEW_H

// Note: used in modules and platform independent code. 
// Do not add platform dependent stuff in here (#ifdef/#endif compile options or camera dependent values)

/*
Protocol notes:
- Unless otherwise specified, all structure values are packed in camera native (little
  endian) byte order
- Frame buffer and palette data are in native camera formats
  Some documentation may be found at http://chdk.wikia.com/wiki/Frame_buffers 
- The frame buffer descriptions returned may not be correct depending on the
  camera model and various camera settings (shooting mode, digital zoom, aspect ratio)
  This may result in partial images, garbage in the "valid" area or incorrect position
- In some cases, the requested data may not be available. If this happens, the framebuffer
  or palette data offset will be zero. 
- The frame buffer descriptions are returned regardless of whether the data is available
- New enum values (e.g. aspect ratio, framebuffer type, palette type) may be added in minor
  versions.
*/
// Live View protocol version
#define LIVE_VIEW_VERSION_MAJOR 2  // increase only with backwards incompatible changes (and reset minor)
#define LIVE_VIEW_VERSION_MINOR 2  // increase with extensions of functionality

/*
protocol version history
< 2.0 - development versions
2.0 - initial release, chdk 1.1
2.1 - added palette type 4 - 16 entry VUYA, 2 bit alpha
2.2 - in development digic 6 support. Added LV_ASPECT_3_2, LV_FB_YUV8B and LV_FB_YUV8C formats
*/


// Control flags for determining which data block to transfer
#define LV_TFR_VIEWPORT     0x01
#define LV_TFR_BITMAP       0x04
#define LV_TFR_PALETTE      0x08
#define LV_TFR_BITMAP_OPACITY   0x10

enum lv_aspect_rato {
    LV_ASPECT_4_3,
    LV_ASPECT_16_9,
    // below added in 2.2
    LV_ASPECT_3_2,
};

/*
Framebuffer types
additional values will be added if new data formats appear
*/
enum lv_fb_type {
    LV_FB_YUV8, // 8 bit per element UYVYYY, used for live view
    LV_FB_PAL8, // 8 bit paletted, used for bitmap overlay. Note palette data and type sent separately
    // below added in 2.2
    LV_FB_YUV8B,// 8 bit per element UYVY, used for live view and overlay on Digic 6
    LV_FB_YUV8C,// 8 bit per element UYVY, used for alternate Digic 6 live view
    LV_FB_OPACITY8,// 8 bit opacity / alpha buffer
};

/*
framebuffer data description
NOTE YUV pixels widths are based on the number of Y elements
*/
typedef struct {
    int fb_type; // framebuffer type - note future versions might use different structures depending on type
    int data_start; // offset of data from start of live view header
    /*
    buffer width in pixels
    data size is always buffer_width*visible_height*(buffer bpp based on type)
    */
    int buffer_width;
    /*
    visible size in pixels
    describes data within the buffer which contains image data to be displayed
    any offsets within buffer data are added before sending, so the top left
    pixel is always the first first byte of data.
    width must always be <= buffer_width
    if buffer_width is > width, the additional data should be skipped
    visible_height also defines the number of data rows
    */
    int visible_width;
    int visible_height;

    /*
    margins
    pixels offsets needed to replicate display position on cameras screen
    not used for any buffer offsets
    */
    int margin_left;
    int margin_top;

    int margin_right;
    int margin_bot;
} lv_framebuffer_desc;

typedef struct {
    // live view sub-protocol version
    int version_major;
    int version_minor;
    int lcd_aspect_ratio; // physical aspect ratio of LCD
    int palette_type;
    int palette_data_start;
    // framebuffer descriptions are given as offsets, to allow expanding the structures in minor protocol changes
    int vp_desc_start;
    int bm_desc_start;
    int bmo_desc_start; // added in protocol 2.2
} lv_data_header;

#endif // __LIVE_VIEW_H
