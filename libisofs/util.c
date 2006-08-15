/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set ts=8 sts=8 sw=8 noet : */

#include <ctype.h>
#include <iconv.h>
#include <stdlib.h>
#include <stdint.h>
#include <langinfo.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include "util.h"

int valid_d_char(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c == '_');
}

int valid_a_char(char c)
{
	return (c >= ' ' && c <= '"') || (c >= '%' && c <= '?') || (c >= 'A' &&
								    c <= 'Z')
		|| (c == '_');
}

int valid_j_char(char msb, char lsb)
{
	return !(msb == '\0' &&
		 (lsb < ' ' || lsb == '*' || lsb == '/' || lsb == ':' ||
		  lsb == ';' || lsb == '?' || lsb == '\\'));
}

int valid_p_char(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' &&
								    c <= 'z')
		|| (c == '.') || (c == '_') || (c == '-');
}

char *iso_d_str(const char *src, int size, int pad)
{
	char *dest = strlen(src) > size ||
		pad ? malloc(size + 1) : malloc(strlen(src) + 1);
	int i;

	/* Try converting to upper-case before validating. */
	for (i = 0; i < size && src[i]; i++) {
		char c = toupper(src[i]);

		dest[i] = valid_d_char(c) ? c : '_';
	}

	/* Optionally pad with spaces and terminate with NULL. */
	if (pad)
		while (i < size)
			dest[i++] = ' ';
	dest[i] = '\0';

	return dest;
}

char *iso_a_str(const char *src, int size)
{
	char *dest = malloc(size + 1);
	int i;

	for (i = 0; i < size && src[i]; i++) {
		char c = toupper(src[i]);

		dest[i] = valid_a_char(c) ? c : '_';
	}
	while (i < size)
		dest[i++] = ' ';
	dest[i] = '\0';

	return dest;
}

uint16_t *iso_j_str(const char *src)
{
	int size = strlen(src) * 2;
	uint16_t *dest = malloc(size + 2);
	char *cpy, *in, *out;
	size_t inleft, outleft;
	iconv_t cd;

	/* If the conversion is unavailable, return NULL. Obviously,
	   nl_langinfo(..) requires the locale to be set correctly. */
	cd = iconv_open("UCS-2BE", nl_langinfo(CODESET));
	if (cd == (iconv_t) - 1) {
		free(dest);
		return NULL;
	}

	/* In, out, inleft and outleft will be updated by iconv, that's why we 
	   need separate variables. */
	cpy = strdup(src); /* iconv doesn't take const * chars, so we need our
			    * own copy. */
	in = cpy;
	out = (char*)dest;
	inleft = strlen(cpy);
	outleft = size;
	iconv(cd, &in, &inleft, &out, &outleft);

	/* Since we need to pad with NULLs, we can pad up to and including the 
	   terminating NULLs. */
	outleft += 2;
	while (outleft) {
		*out++ = '\0';
		outleft--;
	}
	iconv_close(cd);
	free(cpy);

	return dest;
}

char *iso_1_fileid(const char *src)
{
	char *dest = malloc(15);	/* 15 = 8 (name) + 1 (.) + 3 (ext) + 2 
					   (;1) + 1 (\0) */
	char *dot = strrchr(src, '.');	/* Position of the last dot in the
					   filename, will be used to calculate 
					   lname and lext. */
	int lname, lext, pos, i;

	lext = dot ? strlen(dot + 1) : 0;
	lname = strlen(src) - lext - (dot ? 1 : 0);

	/* If we can't build a filename, return NULL. */
	if (lname == 0 && lext == 0) {
		free(dest);
		return NULL;
	}

	pos = 0;
	/* Convert up to 8 characters of the filename. */
	for (i = 0; i < lname && i < 8; i++) {
		char c = toupper(src[i]);

		dest[pos++] = valid_d_char(c) ? c : '_';
	}
	/* This dot is mandatory, even if there is no extension. */
	dest[pos++] = '.';
	/* Convert up to 3 characters of the extension, if any. */
	for (i = 0; i < lext && i < 3; i++) {
		char c = toupper(src[lname + 1 + i]);

		dest[pos++] = valid_d_char(c) ? c : '_';
	}
	/* File versions are mandatory, even if they aren't used. */
	dest[pos++] = ';';
	dest[pos++] = '1';
	dest[pos] = '\0';
	dest = (char *)realloc(dest, pos + 1);

	return dest;
}

