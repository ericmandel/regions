// emcc -O0 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS="['_miniprintf']" miniprintf.c -o miniprintf.js

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include<math.h>
#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* 
 * Double to ASCII
 * thanks to:
 * https://stackoverflow.com/questions/2302969/how-to-implement-char-ftoafloat-num-without-sprintf-library-function-i
 */
static double PRECISION = 0.00000000000001;
char * dtoa(char *s, double n) {
    // handle special cases
    if (isnan(n)) {
        strcpy(s, "nan");
    } else if (isinf(n)) {
        strcpy(s, "inf");
    } else if (n == 0.0) {
        strcpy(s, "0");
    } else {
        int digit, m, m1;
        char *c = s;
        int neg = (n < 0);
        if (neg)
            n = -n;
        // calculate magnitude
        m = log10(n);
        int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
        if (neg)
            *(c++) = '-';
        // set up for scientific notation
        if (useExp) {
            if (m < 0)
               m -= 1.0;
            n = n / pow(10.0, m);
            m1 = m;
            m = 0;
        }
        if (m < 1.0) {
            m = 0;
        }
        // convert the number
        while (n > PRECISION || m >= 0) {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = floor(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && n > 0)
                *(c++) = '.';
            m--;
        }
        if (useExp) {
            // convert the exponent
            int i, j;
            *(c++) = 'e';
            if (m1 > 0) {
                *(c++) = '+';
            } else {
                *(c++) = '-';
                m1 = -m1;
            }
            m = 0;
            while (m1 > 0) {
                *(c++) = '0' + m1 % 10;
                m1 /= 10;
                m++;
            }
            c -= m;
            for (i = 0, j = m-1; i<j; i++, j--) {
                // swap without temporary
                c[i] ^= c[j];
                c[j] ^= c[i];
                c[i] ^= c[j];
            }
            c += m;
        }
        *(c) = '\0';
    }
    return s;
}

/*
 * baseconv
 *
 * thanks to: http://c-faq.com/~scs/cgi-bin/faqcat.cgi?sec=misc#hexio
 *
 */
char *baseconv(unsigned int num, int base){
	static char retbuf[33];
	char *p;

	if(base < 2 || base > 16)
		return NULL;

	p = &retbuf[sizeof(retbuf)-1];
	*p = '\0';

	do {
		*--p = "0123456789abcdef"[num % base];
		num /= base;
	} while(num != 0);

	return p;
}

/*
 * miniprintf
 *
 * thanks to: http://c-faq.com/~scs/cgi-bin/faqcat.cgi?sec=varargs
 *
 */
void miniprintf(const char *fmt, ...){
	const char *p;
	int i;
	double d;
	unsigned int u;
	char *s;
        char tbuf[1024];
	va_list argp;

	va_start(argp, fmt);

	for(p = fmt; *p != '\0'; p++) {
		if(*p != '%') {
			putchar(*p);
			continue;
		}

		switch(*++p) {
		case 'c':
			i = va_arg(argp, int);
			/* *not* va_arg(argp, char); see Q 15.10 */
			putchar(i);
			break;

		case 'd':
			i = va_arg(argp, int);
			if(i < 0) {
				/* XXX won't handle INT_MIN */
				i = -i;
				putchar('-');
			}
			fputs(baseconv(i, 10), stdout);
			break;

		case 'f':
			d = va_arg(argp, double);
			dtoa(tbuf, d);
			fputs(tbuf, stdout);
			break;

		case 'o':
			u = va_arg(argp, unsigned int);
			fputs(baseconv(u, 8), stdout);
			break;

		case 's':
			s = va_arg(argp, char *);
			fputs(s, stdout);
			break;

		case 'u':
			u = va_arg(argp, unsigned int);
			fputs(baseconv(u, 10), stdout);
			break;

		case 'x':
			u = va_arg(argp, unsigned int);
			fputs(baseconv(u, 16), stdout);
			break;

		case '%':
			putchar('%');
			break;
		}
	}

	va_end(argp);
}

#ifndef __EMSCRIPTEN__

int main(int argc, char **argv){
	miniprintf("Hello, %s!\n", "world");
	miniprintf("%c %d %s %f\n", '1', 2, "three", 4.4);
	miniprintf("%o %d %x %f\n", 10, 10, 10, -10.10);
	miniprintf("%u\n", 0xffff);
	return 0;
}

#endif

