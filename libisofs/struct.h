/* vim: set noet ts=8 sts=8 sw=8 : */

/**
 * \file struct.h
 * Add functionality similar to the python "struct" module to make it easier
 * to read and write .iso structures.
 *
 * The following conversions are supported:
 * B	uint8_t, the arg should be (uint8_t*)
 * H	uint16_t, the arg should be (uint16_t*)
 * L	uint32_t, the arg should be (uint32_t*)
 * S	a 7-byte timestamp, the arg should be (time_t*)
 * T	a 17-byte timestamp, the arg should be (time_t*)
 * x	ignored field, no arg should be specified
 *
 * Any of the first 3 conversions may be preceded by a endian specifier:
 * <	little-endian
 * >	big-endian
 * =	both-endian (ie. according to ecma119 7.2.3 or 7.3.3)
 *
 * Each conversion specifier may also be preceded by a length specifier. For
 * example, "<5L" specifies an array of 5 little-endian 32-bit integers. Note
 * that "=L" takes 8 bytes while "<L" and ">L" each take 4.
 *
 * You can use a lower-case conversion specifier instead of an upper-case one
 * to signify that the (multi-element) conversion should stop when a zero is
 * reached. This is useful for writing out NULL-terminated strings. Note that
 * this has no effect when unpacking data from a struct.
 */

#ifndef __ISO_STRUCT
#define __ISO_STRUCT

#include <stdint.h>

/**
 * Unpack a struct into its components. The list of components is a list of
 * pointers to the variables to write.
 *
 * For example:
 * uint8_t byte1, byte2;
 * uint16_t uint;
 * iso_struct_unpack("BB=H", data, &byte1, &byte2, &uint);
 *
 * \return The number of conversions performed, or -1 on error.
 */
int iso_struct_unpack(const char *fmt, const uint8_t *data, ...);

/**
 * Write out a struct from its components. The list of components is a list of
 * pointers to the variables to write and the buffer to which to write
 * is assumed to be large
 * enough to take the data.
 *
 * \return The number of conversions performed, or -1 on error.
 */
int iso_struct_pack(const char *fmt, uint8_t *data, ...);

/**
 * Achieves the same effect as iso_struct_pack(), but the format is passed as
 * a sequence of (int, char, char) triples. This list is terminated by
 * (0, 0, 0) and the list of parameters follows.
 *
 * Example: iso_struct_pack_long(data, 4, '=', 'H', 0, 0, 0, &val) is the same
 * as iso_struct_pack("4=H", 0, 0, 0, &val)
 */
int iso_struct_pack_long(uint8_t *data, ...);

/**
 * Calculate the size of a given format string.
 *
 * \return The sum of the length of all formats in the string, in bytes. Return
 *	  -1 on error.
 */
int iso_struct_calcsize(const char *fmt);

#endif
