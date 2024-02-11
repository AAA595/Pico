#include <stdlib.h>

#include "MarkSelection.h"
#include "VectorsExtension.h"
#include "Message.h"
#include "Draw.h"

typedef enum WriteDirection_ {
    DIRECTION_X_POSITIVE,
    DIRECTION_Z_POSITIVE,
    DIRECTION_X_NEGATIVE,
    DIRECTION_Z_NEGATIVE,
    DIRECTION_INVALID
} WriteDirection;

static char s_TextBuffer[STRING_SIZE];
static cc_string s_Text = { .buffer = s_TextBuffer, .capacity = STRING_SIZE, .length = 0 };

// Each letter is an 8x8 image, so that each letter can be described as 8 `char`s.
// Each `0xHH` is a row. Should be read from top to bottom.
static char font[256][8] = {
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x7e, 0x81, 0xa5, 0x81, 0xa5, 0x99, 0x81, 0x7e},
    {0x7e, 0xff, 0xdb, 0xff, 0xdb, 0xe7, 0xff, 0x7e},
    {0x6c, 0xfe, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x10},
    {0x20, 0x70, 0x70, 0xf8, 0xf8, 0x70, 0x70, 0x20},
    {0x30, 0x78, 0x78, 0xcc, 0xcc, 0x30, 0x30, 0x78},
    {0x30, 0x30, 0x78, 0x78, 0xfc, 0xfc, 0x30, 0x78},
    {0x0, 0x0, 0x60, 0xf0, 0xf0, 0x60, 0x0, 0x0},
    {0xff, 0xff, 0xe7, 0xc3, 0xc3, 0xe7, 0xff, 0xff},
    {0x0, 0x0, 0x60, 0x90, 0x90, 0x60, 0x0, 0x0},
    {0xff, 0xff, 0xe7, 0xdb, 0xdb, 0xe7, 0xff, 0xff},
    {0x1f, 0x3, 0x5, 0x79, 0x89, 0x88, 0x88, 0x70},
    {0x70, 0x88, 0x88, 0x70, 0x20, 0x70, 0x20, 0x20},
    {0x18, 0x1c, 0x16, 0x10, 0x70, 0xf0, 0xf0, 0x60},
    {0x70, 0x4e, 0x42, 0x42, 0x42, 0xc2, 0xc6, 0x6},
    {0x81, 0x42, 0x18, 0x24, 0x24, 0x18, 0x42, 0x81},
    {0x0, 0x0, 0x80, 0xe0, 0xf8, 0xe0, 0x80, 0x0},
    {0x0, 0x0, 0x8, 0x38, 0xf8, 0x38, 0x8, 0x0},
    {0x20, 0x70, 0xa8, 0x20, 0x20, 0xa8, 0x70, 0x20},
    {0x44, 0xee, 0xee, 0xee, 0xee, 0x44, 0x0, 0x44},
    {0x7f, 0x8a, 0x8a, 0x8a, 0x7a, 0xa, 0xa, 0xa},
    {0x70, 0x80, 0x60, 0x90, 0x90, 0x60, 0x10, 0xe0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff},
    {0x20, 0x70, 0xa8, 0x20, 0xa8, 0x70, 0x20, 0xf8},
    {0x20, 0x70, 0xa8, 0x20, 0x20, 0x20, 0x20, 0x20},
    {0x20, 0x20, 0x20, 0x20, 0x20, 0xa8, 0x70, 0x20},
    {0x0, 0x0, 0x0, 0x4, 0x2, 0xff, 0x2, 0x4},
    {0x0, 0x0, 0x0, 0x20, 0x40, 0xff, 0x40, 0x20},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xff},
    {0x0, 0x0, 0x0, 0x24, 0x42, 0xff, 0x42, 0x24},
    {0x0, 0x0, 0x20, 0x20, 0x70, 0x70, 0xf8, 0x0},
    {0x0, 0x0, 0xf8, 0x70, 0x70, 0x20, 0x20, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x0, 0x80, 0x0},
    {0xa0, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x50, 0x50, 0xf8, 0x50, 0xf8, 0x50, 0x50, 0x0},
    {0x20, 0x70, 0xa0, 0x70, 0x28, 0x70, 0x20, 0x0},
    {0x0, 0xc4, 0xc8, 0x10, 0x20, 0x4c, 0x8c, 0x0},
    {0x20, 0x70, 0x80, 0x60, 0x80, 0x70, 0x20, 0x0},
    {0x40, 0x40, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x20, 0x40, 0x40, 0x80, 0x80, 0x40, 0x40, 0x20},
    {0x80, 0x40, 0x40, 0x20, 0x20, 0x40, 0x40, 0x80},
    {0xa8, 0x70, 0xa8, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x40, 0x80},
    {0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0},
    {0x0, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x0},
    {0x60, 0x90, 0xb0, 0xd0, 0x90, 0x90, 0x60, 0x0},
    {0x40, 0xc0, 0x40, 0x40, 0x40, 0x40, 0xe0, 0x0},
    {0x60, 0x90, 0x10, 0x20, 0x40, 0x80, 0xf0, 0x0},
    {0x60, 0x90, 0x10, 0x60, 0x10, 0x90, 0x60, 0x0},
    {0x30, 0x50, 0x90, 0xf8, 0x10, 0x10, 0x10, 0x0},
    {0xf0, 0x80, 0x80, 0x70, 0x10, 0x10, 0xe0, 0x0},
    {0x60, 0x80, 0x80, 0xe0, 0x90, 0x90, 0x60, 0x0},
    {0xf0, 0x90, 0x10, 0x20, 0x20, 0x40, 0x40, 0x0},
    {0x60, 0x90, 0x90, 0x60, 0x90, 0x90, 0x60, 0x0},
    {0x60, 0x90, 0x90, 0x70, 0x10, 0x10, 0x60, 0x0},
    {0x0, 0x0, 0x80, 0x0, 0x0, 0x80, 0x0, 0x0},
    {0x0, 0x0, 0x80, 0x0, 0x0, 0x80, 0x80, 0x0},
    {0x0, 0x0, 0x10, 0x20, 0x40, 0x20, 0x10, 0x0},
    {0x0, 0x0, 0x0, 0x78, 0x0, 0x78, 0x0, 0x0},
    {0x0, 0x0, 0x40, 0x20, 0x10, 0x20, 0x40, 0x0},
    {0x70, 0x88, 0x88, 0x30, 0x20, 0x0, 0x20, 0x0},
    {0x7e, 0x81, 0x9d, 0xa5, 0xa5, 0xba, 0x80, 0x7e},
    {0x70, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x90, 0x0},
    {0xe0, 0x90, 0x90, 0xe0, 0x90, 0x90, 0xf0, 0x0},
    {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x0},
    {0xe0, 0x90, 0x90, 0x90, 0x90, 0x90, 0xe0, 0x0},
    {0xf0, 0x80, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x0},
    {0xf0, 0x80, 0x80, 0xe0, 0x80, 0x80, 0x80, 0x0},
    {0x70, 0x80, 0x80, 0xb0, 0x90, 0x90, 0x70, 0x0},
    {0x90, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x90, 0x0},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x7c, 0x8, 0x8, 0x8, 0x48, 0x48, 0x30, 0x0},
    {0x90, 0x90, 0xa0, 0xc0, 0xa0, 0x90, 0x90, 0x0},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0, 0x0},
    {0xd8, 0xa8, 0xa8, 0x88, 0x88, 0x88, 0x88, 0x0},
    {0x88, 0xc8, 0xa8, 0xa8, 0x98, 0x88, 0x88, 0x0},
    {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0xe0, 0x90, 0x90, 0xe0, 0x80, 0x80, 0x80, 0x0},
    {0x70, 0x88, 0x88, 0x88, 0xa8, 0x90, 0x78, 0x0},
    {0xe0, 0x90, 0x90, 0xe0, 0x90, 0x90, 0x90, 0x0},
    {0x70, 0x80, 0x80, 0x60, 0x10, 0x10, 0xe0, 0x0},
    {0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0},
    {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x0},
    {0x88, 0x88, 0x88, 0x88, 0x50, 0x50, 0x20, 0x0},
    {0x82, 0x82, 0x82, 0x82, 0x54, 0x54, 0x28, 0x0},
    {0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88, 0x0},
    {0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20, 0x0},
    {0xf8, 0x8, 0x10, 0x20, 0x40, 0x80, 0xf8, 0x0},
    {0xc0, 0x80, 0x80, 0x80, 0x80, 0x80, 0xc0, 0x0},
    {0x0, 0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x0},
    {0xc0, 0x40, 0x40, 0x40, 0x40, 0x40, 0xc0, 0x0},
    {0x20, 0x70, 0xd8, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xfe, 0x0},
    {0x80, 0x80, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x60, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x80, 0x80, 0xf0, 0x88, 0x88, 0x88, 0xf0, 0x0},
    {0x0, 0x0, 0x70, 0x80, 0x80, 0x80, 0x70, 0x0},
    {0x8, 0x8, 0x78, 0x88, 0x88, 0x88, 0x78, 0x0},
    {0x0, 0x0, 0x60, 0x90, 0xe0, 0x80, 0x70, 0x0},
    {0x20, 0x50, 0x40, 0xe0, 0x40, 0x40, 0x40, 0x0},
    {0x0, 0x0, 0x60, 0x90, 0x90, 0x60, 0x10, 0xe0},
    {0x80, 0x80, 0xe0, 0x90, 0x90, 0x90, 0x90, 0x0},
    {0x80, 0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x10, 0x0, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60},
    {0x0, 0x80, 0x80, 0xa0, 0xc0, 0xa0, 0xa0, 0x0},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x40, 0x0},
    {0x0, 0x0, 0xf0, 0xa8, 0xa8, 0x88, 0x88, 0x0},
    {0x0, 0x0, 0x78, 0x88, 0x88, 0x88, 0x88, 0x0},
    {0x0, 0x0, 0x70, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x0, 0x70, 0x88, 0x88, 0x88, 0xf0, 0x80},
    {0x0, 0x0, 0x70, 0x88, 0x88, 0x88, 0x78, 0x8},
    {0x0, 0x0, 0xb0, 0xc8, 0x80, 0x80, 0x80, 0x0},
    {0x0, 0x0, 0x78, 0x80, 0x70, 0x8, 0xf0, 0x0},
    {0x40, 0x40, 0xe0, 0x40, 0x40, 0x40, 0x20, 0x0},
    {0x0, 0x0, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x0, 0x88, 0x88, 0x50, 0x50, 0x20, 0x0},
    {0x0, 0x0, 0x88, 0x88, 0xa8, 0xa8, 0x50, 0x0},
    {0x0, 0x0, 0xd8, 0x70, 0x20, 0x70, 0xd8, 0x0},
    {0x0, 0x0, 0x88, 0x88, 0x88, 0x78, 0x8, 0xf0},
    {0x0, 0x0, 0xf8, 0x10, 0x20, 0x40, 0xf8, 0x0},
    {0x60, 0x40, 0x40, 0x80, 0x80, 0x40, 0x40, 0x60},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
    {0xc0, 0x40, 0x40, 0x20, 0x20, 0x40, 0x40, 0xc0},
    {0x0, 0x0, 0x60, 0x90, 0x12, 0xc, 0x0, 0x0},
    {0x20, 0x50, 0x88, 0xf8, 0xb8, 0xe8, 0xe8, 0xf8},
    {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0xc0},
    {0x50, 0x0, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x30, 0x0, 0x60, 0x90, 0xe0, 0x80, 0x70, 0x0},
    {0x60, 0x90, 0x60, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x90, 0x0, 0x60, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x30, 0x0, 0x60, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x40, 0xa0, 0x40, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x0, 0x0, 0x70, 0x80, 0x80, 0x80, 0x70, 0xc0},
    {0x60, 0x90, 0x60, 0x90, 0xe0, 0x80, 0x70, 0x0},
    {0x90, 0x0, 0x60, 0x90, 0xe0, 0x80, 0x70, 0x0},
    {0xc0, 0x0, 0x60, 0x90, 0xe0, 0x80, 0x70, 0x0},
    {0xa0, 0x0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x0},
    {0x40, 0xa0, 0x0, 0x40, 0x40, 0x40, 0x40, 0x0},
    {0x80, 0x40, 0x0, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x90, 0x0, 0xf0, 0x90, 0xf0, 0x90, 0x90, 0x0},
    {0x60, 0x60, 0xe0, 0x90, 0xf0, 0x90, 0x90, 0x0},
    {0x30, 0x0, 0xf0, 0x80, 0xe0, 0x80, 0xf0, 0x0},
    {0x0, 0x0, 0x6c, 0x12, 0x7c, 0x90, 0x6e, 0x0},
    {0x7e, 0x90, 0x90, 0xfc, 0x90, 0x90, 0x9e, 0x0},
    {0x20, 0x50, 0x0, 0x70, 0x88, 0x88, 0x70, 0x0},
    {0x50, 0x0, 0x70, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0xc0, 0x0, 0x70, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x20, 0x50, 0x0, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0xc0, 0x0, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x50, 0x0, 0x90, 0x90, 0x70, 0x10, 0xe0},
    {0x50, 0x70, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x50, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x20, 0x70, 0xa0, 0xa0, 0xa0, 0x70, 0x20},
    {0x18, 0x24, 0x40, 0xf0, 0x40, 0x80, 0xfc, 0x0},
    {0x88, 0x50, 0x20, 0x70, 0x20, 0x70, 0x20, 0x0},
    {0x0, 0xe0, 0x90, 0xe0, 0x88, 0x8c, 0x88, 0x8},
    {0xc, 0x12, 0x12, 0x38, 0x10, 0x90, 0x90, 0x60},
    {0x30, 0x0, 0x60, 0x10, 0x70, 0x90, 0x60, 0x0},
    {0x40, 0x80, 0x0, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x18, 0x0, 0x70, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x18, 0x0, 0x88, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x28, 0x50, 0x78, 0x88, 0x88, 0x88, 0x88, 0x0},
    {0x28, 0x50, 0x88, 0xc8, 0xa8, 0x98, 0x88, 0x0},
    {0x60, 0x90, 0x90, 0x70, 0x0, 0xf0, 0x0, 0x0},
    {0x60, 0x90, 0x90, 0x60, 0x0, 0xf0, 0x0, 0x0},
    {0x20, 0x0, 0x20, 0x60, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x0, 0x0, 0xf0, 0x80, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0xf0, 0x10, 0x0, 0x0, 0x0},
    {0x40, 0xc2, 0x44, 0xee, 0x19, 0x22, 0x44, 0xf},
    {0x40, 0xc2, 0x44, 0xea, 0x16, 0x2f, 0x42, 0x2},
    {0x80, 0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x0, 0x0, 0x0, 0x48, 0x90, 0x48, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x90, 0x48, 0x90, 0x0, 0x0},
    {0x88, 0x0, 0x22, 0x0, 0x88, 0x0, 0x22, 0x0},
    {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55},
    {0xff, 0xaa, 0xff, 0x55, 0xff, 0xaa, 0xff, 0x55},
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80},
    {0x10, 0x10, 0x10, 0xf0, 0x10, 0x10, 0x10, 0x10},
    {0x10, 0x10, 0xf0, 0x10, 0xf0, 0x10, 0x10, 0x10},
    {0x28, 0x28, 0x28, 0xe8, 0x28, 0x28, 0x28, 0x28},
    {0x0, 0x0, 0x0, 0xf8, 0x28, 0x28, 0x28, 0x28},
    {0x0, 0x0, 0xf0, 0x10, 0xf0, 0x10, 0x10, 0x10},
    {0x28, 0x28, 0xe8, 0x28, 0xe8, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28},
    {0x0, 0x0, 0xf8, 0x8, 0xe8, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0xe8, 0x8, 0xf8, 0x0, 0x0, 0x0},
    {0x28, 0x28, 0x28, 0xf8, 0x0, 0x0, 0x0, 0x0},
    {0x10, 0x10, 0xf0, 0x10, 0xf0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0xf0, 0x10, 0x10, 0x10, 0x10},
    {0x10, 0x10, 0x10, 0x1f, 0x0, 0x0, 0x0, 0x0},
    {0x10, 0x10, 0x10, 0xff, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0xff, 0x10, 0x10, 0x10, 0x10},
    {0x10, 0x10, 0x10, 0x1f, 0x10, 0x10, 0x10, 0x10},
    {0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0},
    {0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10},
    {0x10, 0x10, 0x1f, 0x10, 0x1f, 0x10, 0x10, 0x10},
    {0x28, 0x28, 0x28, 0x2f, 0x28, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0x2f, 0x20, 0x3f, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x3f, 0x20, 0x2f, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0xef, 0x0, 0xff, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0xff, 0x0, 0xef, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0x2f, 0x20, 0x2f, 0x28, 0x28, 0x28},
    {0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0},
    {0x28, 0x28, 0xef, 0x0, 0xef, 0x28, 0x28, 0x28},
    {0x10, 0x10, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0},
    {0x28, 0x28, 0x28, 0xff, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0xff, 0x0, 0xff, 0x10, 0x10, 0x10},
    {0x0, 0x0, 0x0, 0xff, 0x28, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0x28, 0x3f, 0x0, 0x0, 0x0, 0x0},
    {0x10, 0x10, 0x1f, 0x10, 0x1f, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x1f, 0x10, 0x1f, 0x10, 0x10, 0x10},
    {0x0, 0x0, 0x0, 0x3f, 0x28, 0x28, 0x28, 0x28},
    {0x28, 0x28, 0x28, 0xff, 0x28, 0x28, 0x28, 0x28},
    {0x10, 0x10, 0xff, 0x0, 0xff, 0x10, 0x10, 0x10},
    {0x10, 0x10, 0x10, 0xf0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x1f, 0x10, 0x10, 0x10, 0x10},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff},
    {0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0},
    {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf},
    {0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x68, 0x90, 0x90, 0x90, 0x68, 0x0},
    {0x0, 0xe0, 0x90, 0xf0, 0x90, 0x90, 0xe0, 0x80},
    {0xf0, 0x90, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0},
    {0x0, 0x0, 0xf8, 0x50, 0x50, 0x50, 0x50, 0x0},
    {0xf8, 0x88, 0x40, 0x20, 0x40, 0x88, 0xf8, 0x0},
    {0x0, 0x0, 0x7c, 0x88, 0x88, 0x88, 0x70, 0x0},
    {0x0, 0x0, 0x88, 0x88, 0x88, 0x88, 0xf0, 0x80},
    {0x0, 0x0, 0xf8, 0x40, 0x40, 0x40, 0x30, 0x0},
    {0x70, 0x20, 0x70, 0x88, 0x88, 0x70, 0x20, 0x70},
    {0x0, 0x70, 0x88, 0xf8, 0x88, 0x88, 0x70, 0x0},
    {0x78, 0x84, 0x84, 0x84, 0x84, 0x48, 0xcc, 0x0},
    {0x0, 0xf0, 0x80, 0x60, 0x90, 0x90, 0x60, 0x0},
    {0x0, 0x0, 0x0, 0x6c, 0x92, 0x92, 0x6c, 0x0},
    {0x0, 0x18, 0xa4, 0xa4, 0x78, 0x20, 0x20, 0x0},
    {0x0, 0x0, 0x70, 0x80, 0x60, 0x80, 0x70, 0x0},
    {0x0, 0x0, 0x30, 0x48, 0x84, 0x84, 0x84, 0x0},
    {0x0, 0x0, 0xf8, 0x0, 0xf8, 0x0, 0xf8, 0x0},
    {0x0, 0x0, 0x20, 0x20, 0xf8, 0x20, 0xf8, 0x0},
    {0x0, 0x0, 0x40, 0x20, 0x10, 0x20, 0x48, 0x10},
    {0x0, 0x0, 0x10, 0x20, 0x40, 0x20, 0x90, 0x40},
    {0x0, 0x10, 0x28, 0x20, 0x20, 0x20, 0x20, 0x20},
    {0x20, 0x20, 0x20, 0x20, 0x20, 0xa0, 0x40, 0x0},
    {0x0, 0x0, 0x20, 0x0, 0xf8, 0x0, 0x20, 0x0},
    {0x0, 0x40, 0xa8, 0x10, 0x40, 0xa8, 0x10, 0x0},
    {0x40, 0xa0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0xc0, 0xc0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0},
    {0x3f, 0x20, 0x20, 0x20, 0xa0, 0x40, 0x40, 0x0},
    {0x40, 0xa0, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x60, 0x90, 0x20, 0x40, 0xf0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

static void Write_Command(const cc_string* args, int argsCount);

struct ChatCommand WriteCommand = {
    "Write",
    Write_Command,
    COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&b/Write <text>",
        "Writes &btext &fusing white wool.",
        NULL,
        NULL,
        NULL
    },
    NULL
};

