#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "base64.h"

static char *tobase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static char frombase64[128];

int
base64enc(char *out, char *buf, int len)
{
	uint32_t tmp;
	int i, j;
	for(i = 0, j = 0; i+2 < len; i += 3, j += 4){
		tmp = (uint32_t)buf[i+2] + ((uint32_t)buf[i+1]<<8) + ((uint32_t)buf[i+0]<<16);
		out[j+3] = tobase64[tmp & 63];
		out[j+2] = tobase64[(tmp>>6) & 63];
		out[j+1] = tobase64[(tmp>>12) & 63];
		out[j+0] = tobase64[(tmp>>18) & 63];
	}
	if(i > len)
		out[j-1] = '=';
	if(i-1 > len)
		out[j-2] = '=';
	return j;
}

int
base64dec(char *out, char *buf, int len)
{
	uint32_t tmp;
	int i, j;

	if(frombase64['='] == 0)
		for(i = 0; tobase64[i] != '\0'; i++)
			frombase64[tobase64[i]] = i;

	for(i = 0, j = 0; i+3 < len; i += 4, j += 3){
		tmp = frombase64[buf[i+3]&127] | ((frombase64[buf[i+2]&127] & 63) << 6) | ((frombase64[buf[i+1]&127] & 63) << 12) | ((frombase64[buf[i+0]&127] & 63) << 18);
		out[j+2] = tmp & 0xff;
		out[j+1] = (tmp>>8) & 0xff;
		out[j+0] = (tmp>>16) & 0xff;
	}

	// handle odd lengths
	if(i+2 < len){
		tmp = ((frombase64[buf[i+2]&127] & 63) << 6) | ((frombase64[buf[i+1]&127] & 63) << 12) | ((frombase64[buf[i+0]&127] & 63) << 18);
		out[j+1] = (tmp>>8) & 0xff;
		out[j+0] = (tmp>>16) & 0xff;
		j += 2;
		i += 3;
	} else if(i+1 < len){
		tmp = ((frombase64[buf[i+1]&127] & 63) << 12) | ((frombase64[buf[i+0]&127] & 63) << 18);
		out[j+0] = (tmp>>16) & 0xff;
		j++;
		i += 2;
	} else if(i < len){
		// deal with a non-compliant partial last byte if there is one..
		tmp = ((frombase64[buf[i+0]&127] & 63) << 18);
		out[j+0] = (tmp>>16) & 0xff;
		j++;
		i++;
	} else {
		// length was a multiple of 4, still need to check for padding.
		if(i > 0 && buf[i-1] == '=')
			j--;
		if(i > 1 && buf[i-2] == '=')
			j--;
	}

	return j;
}

#ifdef MAIN

static char buf[8192];
static char out[base64cap(sizeof buf)];

int
main(void)
{
	int nrd, nout;

	nrd = read(0, buf, sizeof buf);
	nout = base64dec(out, buf, nrd);
	fprintf(stderr, "got %d dec %d\n", nrd, nout);
	write(1, "'", 1);
	write(1, out, nout);
	write(1, "'\n", 2);

	return 0;
}
#endif
