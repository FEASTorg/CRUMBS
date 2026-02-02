/**
 * Unit tests for CRUMBS version header.
 */

#include <stdio.h>
#include <string.h>
#include "crumbs.h"

int main(void)
{
/* Test 1: Version macros are defined */
#ifndef CRUMBS_VERSION_MAJOR
    fprintf(stderr, "FAIL: CRUMBS_VERSION_MAJOR not defined\n");
    return 1;
#endif

#ifndef CRUMBS_VERSION_MINOR
    fprintf(stderr, "FAIL: CRUMBS_VERSION_MINOR not defined\n");
    return 1;
#endif

#ifndef CRUMBS_VERSION_PATCH
    fprintf(stderr, "FAIL: CRUMBS_VERSION_PATCH not defined\n");
    return 1;
#endif

#ifndef CRUMBS_VERSION_STRING
    fprintf(stderr, "FAIL: CRUMBS_VERSION_STRING not defined\n");
    return 1;
#endif

#ifndef CRUMBS_VERSION
    fprintf(stderr, "FAIL: CRUMBS_VERSION not defined\n");
    return 1;
#endif

    /* Test 2: Version values are reasonable */
    if (CRUMBS_VERSION_MAJOR != 0)
    {
        fprintf(stderr, "FAIL: Expected MAJOR=0, got %d\n", CRUMBS_VERSION_MAJOR);
        return 1;
    }

    if (CRUMBS_VERSION_MINOR != 10)
    {
        fprintf(stderr, "FAIL: Expected MINOR=10, got %d\n", CRUMBS_VERSION_MINOR);
        return 1;
    }

    if (CRUMBS_VERSION_PATCH != 1)
    {
        fprintf(stderr, "FAIL: Expected PATCH=1, got %d\n", CRUMBS_VERSION_PATCH);
        return 1;
    }

    /* Test 3: Numeric version matches formula */
    int expected_version = CRUMBS_VERSION_MAJOR * 10000 +
                           CRUMBS_VERSION_MINOR * 100 +
                           CRUMBS_VERSION_PATCH;
    if (CRUMBS_VERSION != expected_version)
    {
        fprintf(stderr, "FAIL: Version mismatch. Expected %d, got %d\n",
                expected_version, CRUMBS_VERSION);
        return 1;
    }

    /* Test 4: Version string matches expected format */
    if (strcmp(CRUMBS_VERSION_STRING, "0.10.3") != 0)
    {
        fprintf(stderr, "FAIL: Expected version string '0.10.3', got '%s'\n",
                CRUMBS_VERSION_STRING);
        return 1;
    }

    printf("PASS: Version header validated (v%s, %d)\n",
           CRUMBS_VERSION_STRING, CRUMBS_VERSION);
    return 0;
}