char *iso_2_fileid(const char *src)
{
	char *dest = malloc(34);	/* 34 = 30 (name + ext) + 1 (.) + 2
					   (;1) + 1 (\0) */
	char *dot = strrchr(src, '.');
	int lname, lext, lnname, lnext, pos, i;

	/* Since the maximum length can be divided freely over the name and
	   extension, we need to calculate their new lengths (lnname and
	   lnext). If the original filename is too long, we start by trimming
	   the extension, but keep a minimum extension length of 3. */
	if (dot == NULL || dot == src || *(dot + 1) == '\0') {
		lname = strlen(src);
		lnname = (lname > 30) ? 30 : lname;
		lext = lnext = 0;
	} else {
		lext = strlen(dot + 1);
		lname = strlen(src) - lext - 1;
		lnext = (strlen(src) > 31 &&
			 lext > 3) ? (lname < 27 ? 30 - lname : 3) : lext;
		lnname = (strlen(src) > 31) ? 30 - lnext : lname;
	}

	if (lnname == 0 && lnext == 0) {
		free(dest);
		return NULL;
	}

	pos = 0;
	/* Convert up to lnname characters of the filename. */
	for (i = 0; i < lnname; i++) {
		char c = toupper(src[i]);

		dest[pos++] = valid_d_char(c) ? c : '_';
	}
	dest[pos++] = '.';
	/* Convert up to lnext characters of the extension, if any. */
	for (i = 0; i < lnext; i++) {
		char c = toupper(src[lname + 1 + i]);

		dest[pos++] = valid_d_char(c) ? c : '_';
	}
	dest[pos++] = ';';
	dest[pos++] = '1';
	dest[pos] = '\0';
	dest = (char *)realloc(dest, pos + 1);

	return dest;
}

uint16_t *iso_j_id(char *src)
{
	char *dest = malloc(136);	/* 136 = 128 (name + ext) + 2 (\0 .) + 
					   4 (\0 ; \0 1) + 2 (\0 \0) */
	char *dot = strrchr(src, '.');
	int lname, lext, lcname, lcext, lnname, lnext, pos, i;
	size_t inleft, outleft;
	char *cname, *cext, *in, *out;
	iconv_t cd;

	if (dot == NULL || dot == src || *(dot + 1) == '\0') {
		lname = strlen(src);
		lext = 0;
	} else {
		lext = strlen(dot + 1);
		lname = strlen(src) - lext - 1;
	}

	if (lname == 0 && lext == 0) {
		free(dest);
		return NULL;
	}

	cd = iconv_open("UCS-2BE", nl_langinfo(CODESET));
	if (cd == (iconv_t) - 1) {
		free(dest);
		return NULL;
	}

	/* We need to convert the name and extension first, in order to
	   calculate the number of characters they have. */
	cname = malloc(lname * 2);
	in = src;
	out = cname;
	inleft = lname;
	outleft = lname * 2;
	iconv(cd, &in, &inleft, &out, &outleft);
	lcname = (lname * 2) - outleft;
	iconv_close(cd);

	if (lext) {
		cd = iconv_open("UCS-2BE", nl_langinfo(CODESET));
		if (cd == (iconv_t) - 1) {
			free(dest);
			free(cname);
			return NULL;
		}
		cext = malloc(lext * 2);
		in = dot + 1;
		out = cext;
		inleft = lext;
		outleft = lext * 2;
		iconv(cd, &in, &inleft, &out, &outleft);
		lcext = (lext * 2) - outleft;
		iconv_close(cd);
	} else
		lcext = 0;

	/* Again, divide the available characters over name and extension, but 
	   keep a minimum of three characters for the new extension. */
	lnext = (lcname + lcext > 128 &&
		 lcext > 6) ? (lcname < 122 ? 128 - lcname : 6) : lcext;
	lnname = (lcname + lcext > 128) ? 128 - lnext : lcname;

	pos = 0;
	/* Copy up to lnname bytes from the converted filename. */
	for (i = 0; i < lnname; i = i + 2)
		if (valid_j_char(cname[i], cname[i + 1])) {
			dest[pos++] = cname[i];
			dest[pos++] = cname[i + 1];
		} else {
			dest[pos++] = '\0';
			dest[pos++] = '_';
		}
	/* Dot is now a 16 bit separator. */
	dest[pos++] = '\0';
	dest[pos++] = '.';
	/* Copy up to lnext bytes from the converted extension, if any. */
	for (i = 0; i < lnext; i = i + 2)
		if (valid_j_char(cext[i], cext[i + 1])) {
			dest[pos++] = cext[i];
			dest[pos++] = cext[i + 1];
		} else {
			dest[pos++] = '\0';
			dest[pos++] = '_';
		}
	/* Again, 2 bytes per character. */
	dest[pos++] = '\0';
	dest[pos++] = ';';
	dest[pos++] = '\0';
	dest[pos++] = '1';
	dest[pos++] = '\0';
	dest[pos] = '\0';

	dest = (char *)realloc(dest, pos + 1);
	free(cname);
	if (lext)
		free(cext);

	/* Fill in the size in bytes (including the terminating NULLs) of the
	   destination string. */
	return (uint16_t *) dest;
}

