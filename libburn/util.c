#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../version.h"
#include "util.h"
#include "libburn.h"

char *burn_strdup(char *s)
{
	char *ret;
	int l;

	assert(s);

	l = strlen(s) + 1;
	ret = malloc(l);
	memcpy(ret, s, l);

	return ret;
}

char *burn_strndup(char *s, int n)
{
	char *ret;
	int l;

	assert(s);
	assert(n > 0);

	l = strlen(s);
	ret = malloc(l < n ? l : n);

	memcpy(ret, s, l < n - 1 ? l : n - 1);
	ret[n - 1] = '\0';

	return ret;
}

void burn_version(int *major, int *minor, int *micro)
{
    *major = BURN_MAJOR_VERSION;
    *minor = BURN_MINOR_VERSION;
    *micro = BURN_MICRO_VERSION;
}
