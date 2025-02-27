/* strncat_s( char *, rsize_t, const char *, rsize_t )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef REGTEST

errno_t strncat_s(char* _PDCLIB_restrict s1, rsize_t s1max, const char* _PDCLIB_restrict s2, rsize_t n)
{
    char* dest = s1;
    const char* src = s2;

    if (s1 != NULL && s2 != NULL && s1max <= RSIZE_MAX && n <= RSIZE_MAX && s1max != 0)
    {
        while (*dest)
        {
            if (--s1max == 0 || dest++ == s2)
            {
                goto runtime_constraint_violation;
            }
        }

        do
        {
            if (n-- == 0)
            {
                *dest = '\0';
                return 0;
            }

            if (s1max-- == 0 || dest == s2 || src == s1)
            {
                goto runtime_constraint_violation;
            }
        }
        while ((*dest++ = *src++));

        return 0;
    }

runtime_constraint_violation:

    if (s1 != NULL && s1max > 0 && s1max <= RSIZE_MAX)
    {
        s1[0] = '\0';
    }

    _PDCLIB_constraint_handler(_PDCLIB_CONSTRAINT_VIOLATION(_PDCLIB_EINVAL));
    return _PDCLIB_EINVAL;
}

#endif

#ifdef TEST

#include "_PDCLIB_test.h"

#if ! defined( REGTEST ) || defined( __STDC_LIB_EXT1__ )

static int HANDLER_CALLS = 0;

static void test_handler( const char * _PDCLIB_restrict msg, void * _PDCLIB_restrict ptr, errno_t error )
{
    ++HANDLER_CALLS;
}

#endif

int main( void )
{
#if ! defined( REGTEST ) || defined( __STDC_LIB_EXT1__ )
    char s[] = "xx\0xxxxxx";
    set_constraint_handler_s( test_handler );

    TESTCASE( strncat_s( s, 10, abcde, 10 ) == 0 );
    TESTCASE( s[2] == 'a' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( s[7] == '\0' );
    TESTCASE( s[8] == 'x' );
    s[0] = '\0';
    TESTCASE( strncat_s( s, 10, abcdx, 10 ) == 0 );
    TESTCASE( s[4] == 'x' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( strncat_s( s, 10, "\0", 10 ) == 0 );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( strncat_s( s, 10, abcde, 0 ) == 0 );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( strncat_s( s, 10, abcde, 3 ) == 0 );
    TESTCASE( s[5] == 'a' );
    TESTCASE( s[7] == 'c' );
    TESTCASE( s[8] == '\0' );

    TESTCASE( strncat_s( s, 9, "", 0 ) == 0 );
    TESTCASE( strncat_s( s, 8, "", 0 ) != 0 );
    TESTCASE( strncat_s( s, 8, "x", 0 ) != 0 );
    TESTCASE( strncat_s( s, 9, "x", 0 ) == 0 );
    TESTCASE( strncat_s( s, 10, "x", 1 ) == 0 );
    TESTCASE( s[8] == 'x' );
    TESTCASE( s[9] == '\0' );

    /* Overlapping */
    TESTCASE( strcat_s( s, 7, s + 6 ) != 0 );
    s[3] = '\0';
    TESTCASE( strcat_s( s + 3, 4, s ) != 0 );

    TESTCASE( HANDLER_CALLS == 4 );
#endif
    return TEST_RESULTS;
}

#endif