char *iso_p_filename(const char *src)
{
	char *dest = malloc(251);	/* We can fit up to 250 characters in
					   a Rock Ridge name entry. */
	char *dot = strrchr(src, '.');
	int lname, lext, lnname, lnext, pos, i;

	if (dot == NULL || dot == src || *(dot + 1) == '\0') {
		lname = strlen(src);
		lnname = (lname > 250) ? 250 : lname;
		lext = lnext = 0;
	} else {
		lext = strlen(dot + 1);
		lname = strlen(src) - lext - 1;
		lnext = (strlen(src) > 250 &&
			 lext > 3) ? (lname < 246 ? 249 - lname : 3) : lext;
		lnname = (strlen(src) > 250) ? 249 - lnext : lname;
	}

	if (lnname == 0 && lnext == 0) {
		free(dest);
		return NULL;
	}

	pos = 0;
	for (i = 0; i < lnname; i++)
		dest[pos++] = valid_p_char(src[i]) ? src[i] : '_';
	if (lnext) {
		dest[pos++] = '.';
		for (i = 0; i < lnext; i++)
			dest[pos++] =
				valid_p_char(src[lname + 1 + i]) ?
					     src[lname + 1 + i] : '_';
	}
	dest[pos] = '\0';
	dest = (char *)realloc(dest, pos + 1);

	return dest;
}

char *iso_p_dirname(const char *src)
{
	char *dest = strlen(src) > 250 ? malloc(251) : malloc(strlen(src) + 1);
	int i;

	if (strlen(src) == 0) {
		free(dest);
		return NULL;
	}

	for (i = 0; i < 250 && src[i]; i++)
		dest[i] = valid_p_char(src[i]) ? src[i] : '_';
	dest[i] = '\0';

	return dest;
}

char *iso_pathname(const char *dir, const char *file)
{
	char *path = malloc(strlen(dir) + strlen(file) + 2);

	strcpy(path, dir);
	path[strlen(dir)] = '/';
	strcpy(path + strlen(dir) + 1, file);

	return path;
}

void iso_a_strcpy(unsigned char *dest, int size, const char *src)
{
	int i = 0, d = 0;

	if (src)
		for (; i < size && *src; ++i, ++src) {
			char s = toupper(*src);

			if (valid_a_char(s))
				dest[d++] = s;
			else
				dest[d++] = '_';
		}
	for (; i < size; ++i) {
		/* pad with spaces */
		dest[d++] = ' ';
	}
}

