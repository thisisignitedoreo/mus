
// uc.h
// General library to interact with several Unicode format strings.
// Features:
// 1. Conversion (UTF16 <-> UTF8; UTF32 <-> UTF8; BOM and zero-termination support)
// 2. Get codepoints from encoded strings (UTF8; UTF16; UTF32)
// Copyleft, 2024, aciddev_

#ifndef UC_H_
#define UC_H_

#include <stdint.h>

#define UC_BYTE_ORDER_LITTLE 0
#define UC_BYTE_ORDER_BIG 1
#define UC_BYTE_ORDER_BOM 2

void uc_utf16_to_utf8_buffered(char* src, char* dst, size_t dst_size, size_t src_size,
                               char byteorder, bool output_bom);
// Convert a UTF16 string from src to dst
// char* src: source buffer
// char* dst: destination buffer
// size_t dst_size: destination buffer size in bytes
// size_t src_size: source buffer size in bytes
//                  note: if 0, read until the U+0000 codepoint
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
//                 UC_BYTE_ORDER_BOM - bom
// bool output_bom: write BOM to output?

uint32_t uc_read_utf16_codepoint(char* src, char* read_bytes, char byteorder);
// Read a single UTF16 codepoint from src
// char* src: source pointer (reads two or four bytes from it)
// char* read_bytes: ptr to a char number
//                   written value is a number of read bytes
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
// returns: a Unicode codepoint

char uc_write_utf8_codepoint(char* dst, uint32_t codepoint);
// Write a single UTF8 codepoint to src
// char* dst: destination pointer (writes 1..4 bytes to it)
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
// char* read_bytes: ptr to a char number
//                   written value is a number of read bytes
// returns: number of bytes written

char uc_write_utf8_bom(char* dst);
// Writes UTF8 BOM marker to dst.
// char* dst: destination buffer (writes 3 bytes to it)
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
// returns: number of bytes written (3)

char uc_read_utf16_bom(char* src, char* bytes_read);
// Reads UTF16 BOM marker from src
// char* src: source buffer (reads 2 bytes from it)
// char* bytes_read: ptr to char
//                   value written is amount of bytes read from src
// returns: endianness (UC_BYTE_ORDER_(LITTLE|BIG))

#ifdef UC_IMPL

void uc_utf16_to_utf8_buffered(char* src, char* dst, size_t dst_size, size_t src_size,
                               char byteorder, bool output_bom) {
    size_t cur = 0, dst_cur = 0, codepoints_read = 0;
                                   
    char endianness = byteorder;
    if (endianness == UC_BYTE_ORDER_BOM) {
        char bytes_read = 0;
        endianness = uc_read_utf16_bom(src + cur, &bytes_read);
        cur += bytes_read;
    }
    if (output_bom) dst_cur += uc_write_utf8_bom(dst + dst_cur);

    uint32_t last_codepoint = -1;
    while (last_codepoint != 0 && (!src_size || cur < src_size) && dst_cur < dst_size-1) {
        char read_bytes = 0;
        uint32_t codepoint = uc_read_utf16_codepoint(src + cur, &read_bytes, endianness);
        cur += read_bytes;
        dst_cur += uc_write_utf8_codepoint(dst + dst_cur, codepoint);
        codepoints_read++;
        last_codepoint = codepoint;
    }
    
    if (last_codepoint != 0) uc_write_utf8_codepoint(dst + dst_cur, 0);
}

uint32_t uc_read_utf16_codepoint(char* src, char* read_bytes, char byteorder) {
    uint16_t codepoint = 0;
    *read_bytes = 2;
    if (byteorder == UC_BYTE_ORDER_LITTLE) codepoint = src[0] | (src[1] << 8);
    if (byteorder == UC_BYTE_ORDER_BIG)    codepoint = src[1] | (src[0] << 8);

    if (codepoint < 0xD800 || codepoint > 0xDFFF) return codepoint;
    else if (codepoint >= 0xDC00) return 0;
    *read_bytes = 4;
    
    codepoint = (codepoint & 0x3FF) << 10;
    uint16_t following_codepoint = 0;
    if (byteorder == UC_BYTE_ORDER_LITTLE) following_codepoint = src[2] | (src[3] << 8);
    if (byteorder == UC_BYTE_ORDER_BIG)    following_codepoint = src[3] | (src[2] << 8);
    if (following_codepoint < 0xDC00 || following_codepoint > 0xDFFF) return 0;

    return (uint32_t) codepoint | (following_codepoint & 0x3FF);
}

char uc_write_utf8_codepoint(char* dst, uint32_t codepoint) {
    if (codepoint < 0x7F) {
        dst[0] = codepoint;
        return 1;
    } else if (codepoint < 0x7FF) {
        dst[0] = 0b11000000 | (codepoint >> 6);
        dst[1] = 0b10000000 | (codepoint & 0b111111);
        return 2;
    } else if (codepoint < 0xFFFF) {
        dst[0] = 0b11100000 | (codepoint >> 12);
        dst[1] = 0b10000000 | (codepoint >> 6 & 0b111111);
        dst[2] = 0b10000000 | (codepoint & 0b111111);
        return 3;
    } else {
        dst[0] = 0b11110000 | (codepoint >> 18);
        dst[1] = 0b10000000 | (codepoint >> 12 & 0b111111);
        dst[2] = 0b10000000 | (codepoint >> 6 & 0b111111);
        dst[3] = 0b10000000 | (codepoint & 0b111111);
        return 4;
    }
}

char uc_write_utf8_bom(char* dst) {
    dst[0] = 0xFE;
    dst[1] = 0xBB;
    dst[2] = 0xBF;
    return 3;
}

char uc_read_utf16_bom(char* src, char* bytes_read) {
    uint16_t feff = uc_read_utf16_codepoint(src, bytes_read, UC_BYTE_ORDER_BIG);
    if (feff == 0xFEFF)      return UC_BYTE_ORDER_BIG;
    else if (feff == 0xFFFE) return UC_BYTE_ORDER_LITTLE;
    return UC_BYTE_ORDER_BIG; // unreachable
}

#endif // UC_IMPL

#endif // UC_H_