static IVec3 From2DTo3D(IVec2 vector, int textOriginX3D, int textOriginZ3D, int lineBaseY, WriteDirection direction) {
    IVec3 result;

    if (direction == DIRECTION_X_POSITIVE) {
        result.X = textOriginX3D + vector.X;
        result.Y = lineBaseY + vector.Y;
        result.Z = textOriginZ3D;
    } else if (direction == DIRECTION_X_NEGATIVE) {
        result.X = textOriginX3D - vector.X;
        result.Y = lineBaseY + vector.Y;
        result.Z = textOriginZ3D;
    } else if (direction == DIRECTION_Z_POSITIVE) {
        result.X = textOriginX3D;
        result.Y = lineBaseY + vector.Y;
        result.Z = textOriginZ3D + vector.X;
    } else {
        result.X = textOriginX3D;
        result.Y = lineBaseY + vector.Y;
        result.Z = textOriginZ3D - vector.X;
    }

    return result;
}

static void WriteLetter(char letter, int textOriginX3D, int textOriginZ3D, int lineBaseY, WriteDirection direction, int* offset) {
    bool letterArray[8][8];

    char* letterPointer = (char*)&font[(unsigned char)letter];
    char row;

    for (int i = 0; i < 8; i++) {
        row = letterPointer[i];

        for (int j = 0; j < 8; j++) {
            letterArray[i][j] = (row >> (7 - j)) & 1;
        }
    }

    int letterWidth = 0;
    IVec2 local2DCoordinates;
    IVec3 global3DCoordinates;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (letterArray[i][j]) {
                local2DCoordinates.X = *offset + j;
                local2DCoordinates.Y = 7 - i;
                global3DCoordinates = From2DTo3D(local2DCoordinates, textOriginX3D, textOriginZ3D, lineBaseY, direction);
                Draw_Block(global3DCoordinates.X, global3DCoordinates.Y, global3DCoordinates.Z, BLOCK_WHITE);

                if (j + 1 > letterWidth) {
                    letterWidth = j + 1;
                }
            }
        }
    }

    *offset += letterWidth + 1;
}

