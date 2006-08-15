/* vim: set noet ts=8 sts=8 sw=8 : */

#include "struct.h"
#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct struct_element {
	uint8_t ch;

	int bytes;	/* The number of bytes in the value to convert
			 * from/to. */
	uint8_t end;	/* The endianness specifier. */
	int mul;	/* The number of values to convert. */

	union {		/* Pointer to the value. */
		uint8_t *val8;
		uint16_t *val16;
		uint32_t *val32;
		time_t *time;
	} val;
};

/* check if a character is a valid endian-ness specifier */
#define isend(a) ((a) == '=' || (a) == '<' || (a) == '>')

static int iso_struct_element_make(struct struct_element *elem,
				   int mul,
				   char end,
				   char ch)
{
	if (!end) {
#ifdef WORDS_BIGENDIAN
		elem->end = '>';	/* default endianness is native */
#else
		elem->end = '<';
#endif
	} else {
		elem->end = end;
	}

	elem->ch = ch;
	elem->mul = mul;
	elem->val.val8 = NULL;
	switch(toupper(ch)) {
		case 'X':
		case 'B':
			elem->bytes = 1;
			break;
		case 'H':
			elem->bytes = 2;
			break;
		case 'L':
			elem->bytes = 4;
			break;
		case 'S':
			elem->bytes = 7;
			elem->end = '<';
			break;
		case 'T':
			elem->bytes = 17;
			elem->end = '<';
			break;
		default:
			elem->bytes = -1;
			break;
	}
	return elem->bytes * elem->mul * ((elem->end == '=') ? 2 : 1);
}

static int iso_struct_element_make_v(struct struct_element *elem,
				     va_list *ap)
{
	int mul = va_arg(*ap, int);
	int end = va_arg(*ap, int);
	int ch = va_arg(*ap, int);
	return iso_struct_element_make(elem, mul, end, ch);
}

static int iso_struct_element_parse(const char **ffmt,
				    struct struct_element *elem)
{
	off_t pos;
	const char *fmt = *ffmt;
	int mul;
	char end = 0;

	mul = 1;
	for (pos=0; isdigit(fmt[pos]) || isend(fmt[pos]); pos++) {
		if (isdigit(fmt[pos])) {
			mul = atoi( fmt + pos );
			while (isdigit(fmt[pos+1])) pos++;
		} else {
			end = fmt[pos];
		}
	}

	(*ffmt) += pos + 1;

	return iso_struct_element_make(elem, mul, end, fmt[pos]);
}

/* read a single integer from data[i] to elem[i], interpreting the endian-ness
 * and offset appropriately. */
static uint32_t iso_struct_element_read_int(struct struct_element *elem,
					    const uint8_t *data,
					    int i)
{
	uint32_t el;

	switch(elem->end) {
	case '>':
		el = iso_read_msb(data + i*elem->bytes, elem->bytes);
		break;
	case '<':
		el = iso_read_lsb(data + i*elem->bytes, elem->bytes);
		break;
	case '=':
		el = iso_read_bb(data + i*elem->bytes*2, elem->bytes);
	}

	switch(elem->bytes) {
	case 1:
		elem->val.val8[i] = el;
		break;
	case 2:
		elem->val.val16[i] = el;
		break;
	case 4:
		elem->val.val32[i] = el;
		break;
	}

	return el;
}

/* write a single integer from elem[i] to data[i]. */
static uint32_t iso_struct_element_write1(struct struct_element *elem,
					  uint8_t *data,
					  int i)
{
	uint32_t el;

	switch(elem->bytes) {
	case 1:
		el = elem->val.val8[i];
		break;
	case 2:
		el = elem->val.val16[i];
		break;
	case 4:
		el = elem->val.val32[i];
		break;
	}

	switch(elem->end) {
	case '>':
		iso_msb(data + i*elem->bytes, el, elem->bytes);
		break;
	case '<':
		iso_lsb(data + i*elem->bytes, el, elem->bytes);
		break;
	case '=':
		iso_bb(data + i*elem->bytes*2, el, elem->bytes);
	}

	return el;
}