void iso_d_strcpy(unsigned char *dest, int size, const char *src)
{
	int i = 0, d = 0;

	if (src)
		for (; i < size && *src; ++i, ++src) {
			char s = toupper(*src);

			if (valid_d_char(s))
				dest[d++] = s;
			else
				dest[d++] = '_';
		}
	for (; i < size; ++i) {
		/* pad with spaces */
		dest[d++] = ' ';
	}
}

char *iso_a_strndup(const char *src, int size)
{
	int i, d;
	char *out;

	out = malloc(size + 1);
	for (i = d = 0; i < size && *src; ++i, ++src) {
		char s = toupper(*src);

		if (valid_a_char(s))
			out[d++] = s;
		else
			out[d++] = '_';
	}
	out[d++] = '\0';
	out = realloc(out, d);	/* shrink the buffer to what we used */
	return out;
}

char *iso_d_strndup(const char *src, int size)
{
	int i, d;
	char *out;

	out = malloc(size + 1);
	for (i = d = 0; i < size && *src; ++i, ++src) {
		char s = toupper(*src);

		if (valid_d_char(s))
			out[d++] = s;
		else
			out[d++] = '_';
	}
	out[d++] = '\0';
	out = realloc(out, d);	/* shrink the buffer to what we used */
	return out;
}

char *iso_strconcat(char sep, const char *a, const char *b)
{
	char *out;
	int la, lb;

	la = strlen(a);
	lb = strlen(b);
	out = malloc(la + lb + 1 + (sep ? 1 : 0));
	memcpy(out, a, la);
	memcpy(out + la + (sep ? 1 : 0), b, lb);
	if (sep)
		out[la] = sep;
	out[la + lb + (sep ? 1 : 0)] = '\0';
	return out;
}

char *iso_strdup(const char *src)
{
	return src ? strdup(src) : NULL;
}

void iso_lsb(uint8_t *buf, uint32_t num, int bytes)
{
	int i;

	assert(bytes <= 4);

	for (i = 0; i < bytes; ++i)
		buf[i] = (num >> (8 * i)) & 0xff;
}

void iso_msb(uint8_t *buf, uint32_t num, int bytes)
{
	int i;

	assert(bytes <= 4);

	for (i = 0; i < bytes; ++i)
		buf[bytes - 1 - i] = (num >> (8 * i)) & 0xff;
}

void iso_bb(uint8_t *buf, uint32_t num, int bytes)
{
	iso_lsb(buf, num, bytes);
	iso_msb(buf+bytes, num, bytes);
}


void iso_datetime_7(unsigned char *buf, time_t t)
{
	static int tzsetup = 0;
	static int tzoffset;
	struct tm tm;

	if (!tzsetup) {
		tzset();

		tzoffset = -timezone / 60 / 15;
		if (tzoffset < -48)
			tzoffset += 101;

		tzsetup = 1;
	}

	localtime_r(&t, &tm);

	buf[0] = tm.tm_year;
	buf[1] = tm.tm_mon + 1;
	buf[2] = tm.tm_mday;
	buf[3] = tm.tm_hour;
	buf[4] = tm.tm_min;
	buf[5] = tm.tm_sec;
	buf[6] = tzoffset;
}

time_t iso_datetime_read_7(const uint8_t *buf)
{
	struct tm tm;

	tm.tm_year = buf[0];
	tm.tm_mon = buf[1] + 1;
	tm.tm_mday = buf[2];
	tm.tm_hour = buf[3];
	tm.tm_min = buf[4];
	tm.tm_sec = buf[5];

	return mktime(&tm) - buf[6] * 60 * 60;
}