static WriteDirection GuessTextDirection(IVec3 mark1, IVec3 mark2) {
    int deltaX = abs(mark1.X - mark2.X);
    int deltaZ = abs(mark1.Z - mark2.Z);

    if (deltaX < deltaZ) {
        if (mark1.Z < mark2.Z) {
            return DIRECTION_Z_POSITIVE;
        } else if (mark1.Z > mark2.Z) {
            return DIRECTION_Z_NEGATIVE;
        }
    } else {
        if (mark1.X < mark2.X) {
            return DIRECTION_X_POSITIVE;
        } else if (mark1.X > mark2.X) {
            return DIRECTION_X_NEGATIVE;
        }
    }

    // Only happens if both marks are on the same column.
    return DIRECTION_INVALID;
}

static void WriteSelectionHandler(IVec3* marks, int count) {
    int textOriginX = marks[0].X;
    int textOriginZ = marks[0].Z;

    int lineBase = marks[0].Y;

    WriteDirection direction = GuessTextDirection(marks[0], marks[1]);

    if (direction == DIRECTION_INVALID) {
        Message_Player("Could not infer direction. The marks must be on different columns.");
        return;
    }

    int offset = 0;
    Draw_Start("Write");

    for (int characterIndex = 0; characterIndex < s_Text.length; characterIndex++) {
        if (s_Text.buffer[characterIndex] == ' ') {
            #define SPACE_WIDTH 3
            offset += SPACE_WIDTH;
        } else {
            WriteLetter(s_Text.buffer[characterIndex], textOriginX, textOriginZ, lineBase, direction, &offset);
        }
    }

    int blocksAffected = Draw_End();

    Message_BlocksAffected(blocksAffected);
}

static void Write_Command(const cc_string* text, int _) {
    if (text->length == 0) {
        Message_CommandUsage(WriteCommand);
        return;
    }

    String_Copy(&s_Text, text);
    MarkSelection_Make(WriteSelectionHandler, 2, "Write");
    Message_Player("Place or break two blocks to indicate direction.");
}