static int iso_struct_element_read(struct struct_element *elem,
				   const uint8_t *data)
{
	int size = elem->bytes * ((elem->end == '=') ? 2 : 1);
	int i;

	if (elem->ch == 'x') {
		return size * elem->mul;
	}

	for (i=0; i<elem->mul; i++) {
		switch(toupper(elem->ch)) {
		case 'S':
			/*
			elem->val.time[i] = iso_datetime_read_7(&data[i*7]);
			*/
			break;
		case 'T':
			/*
			elem->val.time[i] = iso_datetime_read_17(&data[i*17]);
			*/
			break;
		default:
			iso_struct_element_read_int(elem, data, i);
		}
	}

	return size * elem->mul;
}

static int iso_struct_element_write(struct struct_element *elem,
				    uint8_t *data)
{
	int size = elem->bytes * ((elem->end == '=') ? 2 : 1);
	int i;
	uint32_t ret;

	if (elem->ch == 'x') {
		return size*elem->mul;
	}

	for (i=0; i<elem->mul; i++) {
		switch(toupper(elem->ch)) {
		case 'S':
			iso_datetime_7(&data[i*7], elem->val.time[i]);
			ret = elem->val.time[i];
			break;
		case 'T':
			iso_datetime_17(&data[i*17], elem->val.time[i]);
			ret = elem->val.time[i];
			break;
		default:
			ret = iso_struct_element_write1(elem, data, i);
			break;
		}

		if (islower(elem->ch) && ret == 0) {
			memset(data + size*i, 0, size*(elem->mul-i));
			break;
		}
	}

	return size * elem->mul;
}

int iso_struct_unpack(const char *fmt, const uint8_t *data, ...)
{
	int num_conv;
	int ret;
	va_list ap;
	struct struct_element elem;
	off_t off;

	va_start(ap, data);
	num_conv = 0;
	off = 0;
	while(*fmt) {
		ret = iso_struct_element_parse(&fmt, &elem);
		if (ret < 0) {
			va_end(ap);
			return -1;
		}
		if (elem.ch != 'x') {
			elem.val.val8 = va_arg(ap, void*);
		}
		off += iso_struct_element_read(&elem, data + off);
		num_conv++;
	}

	va_end(ap);
	return num_conv;
}

int iso_struct_pack(const char *fmt, uint8_t *data, ...)
{
	int num_conv;
	int ret;
	va_list ap;
	struct struct_element elem;
	off_t off;

	va_start(ap, data);
	num_conv = 0;
	off = 0;
	while(*fmt) {
		ret = iso_struct_element_parse(&fmt, &elem);
		if (ret < 0) {
			va_end(ap);
			return -1;
		}
		if (elem.ch != 'x') {
			elem.val.val8 = va_arg(ap, void*);
		}
		off += iso_struct_element_write(&elem, data + off);
		num_conv++;
	}

	va_end(ap);
	return num_conv;
}

int iso_struct_pack_long(uint8_t *data, ...)
{
	int num_conv;
	int ret;
	int i, j;
	va_list ap;
	struct struct_element *elem = NULL;
	off_t off;

	va_start(ap, data);
	num_conv = 0;
	off = 0;

	elem = calloc(1, sizeof(struct struct_element));
	i=0;
	while ((ret = iso_struct_element_make_v(&elem[i], &ap) > 0)) {
		elem = realloc(elem, (++i + 1) * sizeof(struct struct_element));
	}
	for (j=0; j<i; j++) {
		if (elem[j].ch != 'x') {
			elem[j].val.val8 = va_arg(ap, void*);
		}
		off += iso_struct_element_write(&elem[j], data + off);
	}

	va_end(ap);
	if (ret < 0) {
		return -1;
	}
	return num_conv;
}

int iso_struct_calcsize(const char *fmt)
{
	int ret, total;
	struct struct_element elem;

	total = 0;
	while (*fmt) {
		ret = iso_struct_element_parse(&fmt, &elem);
		if (ret < 0) {
			return -1;
		}
		total += ret;
	}
	return total;
}