void iso_datetime_17(unsigned char *buf, time_t t)
{
	static int tzsetup = 0;
	static int tzoffset;
	struct tm tm;
	char c[5];

	if (t == (time_t) - 1) {
		/* unspecified time */
		memset(buf, '0', 16);
		buf[16] = 0;
	} else {
		if (!tzsetup) {
			tzset();

			tzoffset = -timezone / 60 / 15;
			if (tzoffset < -48)
				tzoffset += 101;

			tzsetup = 1;
		}

		localtime_r(&t, &tm);

		/* year */
		sprintf(c, "%04d", tm.tm_year + 1900);
		memcpy(&buf[0], c, 4);
		/* month */
		sprintf(c, "%02d", tm.tm_mon + 1);
		memcpy(&buf[4], c, 2);
		/* day */
		sprintf(c, "%02d", tm.tm_mday);
		memcpy(&buf[6], c, 2);
		/* hour */
		sprintf(c, "%02d", tm.tm_hour);
		memcpy(&buf[8], c, 2);
		/* minute */
		sprintf(c, "%02d", tm.tm_min);
		memcpy(&buf[10], c, 2);
		/* second */
		sprintf(c, "%02d", MIN(59, tm.tm_sec));
		memcpy(&buf[12], c, 2);
		/* hundreths */
		memcpy(&buf[14], "00", 2);
		/* timezone */
		buf[16] = tzoffset;
	}
}

time_t iso_datetime_read_17(const uint8_t *buf)
{
	struct tm tm;

	sscanf((char*)&buf[0], "%4d", &tm.tm_year);
	sscanf((char*)&buf[4], "%2d", &tm.tm_mon);
	sscanf((char*)&buf[6], "%2d", &tm.tm_mday);
	sscanf((char*)&buf[8], "%2d", &tm.tm_hour);
	sscanf((char*)&buf[10], "%2d", &tm.tm_min);
	sscanf((char*)&buf[12], "%2d", &tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;

	return mktime(&tm) - buf[16] * 60 * 60;
}

void iso_split_filename(char *name, char **ext)
{
	char *r;

	r = strrchr(name, '.');
	if (r) {
		*r = '\0';
		*ext = r + 1;
	} else
		*ext = "";
}

void iso_filecpy(unsigned char *buf, int size, const char *name, int version)
{
	char v[6];
	int nl, vl;

	assert(version >= 0);
	assert(version < 0x8000);

	nl = strlen(name);

	memcpy(buf, name, nl);

	if (!version)
		assert(size >= strlen(name));
	else {
		sprintf(v, "%d", version);
		vl = strlen(v);
		assert(size >= nl + vl + 1);

		buf[nl] = ';';
		memcpy(&buf[nl + 1], v, vl);

		nl += vl + 1;
	}
	/* pad with spaces */
	if (nl < size)
		memset(&buf[nl], ' ', size - nl);
}

size_t ucslen(const uint16_t *str)
{
	int i;

	for (i=0; str[i]; i++)
		;
	return i;
}

int ucscmp(const uint16_t *s1, const uint16_t *s2)
{
	int i;

	for (i=0; 1; i++) {
		if (s1[i] < s2[i]) {
			return -1;
		} else if (s1[i] > s2[i]) {
			return 1;
		} else if (s1[i] == 0 && s2[i] == 0) {
			break;
		}
	}

	return 0;
}

uint32_t iso_read_lsb(const uint8_t *buf, int bytes)
{
	int i;
	uint32_t ret = 0;

	for (i=0; i<bytes; i++) {
		ret += ((uint32_t) buf[i]) << (i*8);
	}
	return ret;
}

uint32_t iso_read_msb(const uint8_t *buf, int bytes)
{
	int i;
	uint32_t ret = 0;

	for (i=0; i<bytes; i++) {
		ret += ((uint32_t) buf[bytes-i-1]) << (i*8);
	}
	return ret;
}

uint32_t iso_read_bb(const uint8_t *buf, int bytes)
{
	uint32_t v1 = iso_read_lsb(buf, bytes);
	uint32_t v2 = iso_read_msb(buf+bytes, bytes);

	assert(v1 == v2);
	return v1;
}
