const unsigned short waStarts[] = {
 0, 4, 12, 20, 29, 42, 53, 56, // <-- (
 62, 67, 73, 81, 85, 91, 95, 100, // <-- 0
 109, 117, 125, 133, 142, 149, 158, 166, // <-- 8
 174, 184, 189, 193, 202, 211, 219, 229, // <-- @
 243, 254, 265, 276, 288, 297, 306, 318, // <-- H
 330, 333, 343, 353, 362, 376, 386, 398, // <-- P
 407, 420, 430, 440, 449, 460, 470, 484, // <-- X
 495, 505, 514, 518, 523, 529, 535, 546, // <-- `
 550, 559, 568, 576, 585, 594, 600, 608, // <-- h
 618, 621, 626, 635, 639, 652, 662, 671, // <-- p
 680, 690, 696, 704, 709, 718, 727, 738, // <-- x
 747, 756, 763, 769, 774, 779, 788 // <-- end of bitmap
};

const unsigned int uiPixelsX=789, uiPixelsY=16;

const unsigned char baCharset[] = {
0x00,0x00,0x00,0x35,0x13,0x53,0x00,0x00,0x00,0x00,0x00,0x00,0xA3,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x10,0x00,0x01,0x10,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x03,0x55,0x30,0x00,0x01,0x55,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x35,0x00,0x00,0x00,0x00,0x00,0x01,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x35,0x30,0x00,0x00,0x00,0x00,0x00,0x15,0x51,0x00,0x00,0x00,0x00,
0x03,0x53,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x53,0x00,0x00,0x01,0x55,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,
0x00,0x51,0x00,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3A,0xA0,0x0A,0xF5,0x7F,
0x80,0x00,0x1A,0x03,0x70,0x01,0x7F,0xA5,0x10,0x01,0x8A,0x30,0x00,0x01,0xF5,0x00,
0x00,0x5E,0xFE,0x50,0x00,0x0F,0xF3,0x00,0x0A,0xAA,0xA1,0x00,0x00,0x87,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xA0,0x08,0xFE,0x50,
0x00,0x01,0x5A,0xA0,0x00,0x5C,0xFF,0xC5,0x00,0x3A,0xFF,0xE8,0x00,0x00,0x00,0x3A,
0xA0,0x00,0xAA,0xAA,0xA3,0x00,0x05,0xCF,0xFC,0x03,0xAA,0xAA,0xAA,0x70,0x05,0xCF,
0xF8,0x00,0x03,0xCF,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3A,0xFF,0xFF,0xA1,0x00,0x00,0x00,0x18,
0xEF,0xF8,0x10,0x00,0x00,0x00,0x8A,0x70,0x00,0x00,0xAA,0xAA,0xAA,0xA5,0x00,0x00,
0x07,0xAF,0xFF,0xFA,0x30,0x7A,0xAA,0xA8,0x30,0x00,0x07,0xAA,0xAA,0xAA,0xA3,0x07,
0xAA,0xAA,0xAA,0xA0,0x00,0x3A,0xFF,0xFF,0xEA,0x00,0x7A,0x70,0x00,0x7A,0x70,0x0A,
0xA3,0x00,0x00,0x0A,0xA3,0x00,0xAA,0x30,0x00,0x7A,0x10,0x0A,0xA3,0x00,0x00,0x00,
0x7A,0xA3,0x00,0x05,0xAA,0x30,0x0A,0xA1,0x00,0x03,0xA3,0x00,0x01,0xAF,0xFF,0xA1,
0x00,0x03,0xAA,0xAA,0xAA,0x50,0x00,0x05,0xCF,0xFE,0x80,0x00,0x0A,0xAA,0xAA,0xA7,
0x00,0x00,0x18,0xEF,0xFF,0xC7,0x07,0xAA,0xAA,0xAA,0xAA,0x07,0xA7,0x00,0x00,0xAA,
0x33,0xAA,0x00,0x00,0x07,0xA3,0xA7,0x00,0x03,0xA7,0x00,0x03,0xA3,0x7A,0xA0,0x00,
0x08,0xA0,0xAA,0x50,0x00,0x05,0xA3,0x3A,0xAA,0xAA,0xAA,0x30,0xAF,0xFE,0xA0,0x00,
0x5F,0xFF,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xEA,0x00,0x00,0x00,
0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0xFA,0x00,
0x00,0x00,0x00,0x00,0x5F,0xFF,0x50,0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0x00,0x03,
0xAA,0x03,0xAA,0x00,0xAF,0xA0,0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xEF,0xA0,0x0F,0x50,0x0F,0xF5,0x00,
0x00,0x00,0x00,0x00,0x00,0x05,0xFF,0x00,0xAF,0x15,0xF5,0x00,0x05,0xC0,0xA7,0x01,
0xEF,0xFE,0xF7,0x00,0xEE,0xCF,0x30,0x00,0xE8,0x00,0x00,0x5F,0xE5,0xEF,0x10,0x00,
0xFF,0x00,0x0A,0xF3,0x3F,0xE0,0x01,0x0A,0x81,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0xAA,0x08,0xF8,0xCF,0x50,0x08,0xFF,0xFA,0x00,0x0A,
0xC5,0x7F,0xF3,0x05,0xC5,0x7E,0xF8,0x00,0x00,0x1E,0xFF,0x00,0x0F,0xFF,0xFF,0x50,
0x08,0xFE,0x58,0xE0,0x5F,0xFF,0xFF,0xFA,0x03,0xFF,0x5C,0xFA,0x03,0xFE,0x5A,0xF8,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x05,0xE8,0x55,0xCF,0xE1,0x00,0x00,0x5F,0x71,0x01,0x8E,0x10,0x00,0x00,
0x1F,0xFF,0x00,0x00,0x0F,0xFC,0xAA,0xCF,0xF8,0x00,0x0A,0xFE,0x85,0x7C,0xF5,0x0A,
0xFE,0xAC,0xFF,0xA0,0x00,0xAF,0xEA,0xAA,0xAA,0x30,0xAF,0xEA,0xAA,0xAA,0x00,0x5F,
0xFA,0x55,0x8E,0xF0,0x0A,0xFA,0x00,0x0A,0xFA,0x00,0xFF,0x50,0x00,0x00,0xFF,0x50,
0x0F,0xF5,0x00,0x5F,0xA0,0x00,0xFF,0x50,0x00,0x00,0x0A,0xFF,0x80,0x00,0xAF,0xF5,
0x00,0xFF,0xA0,0x00,0x5F,0x50,0x03,0xFF,0x85,0x8F,0xF5,0x00,0x5F,0xFA,0xAC,0xFF,
0x80,0x08,0xFE,0x75,0xCF,0xE1,0x00,0xFF,0xCA,0xAF,0xFE,0x00,0x0C,0xFC,0x55,0x8E,
0xA0,0x7A,0xAE,0xFE,0xAA,0xA0,0xAF,0xA0,0x00,0x0F,0xF5,0x1F,0xF5,0x00,0x00,0xEC,
0x3F,0xF0,0x00,0x8F,0xF0,0x00,0x8F,0x01,0xFF,0x80,0x05,0xF8,0x07,0xFE,0x00,0x01,
0xEC,0x03,0xAA,0xAA,0xEF,0xF3,0x0A,0xF0,0x3F,0x10,0x00,0x0A,0xF0,0x00,0x00,0xE3,
0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xE5,0x00,0x00,0x00,0x00,0x05,0xFF,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0x00,0x00,0x00,0x0E,0xFA,
0x00,0x00,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0x00,0x5F,0xF0,0x5F,0xF0,0x0A,0xFA,
0x00,0x00,0x05,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x01,0x51,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xAF,0x30,0x00,0xF5,0x00,0x1C,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
0x5F,0xF0,0x0A,0xF0,0x5F,0x50,0x00,0xA8,0x0E,0x30,0x8F,0xCA,0x51,0x10,0x5F,0x50,
0xFA,0x00,0x8C,0x00,0x00,0x0A,0xFA,0x05,0xF5,0x00,0x0A,0xF0,0x07,0xF8,0x00,0x8F,
0x80,0x8E,0x87,0xE7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0E,0x71,0xFF,0x05,0xFC,0x00,0x55,0xAF,0xA0,0x00,0x10,0x00,0xCF,0xA0,0x00,0x00,
0x7F,0xF0,0x00,0x0A,0xFF,0xF0,0x00,0xF5,0x00,0x00,0x03,0xFE,0x10,0x00,0x00,0x00,
0x00,0x5F,0x50,0xAF,0xA0,0x5F,0xF0,0xCF,0x70,0x0E,0xF1,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x01,
0xFF,0x50,0x00,0x5E,0x10,0x35,0x51,0x7C,0x00,0x00,0x08,0xFE,0xF7,0x00,0x00,0xFF,
0x50,0x00,0x7F,0xF0,0x08,0xFE,0x10,0x00,0x01,0x10,0xAF,0xA0,0x05,0xFF,0x80,0x0A,
0xFA,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0x00,0x3F,0xF5,0x00,0x00,0x03,0x00,0xAF,
0xA0,0x00,0xAF,0xA0,0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x00,0xFF,0x50,0x1F,0xA0,0x00,
0x0F,0xF5,0x00,0x00,0x00,0xAF,0xFE,0x00,0x0F,0xFF,0x50,0x0F,0xFF,0x30,0x05,0xF5,
0x00,0xEF,0x70,0x00,0x7F,0xE0,0x05,0xFF,0x00,0x0A,0xFF,0x03,0xFF,0x10,0x00,0xCF,
0x80,0x0F,0xF5,0x00,0x5F,0xF5,0x05,0xFF,0x10,0x00,0x01,0x00,0x00,0xAF,0xA0,0x00,
0x0A,0xFA,0x00,0x00,0xFF,0x50,0xCF,0xA0,0x00,0x3F,0x70,0xFF,0x30,0x0C,0xFF,0x10,
0x0E,0xC0,0x08,0xFF,0x10,0xEE,0x00,0x0E,0xF8,0x00,0x8F,0x30,0x00,0x00,0x1E,0xFA,
0x00,0xAF,0x00,0xE7,0x00,0x00,0xAF,0x00,0x00,0x7F,0xC0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x0A,0xFA,0x00,0x00,0x00,0x00,0x00,0xFF,0x50,0x00,0x00,0x00,0x00,0x00,
0xAF,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0x00,0x5F,0xF0,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xFF,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0xF7,0x00,
0x0F,0x50,0x00,0xAF,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0xFF,0x00,0x8F,0x05,0xF5,
0x07,0xAE,0xCA,0xFA,0x1A,0xFA,0xA5,0x00,0x05,0xF5,0x0F,0xA0,0x5F,0x10,0x00,0x00,
0x7F,0xE0,0xCF,0x10,0x00,0xAE,0x00,0xEF,0x10,0x01,0xFF,0x11,0x55,0x55,0x10,0x00,
0x3A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF1,0x5F,0xA0,0x0F,0xF0,
0x00,0x0A,0xFA,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0x05,0xFE,0x00,0x05,0xF7,0xFF,
0x00,0x0F,0x50,0x00,0x00,0x8F,0x80,0x00,0x00,0x00,0x00,0x0E,0xA0,0x0A,0xFC,0x05,
0xFC,0x0F,0xF5,0x00,0xAF,0x70,0x7A,0x70,0x07,0xA7,0x00,0x00,0x00,0x00,0x70,0x00,
0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0xF3,0x00,0x3E,0x10,0xAC,
0xCF,0x50,0xE3,0x00,0x00,0xEC,0x8F,0xC0,0x00,0x0F,0xF5,0x00,0x05,0xFE,0x00,0xFF,
0x70,0x00,0x00,0x00,0x0A,0xFA,0x00,0x08,0xFF,0x00,0xAF,0xA0,0x00,0x00,0x00,0xAF,
0xA0,0x00,0x00,0x0A,0xFC,0x00,0x00,0x00,0x00,0x0A,0xFA,0x00,0x0A,0xFA,0x00,0xFF,
0x50,0x00,0x00,0xFF,0x50,0x0F,0xF5,0x1E,0xE1,0x00,0x00,0xFF,0x50,0x00,0x00,0x0A,
0xFE,0xF1,0x05,0xFC,0xF5,0x00,0xFF,0xFE,0x00,0x5F,0x50,0x5F,0xF0,0x00,0x00,0xFF,
0x50,0x5F,0xF0,0x00,0x5F,0xF0,0xAF,0xA0,0x00,0x05,0xFF,0x00,0xFF,0x50,0x00,0xFF,
0x50,0x5F,0xF1,0x00,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0xAF,0xA0,0x00,0x0F,0xF5,
0x07,0xFF,0x00,0x08,0xF1,0x0A,0xF7,0x00,0xFF,0xF5,0x01,0xF7,0x00,0x0E,0xFA,0x8F,
0x30,0x00,0x7F,0xF1,0x3F,0x80,0x00,0x00,0x08,0xFE,0x10,0x0A,0xF0,0x08,0xC0,0x00,
0x0A,0xF0,0x00,0x0C,0xCF,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0xCF,0xFE,
0x50,0x05,0xFF,0x3C,0xFC,0x10,0x00,0x7C,0xFF,0xF7,0x00,0x5C,0xFF,0xFF,0xA0,0x01,
0xCF,0xF8,0x10,0x7F,0xFC,0x70,0x08,0xEF,0xCA,0xA0,0x0A,0xFA,0x5E,0xFA,0x00,0x3A,
0xA0,0x0A,0xA3,0x0A,0xFA,0x00,0xA8,0x05,0xFF,0x00,0xAA,0x5C,0xFE,0x55,0xEF,0x80,
0x07,0xA7,0x5E,0xFA,0x00,0x01,0x8E,0xFE,0x50,0x00,0xAA,0x58,0xFE,0x50,0x00,0x5C,
0xFE,0xAA,0x70,0x0A,0xA7,0xE5,0x05,0xCF,0xFF,0x70,0x7F,0xFC,0x70,0xAA,0x30,0x0A,
0xA3,0x3A,0xA0,0x00,0x3A,0x38,0xA1,0x03,0xA8,0x00,0x78,0x0A,0xA5,0x00,0x8A,0x0A,
0xA3,0x00,0x0A,0x70,0xAA,0xAA,0xA7,0x00,0x7F,0xA0,0x00,0xF5,0x00,0x0F,0xE0,0x00,
0x00,0x00,0x00,0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0x00,0x5A,0xFA,0xCE,0xA3,0x3F,
0xFE,0x50,0x00,0x1F,0xA7,0xF7,0x1E,0x50,0x00,0x00,0x01,0xFF,0xFE,0x30,0x00,0x03,
0x30,0x5F,0xE0,0x00,0x0E,0xF5,0x0A,0x8C,0xA0,0x00,0x05,0xF0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x8C,0x0A,0xFA,0x00,0xFF,0x50,0x00,0xAF,0xA0,0x00,0x00,
0x00,0xEF,0x70,0x00,0x03,0xEE,0x50,0x01,0xEC,0x0F,0xF0,0x00,0xF8,0x51,0x00,0x0C,
0xF5,0x7A,0x81,0x00,0x00,0x07,0xF1,0x00,0x3F,0xFA,0xCE,0x10,0xFF,0x50,0x0A,0xFA,
0x0A,0xFA,0x00,0xAF,0xA0,0x00,0x00,0x07,0xEF,0x00,0x00,0x00,0x00,0x00,0xEE,0x70,
0x00,0x00,0x00,0x00,0x0A,0xF5,0x00,0x0C,0x50,0xA7,0x01,0xF3,0x0A,0x50,0x00,0x5F,
0x73,0xFF,0x10,0x00,0xFF,0x50,0x03,0xEF,0x50,0x5F,0xF1,0x00,0x00,0x00,0x00,0xAF,
0xA0,0x00,0x1F,0xF5,0x0A,0xFA,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0x00,0xEF,0x70,
0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0xAF,0xA0,0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x00,
0xFF,0x5C,0xE1,0x00,0x00,0x0F,0xF5,0x00,0x00,0x00,0xAF,0x8F,0x70,0x8C,0xAF,0x50,
0x0F,0xCE,0xF8,0x05,0xF5,0x0A,0xFC,0x00,0x00,0x0A,0xFA,0x05,0xFF,0x00,0x08,0xFE,
0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x0F,0xF5,0x00,0x5F,0xF1,0x01,0xFF,0xFA,0x50,0x00,
0x00,0x00,0xAF,0xA0,0x00,0x0A,0xFA,0x00,0x00,0xFF,0x50,0x1F,0xF5,0x00,0xEC,0x00,
0x7F,0xA0,0x5F,0xAF,0xA0,0x5F,0x30,0x00,0x7F,0xFF,0x80,0x00,0x00,0xEF,0x8E,0xE1,
0x00,0x00,0x03,0xFF,0x70,0x00,0xAF,0x00,0x5F,0x00,0x00,0xAF,0x00,0x03,0xF1,0xCC,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAA,0x57,0xFF,0x10,0x5F,0xFF,0xAE,0xFC,
0x00,0x8F,0xE7,0x5A,0x80,0x3F,0xF7,0x5E,0xFA,0x01,0xEF,0x7A,0xF8,0x07,0xFF,0xC7,
0x0A,0xFE,0x57,0xFF,0x00,0xAF,0xEF,0xAF,0xF8,0x05,0xFF,0x00,0xFF,0x50,0xAF,0xA0,
0x8F,0x50,0x5F,0xF0,0x0F,0xFF,0xCE,0xFE,0xFC,0xFF,0x30,0xAF,0xEF,0xAF,0xF8,0x00,
0xEF,0xA5,0xEF,0x70,0x0F,0xFF,0xCC,0xFF,0x10,0x3F,0xF7,0x5E,0xFA,0x00,0xFF,0xFA,
0x51,0xFF,0x75,0x88,0x07,0xFF,0xC7,0x0F,0xF5,0x00,0xFF,0x51,0xFF,0x30,0x08,0xF1,
0xAF,0x50,0x7F,0xF0,0x0E,0xA0,0x7F,0xE1,0x5F,0x70,0xCF,0x80,0x03,0xF5,0x0A,0xAA,
0xFF,0xA0,0x01,0xFA,0x00,0x0F,0x50,0x00,0xFA,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
0xFC,0x00,0x00,0x00,0x00,0x00,0x5C,0x0A,0x80,0x00,0x5E,0xFE,0x10,0x00,0x5E,0xF8,
0x0C,0xA0,0x35,0x00,0x03,0xEF,0xFF,0x30,0x0A,0x70,0x00,0x0A,0xFA,0x00,0x00,0xAF,
0xA0,0xA1,0x3A,0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,
0x80,0xAF,0xA0,0x0F,0xF5,0x00,0x0A,0xFA,0x00,0x00,0x00,0x8F,0xC0,0x00,0x5F,0xFC,
0x30,0x00,0xCE,0x13,0xFF,0x00,0x0F,0xFF,0xF8,0x00,0xFF,0xEC,0xCF,0xE1,0x00,0x01,
0xF8,0x00,0x00,0x7F,0xFF,0x70,0x0C,0xFC,0x05,0xFF,0xA0,0x7A,0x70,0x07,0xA7,0x00,
0x00,0x7E,0xF8,0x10,0xAF,0xFF,0xFF,0xFA,0x00,0x7E,0xE7,0x00,0x00,0x00,0x1E,0xC1,
0x00,0x01,0xE0,0x3E,0x00,0x7F,0x00,0xA5,0x00,0x0A,0xF0,0x0C,0xF8,0x00,0x0F,0xFF,
0xFF,0xFC,0x10,0x05,0xFF,0x00,0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0xFF,0x50,0xAF,
0xFF,0xFF,0xF5,0x00,0xAF,0xEA,0xAA,0xA0,0x0F,0xF5,0x00,0x00,0x00,0x00,0x0A,0xFF,
0xFF,0xFF,0xFA,0x00,0xFF,0x50,0x00,0x00,0xFF,0x50,0x0F,0xFE,0xFA,0x00,0x00,0x00,
0xFF,0x50,0x00,0x00,0x0A,0xF5,0xFA,0x0E,0x8C,0xF5,0x00,0xFA,0x5F,0xF3,0x5F,0x50,
0xAF,0xA0,0x00,0x00,0xAF,0xA0,0x5F,0xF5,0x58,0xFF,0x50,0xFF,0x50,0x00,0x00,0xFF,
0x50,0xFF,0xCA,0xCF,0xF5,0x00,0x03,0xEF,0xFF,0xE7,0x00,0x00,0x0A,0xFA,0x00,0x00,
0xAF,0xA0,0x00,0x0F,0xF5,0x00,0xAF,0xA0,0x3F,0x70,0x03,0xFF,0x08,0xF3,0xFE,0x0A,
0xF0,0x00,0x00,0xCF,0xF1,0x00,0x00,0x07,0xFF,0xF7,0x00,0x00,0x00,0xEF,0xC0,0x00,
0x0A,0xF0,0x00,0xF5,0x00,0x0A,0xF0,0x00,0xC8,0x03,0xF3,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x0F,0xF5,0x05,0xFF,0x50,0x5F,0xF1,0x1F,0xF7,0x00,0x00,0x0A,
0xF8,0x00,0xAF,0xA0,0x8F,0xA0,0x0F,0xF0,0x0F,0xF5,0x01,0xFF,0x10,0x0F,0xF0,0x0A,
0xFF,0x10,0xAF,0xA0,0x5F,0xF0,0x0F,0xF5,0x0A,0xFA,0x3F,0x80,0x05,0xFF,0x00,0xFF,
0xA0,0x5F,0xF5,0x0F,0xF5,0x0A,0xFF,0x10,0xAF,0xA0,0x5F,0xE0,0x05,0xFF,0x00,0xFF,
0xA0,0x0F,0xF7,0x0A,0xF8,0x00,0xAF,0xA0,0x0F,0xFA,0x00,0x5F,0xF1,0x00,0x00,0x0F,
0xF5,0x00,0xFF,0x50,0x0F,0xF5,0x0A,0xF8,0x00,0xEC,0x05,0xFA,0x0A,0xFF,0x31,0xF5,
0x00,0xCF,0xAE,0xE0,0x05,0xFE,0x00,0xCE,0x00,0x00,0x3F,0xF3,0x00,0x5F,0x50,0x00,
0xF5,0x00,0x0A,0xC1,0x00,0x15,0x30,0x00,0x00,0x00,0x0F,0xA0,0x00,0x00,0x00,0x00,
0x08,0x80,0xE3,0x00,0x00,0x1F,0xFF,0x30,0x00,0x00,0x07,0xE0,0xAF,0xFE,0x10,0xEF,
0x58,0xFC,0x00,0xFA,0x00,0x00,0xAF,0xA0,0x00,0x0A,0xFA,0x00,0x00,0x00,0x3A,0xAC,
0xFA,0xAA,0x00,0x00,0x05,0xFF,0xF5,0x00,0x00,0x01,0xF3,0x0A,0xFA,0x00,0xFF,0x50,
0x00,0xAF,0xA0,0x00,0x00,0x5F,0xA1,0x00,0x01,0x57,0xEF,0x80,0x7F,0x50,0x5F,0xF0,
0x00,0x00,0x5F,0xF3,0x0F,0xFC,0x00,0xEF,0x70,0x00,0xAF,0x10,0x00,0x5F,0xC8,0xFF,
0xA0,0x1E,0xFF,0xEC,0xFA,0x00,0x00,0x00,0x00,0x00,0x07,0xEF,0x81,0x00,0x03,0x55,
0x55,0x55,0x30,0x00,0x07,0xEE,0x70,0x00,0x0E,0xF1,0x00,0x00,0x5A,0x08,0xA0,0x1E,
0xC0,0x0F,0x00,0x01,0xF8,0x00,0x7F,0xE0,0x00,0xFF,0x85,0x58,0xFE,0x50,0x5F,0xF0,
0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0x0F,0xF5,0x0A,0xFC,0x55,0x55,0x10,0x0A,0xFE,
0xAA,0xAA,0x00,0xFF,0x50,0x00,0xFF,0xFF,0x00,0xAF,0xC5,0x55,0xCF,0xA0,0x0F,0xF5,
0x00,0x00,0x0F,0xF5,0x00,0xFF,0xAF,0xF5,0x00,0x00,0x0F,0xF5,0x00,0x00,0x00,0xAF,
0x0F,0xF1,0xF3,0xFF,0x50,0x0F,0xA0,0xCF,0xC5,0xF5,0x0A,0xFA,0x00,0x00,0x0A,0xFA,
0x05,0xFF,0xFF,0xFA,0x50,0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x0F,0xFC,0xAE,0xF7,0x00,
0x00,0x00,0x5A,0xFF,0xFA,0x00,0x00,0xAF,0xA0,0x00,0x0A,0xFA,0x00,0x00,0xFF,0x50,
0x05,0xFF,0x18,0xF1,0x00,0x0F,0xF3,0xCA,0x0F,0xF0,0xFA,0x00,0x00,0x0E,0xFF,0x70,
0x00,0x00,0x0C,0xFC,0x00,0x00,0x00,0x7F,0xF3,0x00,0x00,0xAF,0x00,0x0A,0xA0,0x00,
0xAF,0x00,0x3F,0x10,0x0C,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x8A,0xFF,
0x50,0x5F,0xF0,0x00,0xFF,0x55,0xFF,0x00,0x00,0x00,0xFF,0x50,0x0A,0xFA,0x0A,0xFE,
0xAA,0xFF,0x30,0xFF,0x50,0x5F,0xF0,0x00,0xFF,0x00,0xAF,0xA0,0x0A,0xFA,0x05,0xFF,
0x00,0xFF,0x50,0xAF,0xAE,0xE0,0x00,0x5F,0xF0,0x0F,0xF5,0x05,0xFF,0x00,0xFF,0x50,
0xAF,0xA0,0x0A,0xFA,0x0A,0xFA,0x00,0x0F,0xF5,0x0F,0xF5,0x00,0xAF,0xA0,0xFF,0x50,
0x0A,0xFA,0x00,0xFF,0x50,0x01,0xFF,0xE8,0x10,0x00,0xFF,0x50,0x0F,0xF5,0x00,0xFF,
0x50,0x5F,0xE0,0x3F,0x50,0x1F,0xE0,0xFA,0xF7,0x5F,0x10,0x03,0xFF,0xF3,0x00,0x0F,
0xF3,0x1F,0x70,0x00,0x0E,0xF8,0x00,0xAF,0x80,0x00,0x0F,0x50,0x00,0x0E,0xF0,0x1E,
0xFF,0xA1,0x0C,0x10,0x00,0xFA,0x00,0x00,0x00,0x00,0x5F,0xFF,0xFF,0xF8,0x00,0x00,
0xFA,0xFE,0x00,0x00,0x03,0xF3,0x3F,0x71,0xF8,0x5F,0xF0,0x0E,0xF8,0x5F,0x50,0x00,
0x0A,0xFA,0x00,0x00,0xAF,0xA0,0x00,0x00,0x03,0xAA,0xCF,0xAA,0xA0,0x00,0x00,0x15,
0x55,0x10,0x00,0x00,0x7E,0x00,0xAF,0xA0,0x0F,0xF5,0x00,0x0A,0xFA,0x00,0x00,0x5F,
0xA0,0x00,0x00,0x00,0x01,0xFF,0x3A,0xFF,0xFF,0xFF,0xF0,0x00,0x00,0xCF,0xA0,0xEF,
0x70,0x0A,0xFA,0x00,0x3F,0x80,0x00,0x1F,0xF1,0x05,0xFF,0x30,0x05,0x50,0xAF,0x70,
0x00,0x00,0x00,0x00,0x00,0x7E,0xF8,0x10,0x00,0x35,0x55,0x55,0x53,0x00,0x01,0x8F,
0xE7,0x00,0x05,0xFF,0x00,0x00,0x05,0xA0,0xAA,0x1E,0xAA,0x1A,0x80,0x00,0x7F,0xFF,
0xFF,0xFF,0x50,0x0F,0xF5,0x00,0x05,0xFF,0x13,0xFF,0x30,0x00,0x00,0x00,0x0A,0xFA,
0x00,0x03,0xFF,0x10,0xAF,0xA0,0x00,0x00,0x00,0xAF,0xA0,0x00,0x00,0x0E,0xF8,0x00,
0x05,0x8F,0xF0,0x0A,0xFA,0x00,0x0A,0xFA,0x00,0xFF,0x50,0x00,0x00,0xFF,0x50,0x0F,
0xF5,0x7F,0xF3,0x00,0x00,0xFF,0x50,0x00,0x00,0x0A,0xF0,0xAF,0xAE,0x0F,0xF5,0x00,
0xFA,0x01,0xFF,0xCF,0x50,0x8F,0xC0,0x00,0x00,0xCF,0x80,0x5F,0xF0,0x00,0x00,0x00,
0xEF,0x70,0x00,0x01,0xFF,0x30,0xFF,0x50,0x5F,0xF1,0x00,0x00,0x00,0x01,0xAF,0xF3,
0x00,0x0A,0xFA,0x00,0x00,0xAF,0xA0,0x00,0x0F,0xF5,0x00,0x0F,0xF7,0xEC,0x00,0x00,
0xAF,0x7F,0x70,0xAF,0x8F,0x50,0x00,0x07,0xFC,0xFF,0x10,0x00,0x00,0xAF,0xA0,0x00,
0x00,0x1F,0xF8,0x00,0x00,0x0A,0xF0,0x00,0x5E,0x00,0x0A,0xF0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0xFE,0xAF,0xF5,0x05,0xFF,0x00,0x0F,0xF5,
0x5F,0xF0,0x00,0x00,0x0F,0xF5,0x00,0xAF,0xA0,0xAF,0xEA,0xAA,0xA3,0x0F,0xF5,0x05,
0xFF,0x00,0x0F,0xF0,0x0A,0xFA,0x00,0xAF,0xA0,0x5F,0xF0,0x0F,0xF5,0x0A,0xFC,0xFF,
0x30,0x05,0xFF,0x00,0xFF,0x50,0x5F,0xF0,0x0F,0xF5,0x0A,0xFA,0x00,0xAF,0xA0,0xAF,
0xA0,0x00,0xFF,0x50,0xFF,0x50,0x0A,0xFA,0x0F,0xF5,0x00,0xAF,0xA0,0x0F,0xF5,0x00,
0x03,0xCF,0xFF,0x80,0x0F,0xF5,0x00,0xFF,0x50,0x0F,0xF5,0x00,0xFF,0x5A,0xF0,0x00,
0xEF,0x5F,0x3F,0xAA,0xC0,0x00,0x0C,0xFE,0x00,0x00,0xAF,0xA8,0xF1,0x00,0x08,0xFE,
0x00,0x03,0x7F,0x30,0x00,0xF5,0x00,0x08,0xE5,0x05,0x70,0x1C,0xFF,0xC0,0x00,0x05,
0x30,0x00,0x00,0x00,0x00,0x5F,0x58,0xC5,0x30,0x00,0x0F,0x5F,0xF0,0x00,0x01,0xE7,
0x05,0xF5,0x0F,0xA5,0xFF,0x10,0x3F,0xFE,0xE0,0x00,0x00,0x7F,0xC0,0x00,0x0C,0xF7,
0x00,0x00,0x00,0x00,0x05,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0xA0,
0x05,0xFC,0x01,0xFF,0x00,0x00,0xAF,0xA0,0x00,0x5F,0xE0,0x00,0x00,0x00,0x00,0x0F,
0xF5,0x35,0x55,0x8F,0xF5,0x00,0x00,0x0A,0xFA,0x0A,0xF7,0x00,0xAF,0xA0,0x0C,0xF3,
0x00,0x05,0xFF,0x00,0x0F,0xF5,0x00,0x00,0x0E,0xF3,0x00,0x00,0x00,0x00,0x00,0x00,
0x07,0xEF,0x81,0x0A,0xFF,0xFF,0xFF,0xA0,0x18,0xFE,0x70,0x00,0x00,0x15,0x50,0x00,
0x00,0x1E,0x07,0xFF,0x5A,0xFF,0xA0,0x00,0x0C,0xC5,0x55,0x5A,0xFA,0x00,0xFF,0x50,
0x00,0x0F,0xF5,0x0E,0xF8,0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0xCF,0xC0,0x0A,0xFA,
0x00,0x00,0x00,0x0A,0xFA,0x00,0x00,0x00,0x8F,0xE0,0x00,0x05,0xFF,0x00,0xAF,0xA0,
0x00,0xAF,0xA0,0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x00,0xFF,0x50,0xAF,0xE1,0x00,0x0F,
0xF5,0x00,0x00,0x00,0xAF,0x07,0xFF,0xA0,0xFF,0x50,0x0F,0xA0,0x08,0xFF,0xF5,0x03,
0xFF,0x10,0x00,0x1F,0xF3,0x05,0xFF,0x00,0x00,0x00,0x08,0xFC,0x00,0x00,0x7F,0xE0,
0x0F,0xF5,0x00,0xAF,0xC0,0x00,0x00,0x00,0x00,0xFF,0x50,0x00,0xAF,0xA0,0x00,0x0A,
0xFC,0x00,0x01,0xFF,0x50,0x00,0xAF,0xEF,0x70,0x00,0x05,0xFF,0xF3,0x08,0xFE,0xF1,
0x00,0x01,0xFA,0x1E,0xFA,0x00,0x00,0x0A,0xFA,0x00,0x00,0x0C,0xFE,0x00,0x00,0x00,
0xAF,0x00,0x01,0xF3,0x00,0xAF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x03,0xFF,0x10,0xFF,0x50,0x5F,0xF0,0x01,0xFF,0x33,0xFF,0x10,0x00,0x00,0xFF,
0x50,0x0C,0xFA,0x0A,0xFA,0x00,0x00,0x00,0xFF,0x50,0x5F,0xF0,0x03,0xFF,0x00,0xAF,
0xA0,0x0A,0xFA,0x05,0xFF,0x00,0xFF,0x50,0xAF,0xA7,0xFC,0x00,0x5F,0xF0,0x0F,0xF5,
0x05,0xFF,0x00,0xFF,0x50,0xAF,0xA0,0x0A,0xFA,0x08,0xFC,0x00,0x1F,0xF3,0x0F,0xF5,
0x00,0xCF,0x80,0xFF,0x50,0x0C,0xFA,0x00,0xFF,0x50,0x00,0x00,0x1A,0xFF,0x50,0xFF,
0x50,0x0F,0xF5,0x01,0xFF,0x50,0x0A,0xFA,0xF8,0x00,0x0A,0xFC,0xE0,0xEE,0xE8,0x00,
0x07,0xFE,0xF8,0x00,0x05,0xFF,0xF8,0x00,0x03,0xFF,0x30,0x00,0x00,0xFA,0x00,0x0F,
0x50,0x00,0xFA,0x00,0x00,0x00,0x05,0x50,0x00,0x01,0x55,0x00,0x00,0x00,0x00,0x05,
0xE0,0xA8,0x00,0x08,0x50,0xFA,0xFC,0x00,0x00,0xAA,0x00,0x3F,0x71,0xF8,0x1F,0xF8,
0x00,0x8F,0xF3,0x00,0x00,0x01,0xFF,0x00,0x00,0xFF,0x10,0x00,0x00,0x00,0x00,0x5F,
0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0xFF,0x50,0xF5,0x00,0x0F,0xF1,0x7F,0xC0,0x00,
0x0A,0xFA,0x00,0x0E,0xFA,0x55,0x53,0x01,0x10,0x03,0xFF,0x10,0x00,0x05,0xFF,0x00,
0x10,0x01,0xEF,0x70,0x3F,0xC0,0x0E,0xF5,0x03,0xFF,0x00,0x00,0x3F,0xF3,0x01,0xFF,
0x11,0x10,0x07,0xFC,0x00,0xAF,0xA0,0x0A,0xFA,0x00,0x00,0x00,0x7E,0xF0,0x00,0x00,
0x00,0x00,0x0F,0xE7,0x00,0x00,0x00,0x01,0x55,0x00,0x00,0x00,0xAA,0x05,0x10,0x05,
0x30,0x00,0x03,0xF7,0x00,0x00,0x3F,0xF1,0x0F,0xF5,0x00,0x08,0xFF,0x30,0x5F,0xF7,
0x00,0x00,0x73,0x0A,0xFC,0x55,0xCF,0xF3,0x00,0xAF,0xA0,0x00,0x00,0x00,0xAF,0xA0,
0x00,0x00,0x01,0xEF,0xA1,0x00,0x5F,0xF0,0x0A,0xFA,0x00,0x0A,0xFA,0x00,0xFF,0x50,
0x00,0x00,0xFF,0x50,0x0F,0xF5,0x00,0xEF,0xE0,0x00,0xFF,0x50,0x00,0x00,0x0A,0xF0,
0x1F,0xF5,0x0F,0xF5,0x00,0xFA,0x00,0x0E,0xFF,0x50,0x0C,0xFA,0x00,0x0A,0xFC,0x00,
0x5F,0xF0,0x00,0x00,0x00,0x1F,0xF5,0x00,0x1E,0xF5,0x00,0xFF,0x50,0x01,0xEF,0x80,
0x35,0x00,0x00,0x5F,0xF1,0x00,0x0A,0xFA,0x00,0x00,0x3F,0xF5,0x00,0x7F,0xF0,0x00,
0x05,0xFF,0xF1,0x00,0x00,0x1F,0xFF,0x00,0x5F,0xFE,0x00,0x00,0xCF,0x10,0x7F,0xF3,
0x00,0x00,0xAF,0xA0,0x00,0x05,0xFF,0x50,0x00,0x00,0x0A,0xF0,0x00,0x0C,0x80,0x0A,
0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5F,0xF1,0x1F,0xF5,
0x05,0xFF,0x00,0x7F,0xE0,0x0E,0xFA,0x00,0x01,0x0A,0xFC,0x07,0xFF,0xA0,0x3F,0xF5,
0x00,0x11,0x0F,0xF5,0x01,0xFF,0xA7,0xEF,0xF0,0x0A,0xFA,0x00,0xAF,0xA0,0x5F,0xF0,
0x0F,0xF5,0x0A,0xFA,0x0E,0xF7,0x05,0xFF,0x00,0xFF,0x50,0x5F,0xF0,0x0F,0xF5,0x0A,
0xFA,0x00,0xAF,0xA0,0x3F,0xF1,0x07,0xFE,0x00,0xFF,0x50,0x1F,0xF3,0x0A,0xFC,0x07,
0xFF,0xA0,0x0F,0xF5,0x00,0x31,0x00,0x1F,0xF5,0x0F,0xF7,0x00,0xFF,0x71,0xEF,0xF5,
0x00,0x3F,0xFF,0x30,0x00,0x5F,0xFA,0x0A,0xFF,0x50,0x01,0xFA,0x5F,0xF3,0x00,0x0E,
0xFF,0x30,0x00,0xEF,0x80,0x00,0x00,0x5F,0x80,0x00,0xF5,0x00,0x0F,0xA0,0x00,0x00,
0x00,0x00,0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0x00,0x8A,0x0E,0x50,0x00,0xAF,0xFF,
0xFE,0x10,0x00,0x5E,0x10,0x00,0xAF,0xFE,0x10,0x5F,0xFC,0xCF,0xEF,0xE0,0x00,0x00,
0x08,0xF7,0x00,0x7F,0x80,0x00,0x00,0x00,0x00,0x03,0xA0,0x00,0x05,0xFF,0x00,0x00,
0x00,0x0F,0xF5,0x5F,0x00,0x00,0x5F,0xCE,0xE1,0x00,0xAF,0xFF,0xFF,0xF0,0xFF,0xFF,
0xFF,0xA0,0x5F,0xCA,0xFF,0x50,0x00,0x00,0x5F,0xF0,0x05,0xEA,0xEF,0xA0,0x00,0xAF,
0xCC,0xFA,0x00,0x8F,0xC0,0x00,0x00,0xAF,0xFA,0xEF,0x50,0x5F,0xAC,0xFE,0x10,0x0A,
0xFA,0x00,0xAF,0xA0,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,
0x00,0x00,0x5F,0xF0,0x00,0x00,0x00,0xAC,0x75,0x7C,0x00,0x00,0x00,0x8F,0x10,0x00,
0x00,0xEF,0x70,0xFF,0xFF,0xFF,0xFF,0x80,0x00,0x7F,0xFE,0xAA,0xFF,0x50,0xAF,0xFF,
0xFF,0xC3,0x00,0x0A,0xFF,0xFF,0xFF,0xFA,0x0A,0xFA,0x00,0x00,0x00,0x03,0xEF,0xFA,
0xAF,0xFF,0x00,0xAF,0xA0,0x00,0xAF,0xA0,0x0F,0xF5,0x00,0x00,0x0F,0xF5,0x00,0xFF,
0x50,0x01,0xEF,0xA0,0x0F,0xFF,0xFF,0xFF,0xA0,0xAF,0x00,0xAA,0x00,0xFF,0x50,0x0F,
0xA0,0x00,0x3F,0xF5,0x00,0x1C,0xFE,0xAE,0xFE,0x10,0x05,0xFF,0x00,0x00,0x00,0x00,
0x5F,0xFC,0xAF,0xFA,0x00,0x0F,0xF5,0x00,0x05,0xFF,0x35,0xFF,0xCA,0xCF,0xF7,0x00,
0x00,0xAF,0xA0,0x00,0x00,0xAF,0xFA,0xCF,0xF5,0x00,0x00,0x0F,0xFC,0x00,0x00,0x00,
0xEF,0xA0,0x00,0xFF,0x80,0x00,0x7F,0x70,0x00,0xCF,0xE0,0x00,0x0A,0xFA,0x00,0x00,
0xAF,0xFF,0xFF,0xFF,0x50,0xAF,0x00,0x00,0x8C,0x00,0xAF,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xEF,0xEE,0xEF,0xE5,0x5F,0xFE,0xAF,0xF3,0x00,
0x5F,0xFE,0xAE,0xA0,0x3F,0xFF,0xEE,0xFA,0x00,0x8F,0xFC,0xCF,0x50,0xFF,0x50,0x05,
0xFF,0xF5,0xFF,0x00,0xAF,0xA0,0x0A,0xFA,0x05,0xFF,0x00,0xFF,0x50,0xAF,0xA0,0x3F,
0xF1,0x5F,0xF0,0x0F,0xF5,0x05,0xFF,0x00,0xFF,0x50,0xAF,0xA0,0x0A,0xFA,0x00,0xAF,
0xEA,0xFE,0x30,0x0F,0xFF,0xAE,0xF8,0x00,0x3F,0xFF,0xEE,0xFA,0x00,0xFF,0x50,0x05,
0xFC,0xAE,0xFA,0x00,0x8F,0xFF,0x08,0xFF,0xFA,0xFF,0x50,0x00,0xEF,0xE0,0x00,0x01,
0xFF,0x50,0x7F,0xF0,0x00,0xCE,0x10,0xAF,0xC0,0x00,0x8F,0xC0,0x00,0x5F,0xFF,0xFF,
0xA0,0x08,0xF5,0x00,0x0F,0x50,0x00,0xCF,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x55,
0x00,0x00,0x00,0x00,0x03,0x10,0x50,0x00,0x00,0x15,0xF8,0x00,0x00,0x0A,0x50,0x00,
0x00,0x55,0x10,0x00,0x18,0xAA,0x50,0x55,0x10,0x00,0x00,0x0E,0xE1,0x1E,0xE0,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0xF0,0x00,0x00,0x00,0x55,0x18,0xA0,0x00,
0x00,0x3A,0x81,0x00,0x03,0x55,0x55,0x55,0x05,0x55,0x55,0x53,0x01,0x7A,0xA8,0x30,
0x00,0x00,0x01,0x55,0x00,0x18,0xAA,0x50,0x00,0x00,0x5A,0xA5,0x00,0x03,0x53,0x00,
0x00,0x00,0x3A,0xA8,0x30,0x01,0x8A,0xA7,0x00,0x00,0x35,0x30,0x03,0xFA,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x55,0x00,0x00,
0x00,0x00,0x3A,0xAA,0x50,0x00,0x00,0x05,0x30,0x00,0x00,0x03,0x53,0x05,0x55,0x55,
0x55,0x10,0x00,0x00,0x17,0xAA,0xA7,0x30,0x03,0x55,0x55,0x10,0x00,0x00,0x35,0x55,
0x55,0x55,0x30,0x35,0x30,0x00,0x00,0x00,0x00,0x5A,0xAA,0xA7,0x50,0x03,0x53,0x00,
0x03,0x53,0x00,0x55,0x17,0x10,0x08,0xFF,0x00,0x05,0x51,0x00,0x03,0x55,0x10,0x55,
0x55,0x55,0x53,0x03,0x50,0x00,0x00,0x05,0x51,0x00,0x53,0x00,0x00,0x55,0x10,0x00,
0x05,0xAA,0xA5,0x00,0x00,0x15,0x50,0x00,0x00,0x00,0x00,0x17,0xAA,0xEF,0x75,0x00,
0x55,0x10,0x00,0x05,0x53,0x15,0xAA,0xAA,0x71,0x00,0x00,0x03,0x53,0x00,0x00,0x00,
0x38,0xAA,0x71,0x00,0x00,0x00,0x35,0x30,0x00,0x00,0x03,0x53,0x00,0x05,0x51,0x00,
0x05,0x50,0x00,0x01,0x55,0x10,0x00,0x35,0x30,0x00,0x03,0x55,0x55,0x55,0x51,0x0A,
0xF0,0x00,0x03,0xF1,0x0A,0xF0,0x00,0x00,0x00,0x00,0x0A,0xAA,0xAA,0xAA,0xA3,0x00,
0x00,0x01,0x8A,0x33,0xAA,0x11,0x55,0x7A,0x81,0x00,0x00,0x17,0xAA,0x83,0x00,0x3A,
0x81,0x35,0x30,0x00,0x38,0xAA,0x71,0x05,0x51,0x00,0x03,0x51,0x1F,0xF0,0x03,0x53,
0x00,0x35,0x30,0x15,0x50,0x0F,0xF5,0x03,0x53,0x00,0x35,0x31,0x55,0x00,0x55,0x10,
0x15,0x50,0x05,0x51,0x03,0x53,0x00,0x35,0x30,0x00,0x3A,0xA7,0x10,0x00,0xFF,0x8A,
0xA3,0x00,0x00,0x3A,0x81,0xAF,0xA0,0x05,0x51,0x00,0x05,0xAA,0xA5,0x00,0x00,0x7A,
0x70,0x08,0xA7,0x05,0x51,0x00,0x03,0x53,0x00,0x00,0x05,0x51,0x01,0x55,0x00,0x15,
0x30,0x01,0x55,0x10,0x07,0xF5,0x00,0x01,0x55,0x55,0x53,0x00,0xAF,0x10,0x00,0xF5,
0x00,0x0C,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x0A,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x1E,0xAA,0xE1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x01,0xA8,0x00,0x00,0x00,0x00,0x00,0xE7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0xFE,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0xFF,0xA0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0xA7,0x00,0x0E,0xAA,0xEF,
0x00,0x00,0x00,0x00,0x00,0xAA,0xAA,0xAA,0xAA,0x30,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x08,0x30,0x1A,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,
0xFF,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xF5,0x00,0x00,0x00,0x00,0x00,0x0A,
0xFA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xEE,
0x00,0x00,0x00,0x00,0x00,0x00,0x05,0xFF,0x70,0x0F,0x50,0x0C,0xFA,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x05,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x10,0x00,0x00,
0x00,0x00,0x1A,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x07,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x35,0x53,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x03,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x07,0xAA,0x70,0x00,0x7A,0xAA,0xA0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
0xFF,0xFC,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xA0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFF,0x50,0x00,0x00,0x00,0x00,0x00,0xAF,0xA0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAF,0x70,0x00,0x00,0x00,0x00,0x00,
0x00,0x01,0x55,0x00,0xA3,0x00,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF
};

