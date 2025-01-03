#ifndef _monofonto6pt_h
#define _monofonto6pt_h

#include <Arduino.h>
#include <GxEPD.h>

const uint8_t monofonto6pt7bBitmaps[] PROGMEM = {
  0x00, 0xFF, 0xF0, 0xC0, 0xFC, 0x28, 0xA2, 0x9F, 0x29, 0x4F, 0x94, 0x50,
  0x47, 0xA5, 0x0E, 0x38, 0x73, 0xF3, 0x10, 0xC6, 0xAA, 0xAC, 0xEC, 0xD5,
  0x55, 0x8C, 0x71, 0x45, 0x1C, 0x63, 0xD9, 0xB6, 0x7C, 0xE0, 0x5A, 0xAA,
  0xAD, 0xA5, 0x55, 0x5E, 0x32, 0x9F, 0xCC, 0x49, 0x20, 0x20, 0x8F, 0xC8,
  0x20, 0xFA, 0xF0, 0xC0, 0x04, 0x20, 0x82, 0x10, 0x42, 0x08, 0x21, 0x04,
  0x20, 0xF9, 0x99, 0x99, 0x99, 0xF0, 0x2F, 0x92, 0x49, 0x20, 0xE9, 0x91,
  0x32, 0x64, 0xF0, 0x31, 0x20, 0x82, 0x18, 0x2C, 0xD2, 0x78, 0x18, 0xE3,
  0x96, 0x59, 0x6F, 0xC6, 0x18, 0xF4, 0x21, 0x0F, 0x48, 0x76, 0xE0, 0x18,
  0xC3, 0x18, 0x79, 0x2C, 0xD2, 0x38, 0xFC, 0x21, 0x86, 0x10, 0xC3, 0x18,
  0x60, 0x31, 0xA4, 0x92, 0x31, 0x2C, 0xD2, 0x78, 0x71, 0xAC, 0xD2, 0x78,
  0x61, 0x0C, 0x60, 0xC0, 0x30, 0xC0, 0xE8, 0x0C, 0xCC, 0x18, 0x18, 0x10,
  0xFC, 0x00, 0x3F, 0xC0, 0xC0, 0xC6, 0x62, 0x00, 0x6F, 0x13, 0x66, 0x00,
  0x60, 0x79, 0x38, 0x6D, 0xB6, 0xF8, 0x10, 0x20, 0x30, 0xC3, 0x0C, 0x79,
  0xE7, 0x92, 0xCC, 0xFB, 0x99, 0xEB, 0x9B, 0xF0, 0xF9, 0x88, 0x88, 0x8F,
  0xF0, 0xFB, 0x99, 0x99, 0x99, 0xF0, 0xF8, 0x88, 0xF8, 0x88, 0xF0, 0xF8,
  0x88, 0xF8, 0x88, 0x80, 0xFD, 0x88, 0xB9, 0x99, 0xF0, 0x99, 0x99, 0xF9,
  0x99, 0x90, 0xF6, 0x66, 0x66, 0x66, 0xF0, 0x31, 0x11, 0x11, 0x13, 0xE0,
  0x9D, 0xA9, 0x4E, 0x52, 0xD6, 0x98, 0x88, 0x88, 0x88, 0x88, 0xF0, 0xCF,
  0x3F, 0xFF, 0xFF, 0xFD, 0xF3, 0xCC, 0x9D, 0xDD, 0xFB, 0xBB, 0x90, 0x79,
  0x2C, 0xF3, 0xCF, 0x3C, 0xD2, 0x78, 0xF5, 0xA7, 0x39, 0x7A, 0x10, 0x80,
  0x79, 0x2C, 0xF3, 0xCF, 0x3C, 0xD2, 0x78, 0x60, 0x80, 0xF5, 0xA7, 0x2F,
  0x72, 0x96, 0x98, 0xF4, 0xA1, 0x87, 0x18, 0x7E, 0xF0, 0xFC, 0xC3, 0x0C,
  0x30, 0xC3, 0x0C, 0x30, 0xCF, 0x3C, 0xF3, 0xCF, 0x3C, 0xD2, 0x78, 0xCD,
  0x24, 0x9A, 0x79, 0xE3, 0x0C, 0x30, 0xCF, 0x3D, 0xDE, 0x79, 0xE7, 0x9E,
  0x48, 0xCD, 0xE7, 0x8C, 0x30, 0xC7, 0x9E, 0xCC, 0xCD, 0x27, 0x9E, 0x30,
  0xC3, 0x0C, 0x30, 0xFC, 0x61, 0x84, 0x30, 0x86, 0x18, 0xFC, 0xEA, 0xAA,
  0xAB, 0x81, 0x04, 0x10, 0x20, 0x81, 0x04, 0x10, 0x20, 0x81, 0xD5, 0x55,
  0x57, 0x30, 0xC4, 0xB3, 0xFC, 0xD0, 0xF9, 0xF9, 0xBF, 0x84, 0x21, 0x6D,
  0x4E, 0x7A, 0xB0, 0x3B, 0x71, 0x86, 0x9C, 0x08, 0x42, 0xD5, 0xE7, 0x2B,
  0x68, 0x79, 0x2F, 0xF0, 0x49, 0xE0, 0x7A, 0x11, 0xE4, 0x21, 0x08, 0x40,
  0x7A, 0xF3, 0x95, 0xBF, 0x6F, 0x88, 0x8B, 0xD9, 0x99, 0x90, 0x60, 0x0E,
  0x66, 0x66, 0xF0, 0x30, 0x0F, 0x33, 0x33, 0x32, 0xE0, 0x88, 0x8B, 0xAE,
  0xEB, 0xB0, 0xE4, 0x92, 0x49, 0x20, 0xEB, 0xFE, 0xFB, 0xEF, 0xB0, 0xBD,
  0x99, 0x99, 0x31, 0xEC, 0xF3, 0x68, 0xC0, 0xFE, 0xF3, 0x9D, 0xFF, 0x18,
  0x6A, 0xF3, 0x95, 0xB4, 0x21, 0xBD, 0x88, 0x88, 0xEB, 0xC3, 0x9F, 0x44,
  0xF4, 0x44, 0x47, 0x99, 0x99, 0x9F, 0xCD, 0x26, 0x9E, 0x30, 0xC0, 0xDF,
  0x77, 0x9E, 0x79, 0xE0, 0x49, 0xE3, 0x0C, 0x7B, 0x30, 0xCD, 0x26, 0x9E,
  0x30, 0xC3, 0x08, 0x78, 0xCC, 0x46, 0x7C, 0x6D, 0x25, 0x92, 0x49, 0x90,
  0xFF, 0xE0, 0xD9, 0x24, 0xD2, 0x4B, 0x40, 0xE6, 0xF0 };

