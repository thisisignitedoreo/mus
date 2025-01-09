
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
// Convert a UTF16 string to a UTF8 string from src to dst
// char* src: source buffer
// char* dst: destination buffer
// size_t dst_size: destination buffer size in bytes
// size_t src_size: source buffer size in bytes
//                  note: if 0, read until the U+0000 codepoint
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
//                 UC_BYTE_ORDER_BOM - bom
// bool output_bom: write BOM to output?

void uc_utf8_to_utf16_buffered(char* src, char* dst, size_t dst_size, size_t src_size,
                               bool bom, char output_byteorder);
// Convert a UTF8 string to a UTF16 string from src to dst
// char* src: source buffer
// char* dst: destination buffer
// size_t dst_size: destination buffer size in bytes
// size_t src_size: source buffer size in bytes
//                  note: if 0, read until the U+0000 codepoint
// bool bom: expect BOM from input?
// char output_byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                        UC_BYTE_ORDER_BIG - big endian
//                        UC_BYTE_ORDER_BOM - bom

uint32_t uc_read_utf8_codepoint(char* src, char* read_bytes);
// Read a single UTF8 codepoint from src
// char* src: source pointer (reads two or four bytes from it)
// char* read_bytes: ptr to a char number
//                   written value is a number of read bytes
// returns: a Unicode codepoint

uint32_t uc_read_utf16_codepoint(char* src, char* read_bytes, char byteorder);
// Read a single UTF16 codepoint from src
// char* src: source pointer (reads two or four bytes from it)
// char* read_bytes: ptr to a char number
//                   written value is a number of read bytes
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
// returns: a Unicode codepoint

char uc_write_utf8_codepoint(char* dst, uint32_t codepoint);
// Write a single UTF8 codepoint to dst
// char* dst: destination pointer (writes 1..4 bytes to it)
// uint32_t codepoint: codepoint to write
// returns: number of bytes written

char uc_write_utf16_codepoint(char* dst, uint32_t codepoint, char byteorder);
// Write a single UTF16 codepoint to dst
// char* dst: destination pointer (writes 2 or 4 bytes to it)
// uint32_t codepoint: codepoint to write
// char byteorder: UC_BYTE_ORDER_LITTLE - little endian
//                 UC_BYTE_ORDER_BIG - big endian
// returns: number of bytes written

char uc_write_utf8_bom(char* dst);
// Writes UTF8 BOM marker to dst.
// char* dst: destination buffer (writes 3 bytes to it)
// returns: number of bytes written (3)

char uc_read_utf8_bom(char* src);
// Reads UTF8 BOM marker from src.
// char* src: destination buffer (reads 3 bytes from it)
// returns: number of bytes read (3)

char uc_write_utf16_bom(char* dst, char byteorder);
// Write UTF16 BOM marker to dst
// char* dst: source buffer (reads 2 bytes from it)
// char byteorder: byteorder of the BOM
// returns: amount of bytes written (4)

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

void uc_utf8_to_utf16_buffered(char* src, char* dst, size_t dst_size, size_t src_size,
                               bool bom, char output_byteorder) {
    size_t cur = 0, dst_cur = 0, codepoints_read = 0;
    char output_endianness = output_byteorder;
    
    if (bom) cur += uc_read_utf8_bom(src + cur);
    if (output_endianness == UC_BYTE_ORDER_BOM) {
        output_endianness = UC_BYTE_ORDER_BIG;
        dst_cur += uc_write_utf16_bom(dst + dst_cur, output_endianness);
    }

    uint32_t last_codepoint = -1;
    while (last_codepoint != 0 && (!src_size || cur < src_size) && dst_cur < dst_size-1) {
        char read_bytes = 0;
        uint32_t codepoint = uc_read_utf8_codepoint(src + cur, &read_bytes);
        cur += read_bytes;
        dst_cur += uc_write_utf16_codepoint(dst + dst_cur, codepoint, output_endianness);
        codepoints_read++;
        last_codepoint = codepoint;
    }
    
    if (last_codepoint != 0) uc_write_utf16_codepoint(dst + dst_cur, 0, output_endianness);
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

uint32_t uc_read_utf8_codepoint(char* src, char* read_bytes) {
    *read_bytes = 1;
    if (src[0] < 0x7F) return (uint32_t) src[0];
    *read_bytes = 2;
    if ((src[0] & 0b11100000) == 0b11000000)
        return ((uint32_t) (src[0] & 0b11111) << 6) | ((uint32_t) (src[1] & 0b111111));
    *read_bytes = 3;
    if ((src[0] & 0b11110000) == 0b11100000)
        return ((uint32_t) (src[0] & 0b1111) << 12) | ((uint32_t) (src[1] & 0b111111) << 6) | ((uint32_t) ((src[2] & 0b111111)));
    *read_bytes = 4;
    return ((uint32_t) (src[0] & 0b111) << 18) | ((uint32_t) (src[1] & 0b111111) << 12) | ((uint32_t) (src[2] & 0b111111) << 6) | ((uint32_t) (src[3] & 0b111111));
}

char uc_write_utf16_codepoint(char* dst, uint32_t codepoint, char byteorder) {
    if (codepoint < 0x10000) {
        if (byteorder == UC_BYTE_ORDER_LITTLE) {
            dst[0] = (codepoint >> 0) & 0xFF;
            dst[1] = (codepoint >> 8) & 0xFF;
        } else {
            dst[1] = (codepoint >> 0) & 0xFF;
            dst[0] = (codepoint >> 8) & 0xFF;
        }
        return 2;
    } else {
        codepoint = codepoint - 0x10000;
        uint16_t lo10 = 0xDC00 | (codepoint & 0x3FF);
        uint16_t hi10 = 0xD800 | (codepoint >> 10);
        if (byteorder == UC_BYTE_ORDER_LITTLE) {
            dst[0] = (hi10 >> 0) & 0xFF;
            dst[1] = (hi10 >> 8) & 0xFF;
            dst[2] = (lo10 >> 0) & 0xFF;
            dst[3] = (lo10 >> 8) & 0xFF;
        } else {
            dst[1] = (hi10 >> 0) & 0xFF;
            dst[0] = (hi10 >> 8) & 0xFF;
            dst[3] = (lo10 >> 0) & 0xFF;
            dst[2] = (lo10 >> 8) & 0xFF;
        }
        return 4;
    }
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
    return uc_write_utf8_codepoint(dst, 0xFEFF);
}

char uc_write_utf16_bom(char* dst, char endianness) {
    return uc_write_utf16_codepoint(dst, 0xFEFF, endianness);
}

char uc_read_utf8_bom(char* dst) {
    char bytes_read = 0;
    uint32_t feff = uc_read_utf8_codepoint(dst, &bytes_read);
    if (feff != 0xFEFF) {} // idk, you do
    return bytes_read;
}

char uc_read_utf16_bom(char* src, char* bytes_read) {
    uint16_t feff = uc_read_utf16_codepoint(src, bytes_read, UC_BYTE_ORDER_BIG);
    if (feff == 0xFEFF)      return UC_BYTE_ORDER_BIG;
    else if (feff == 0xFFFE) return UC_BYTE_ORDER_LITTLE;
    return UC_BYTE_ORDER_BIG; // unreachable
}

#endif // UC_IMPL

#endif // UC_H_
