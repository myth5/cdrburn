/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/**
 * Utility functions for the Libisofs library.
 */

#ifndef __ISO_UTIL
#define __ISO_UTIL

#include <stdint.h>

/**
 * Checks if the given character is a valid d-character.
 */
int valid_d_char(char c);

/**
 * Checks if the given character is a valid a-character.
 */
int valid_a_char(char c);

/**
 * Checks if the given character is part of the Joliet Allowed Character Set.
 */
int valid_j_char(char msb, char lsb);

/**
 * Checks if the given character is part of the POSIX Portable Filename Character Set.
 */
int valid_p_char(char c);

/**
 * Build a string of d-characters of maximum size 'size', optionally padded with spaces if necessary.
 * Can be used to create directory identifiers if padding is turned off.
 */
char *iso_d_str(const char *src, int size, int pad);

/**
 * Build a string of a-characters of maximum size 'size', padded with spaces if necessary.
 */
char *iso_a_str(const char *src, int size);

/**
 * Build a string of j-characters of maximum size 'size', padded with NULLs if necessary.
 * Requires the locale to be set correctly.
 * @return NULL if the conversion from the current codeset to UCS-2BE is not available.
 */
uint16_t *iso_j_str(const char *src);

/**
 * Create a level 1 file identifier that consists of a name, extension and version number.
 * The resulting string will have a file name of maximum length 8, followed by a separator (.),
 * an optional extension of maximum length 3, followed by a separator (;) and a version number (digit 1).
 * @return NULL if the original name and extension both are of length 0.
 */
char *iso_1_fileid(const char *src);

/**
 * Create a level 2 file identifier that consists of a name, extension and version number.
 * The combined file name and extension length will not exceed 30, the name and extension will be separated (.),
 * and the extension will be followed by a separator (;) and a version number (digit 1).
 * @return NULL if the original name and extension both are of length 0.
 */
char *iso_2_fileid(const char *src);

/**
 * Create a Joliet file or directory identifier that consists of a name, extension and version number.
 * The combined name and extension length will not exceed 128 bytes, the name and extension will be separated (.),
 * and the extension will be followed by a separator (;) and a version number (digit 1).
 * All characters consist of 2 bytes and the resulting string is NULL-terminated by a 2-byte NULL.
 * Requires the locale to be set correctly.
 * @param size will be set to the size (in bytes) of the identifier.
 * @return NULL if the original name and extension both are of length 0 or the conversion from the current codeset to UCS-2BE is not available.
 */
uint16_t *iso_j_id(char *src);

/**
 * Create a POSIX portable file name that consists of a name and extension.
 * The resulting file name will not exceed 250 characters.
 * @return NULL if the original name and extension both are of length 0.
 */
char *iso_p_filename(const char *src);

/**
 * Create a POSIX portable directory name.
 * The resulting directory name will not exceed 250 characters.
 * @return NULL if the original name is of length 0.
 */
char *iso_p_dirname(const char *src);

/**
 * Build a pathname out of a directory and file name.
 */
char *iso_pathname(const char *dir, const char *file);

#include <time.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/** Copy a string of a-characters, padding the unused part of the
    destination buffer */
void iso_a_strcpy(unsigned char *dest, int size, const char *src);

/** Copy a string of d-characters, padding the unused part of the
    destination buffer */
void iso_d_strcpy(unsigned char *dest, int size, const char *src);

/** Returns a null terminated string containing the first 'size' a-characters
    from the source */
char *iso_a_strndup(const char *src, int size);

/** Returns a null terminated string containing the first 'size' d-characters
    from the source */
char *iso_d_strndup(const char *src, int size);

char *iso_strconcat(char sep, const char *a, const char *b);

char *iso_strdup(const char *src);

void iso_lsb(uint8_t *buf, uint32_t num, int bytes);
void iso_msb(uint8_t *buf, uint32_t num, int bytes);
void iso_bb(uint8_t *buf, uint32_t num, int bytes);

uint32_t iso_read_lsb(const uint8_t *buf, int bytes);
uint32_t iso_read_msb(const uint8_t *buf, int bytes);
uint32_t iso_read_bb(const uint8_t *buf, int bytes);

/** Records the date/time into a 7 byte buffer (9.1.5) */
void iso_datetime_7(unsigned char *buf, time_t t);

/** Records the date/time into a 17 byte buffer (8.4.26.1) */
void iso_datetime_17(unsigned char *buf, time_t t);

time_t iso_datetime_read_7(const uint8_t *buf);
time_t iso_datetime_read_17(const uint8_t *buf);

/** Removes the extension from the name, and returns the extension. The
    returned extension should _not_ be freed! */
void iso_split_filename(char *name, char **ext);

void iso_filecpy(unsigned char *buf, int size, const char *name, int version);

int iso_filename_len(const char *name, int iso_level);

/* replacement for strlen for joliet names (wcslen doesn't work on machines
 * with 32-bit wchar_t */
size_t ucslen(const uint16_t *);

/* replacement for strcmp for joliet names */
int ucscmp(const uint16_t*, const uint16_t*);

#define DIV_UP(n,div) ( ((n)+(div)-1) / (div) )
#define ROUND_UP(n,mul) ( DIV_UP(n,mul) * (mul) )

#endif /* __ISO_UTIL */