const GFXglyph monofonto6pt7bGlyphs[] PROGMEM = {
  {     0,   1,   1,   6,    0,    0 },   // 0x20 ' '
  {     1,   2,   9,   6,    2,   -8 },   // 0x21 '!'
  {     4,   2,   3,   6,    2,   -8 },   // 0x22 '"'
  {     5,   6,   9,   6,    0,   -8 },   // 0x23 '#'
  {    12,   5,  11,   6,    1,   -9 },   // 0x24 '$'
  {    19,   6,   9,   6,    0,   -8 },   // 0x25 '%'
  {    26,   6,   9,   6,    0,   -8 },   // 0x26 '&'
  {    33,   1,   3,   6,    3,   -8 },   // 0x27 '''
  {    34,   2,  12,   6,    3,   -9 },   // 0x28 '('
  {    37,   2,  12,   6,    1,   -9 },   // 0x29 ')'
  {    40,   6,   6,   6,    0,   -8 },   // 0x2A '*'
  {    45,   6,   5,   6,    0,   -6 },   // 0x2B '+'
  {    49,   2,   4,   6,    2,   -1 },   // 0x2C ','
  {    50,   4,   1,   6,    1,   -4 },   // 0x2D '-'
  {    51,   2,   1,   6,    2,    0 },   // 0x2E '.'
  {    52,   6,  12,   6,    0,   -9 },   // 0x2F '/'
  {    61,   4,   9,   6,    1,   -8 },   // 0x30 '0'
  {    66,   3,   9,   6,    1,   -8 },   // 0x31 '1'
  {    70,   4,   9,   6,    1,   -8 },   // 0x32 '2'
  {    75,   6,   9,   6,    0,   -8 },   // 0x33 '3'
  {    82,   6,   9,   6,    0,   -8 },   // 0x34 '4'
  {    89,   5,   9,   6,    1,   -8 },   // 0x35 '5'
  {    95,   6,   9,   6,    0,   -8 },   // 0x36 '6'
  {   102,   6,   9,   6,    0,   -8 },   // 0x37 '7'
  {   109,   6,   9,   6,    0,   -8 },   // 0x38 '8'
  {   116,   6,   9,   6,    0,   -8 },   // 0x39 '9'
  {   123,   2,   6,   6,    2,   -5 },   // 0x3A ':'
  {   125,   2,   7,   6,    2,   -5 },   // 0x3B ';'
  {   127,   6,   6,   6,    0,   -6 },   // 0x3C '<'
  {   132,   6,   4,   6,    0,   -5 },   // 0x3D '='
  {   135,   6,   6,   6,    0,   -6 },   // 0x3E '>'
  {   140,   4,   9,   6,    1,   -8 },   // 0x3F '?'
  {   145,   6,   9,   6,    0,   -6 },   // 0x40 '@'
  {   152,   6,   9,   6,    0,   -8 },   // 0x41 'A'
  {   159,   4,   9,   6,    1,   -8 },   // 0x42 'B'
  {   164,   4,   9,   6,    1,   -8 },   // 0x43 'C'
  {   169,   4,   9,   6,    1,   -8 },   // 0x44 'D'
  {   174,   4,   9,   6,    1,   -8 },   // 0x45 'E'
  {   179,   4,   9,   6,    1,   -8 },   // 0x46 'F'
  {   184,   4,   9,   6,    1,   -8 },   // 0x47 'G'
  {   189,   4,   9,   6,    1,   -8 },   // 0x48 'H'
  {   194,   4,   9,   6,    1,   -8 },   // 0x49 'I'
  {   199,   4,   9,   6,    1,   -8 },   // 0x4A 'J'
  {   204,   5,   9,   6,    1,   -8 },   // 0x4B 'K'
  {   210,   4,   9,   6,    1,   -8 },   // 0x4C 'L'
  {   215,   6,   9,   6,    0,   -8 },   // 0x4D 'M'
  {   222,   4,   9,   6,    1,   -8 },   // 0x4E 'N'
  {   227,   6,   9,   6,    0,   -8 },   // 0x4F 'O'
  {   234,   5,   9,   6,    1,   -8 },   // 0x50 'P'
  {   240,   6,  11,   6,    0,   -8 },   // 0x51 'Q'
  {   249,   5,   9,   6,    1,   -8 },   // 0x52 'R'
  {   255,   5,   9,   6,    1,   -8 },   // 0x53 'S'
  {   261,   6,   9,   6,    0,   -8 },   // 0x54 'T'
  {   268,   6,   9,   6,    0,   -8 },   // 0x55 'U'
  {   275,   6,   9,   6,    0,   -8 },   // 0x56 'V'
  {   282,   6,   9,   6,    0,   -8 },   // 0x57 'W'
  {   289,   6,   9,   6,    0,   -8 },   // 0x58 'X'
  {   296,   6,   9,   6,    0,   -8 },   // 0x59 'Y'
  {   303,   6,   9,   6,    0,   -8 },   // 0x5A 'Z'
  {   310,   2,  12,   6,    3,   -9 },   // 0x5B '['
  {   313,   6,  12,   6,    0,   -9 },   // 0x5C '\'
  {   322,   2,  12,   6,    1,   -9 },   // 0x5D ']'
  {   325,   6,   4,   6,    0,   -8 },   // 0x5E '^'
  {   328,   6,   1,   6,    0,    2 },   // 0x5F '_'
  {   329,   2,   2,   6,    2,   -8 },   // 0x60 '`'
  {   330,   4,   6,   6,    1,   -5 },   // 0x61 'a'
  {   333,   5,   9,   6,    1,   -8 },   // 0x62 'b'
  {   339,   5,   6,   6,    0,   -5 },   // 0x63 'c'
  {   343,   5,   9,   6,    0,   -8 },   // 0x64 'd'
  {   349,   6,   6,   6,    0,   -5 },   // 0x65 'e'
  {   354,   5,   9,   6,    1,   -8 },   // 0x66 'f'
  {   360,   5,   8,   6,    0,   -5 },   // 0x67 'g'
  {   365,   4,   9,   6,    1,   -8 },   // 0x68 'h'
  {   370,   4,   9,   6,    1,   -8 },   // 0x69 'i'
  {   375,   4,  11,   6,    1,   -8 },   // 0x6A 'j'
  {   381,   4,   9,   6,    1,   -8 },   // 0x6B 'k'
  {   386,   3,   9,   6,    1,   -8 },   // 0x6C 'l'
  {   390,   6,   6,   6,    0,   -5 },   // 0x6D 'm'
  {   395,   4,   6,   6,    1,   -5 },   // 0x6E 'n'
  {   398,   6,   6,   6,    0,   -5 },   // 0x6F 'o'
  {   403,   5,   8,   6,    0,   -5 },   // 0x70 'p'
  {   408,   5,   8,   6,    0,   -5 },   // 0x71 'q'
  {   413,   4,   6,   6,    1,   -5 },   // 0x72 'r'
  {   416,   4,   6,   6,    1,   -5 },   // 0x73 's'
  {   419,   4,   8,   6,    1,   -7 },   // 0x74 't'
  {   423,   4,   6,   6,    1,   -5 },   // 0x75 'u'
  {   426,   6,   6,   6,    0,   -5 },   // 0x76 'v'
  {   431,   6,   6,   6,    0,   -5 },   // 0x77 'w'
  {   436,   6,   6,   6,    0,   -5 },   // 0x78 'x'
  {   441,   6,   8,   6,    0,   -5 },   // 0x79 'y'
  {   447,   5,   6,   6,    0,   -5 },   // 0x7A 'z'
  {   451,   3,  12,   6,    2,   -9 },   // 0x7B '{'
  {   456,   1,  11,   6,    3,   -8 },   // 0x7C '|'
  {   458,   3,  12,   6,    1,   -9 },   // 0x7D '}'
  {   463,   6,   2,   6,    0,   -5 } }; // 0x7E '~'

const GFXfont monofonto6pt7b PROGMEM = {
  (uint8_t  *)monofonto6pt7bBitmaps,
  (GFXglyph *)monofonto6pt7bGlyphs,
  0x20, 0x7E, 14 };

// Approx. 1137 bytes

#endif