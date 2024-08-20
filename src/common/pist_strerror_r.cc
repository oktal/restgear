/*
 * SPDX-FileCopyrightText: 2024 Duncan Greatwood
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Defines a pist_strerror_r for use in Windows

#include <pistache/pist_strerror_r.h>

#include <string.h>
#include <algorithm>

/* ------------------------------------------------------------------------- */

#ifdef _IS_WINDOWS

#include <pistache/ps_strl.h>

/* ------------------------------------------------------------------------- */

// Note: We use the GNU-specific definition (which returns char *), not the
// XSI-compliant definition (which returns int) even in the non-GNU case.

// strerror_s in Windows is the XSI form (returns int)

static const char * const_bad_strerror_parms = "{Invalid strerror_r parms}";
static char bad_strerror_parms_buff[128+16];

extern "C" char * pist_strerror_r(int errnum, char *buf, size_t buflen)
{
    if ((!buf) || (buflen <= 1))
    {
        if (strcmp(&(bad_strerror_parms_buff[0]), const_bad_strerror_parms))
            PS_STRLCPY(&(bad_strerror_parms_buff[0]),
                    const_bad_strerror_parms, 128);

        return(&(bad_strerror_parms_buff[0]));
    }

    buf[0] = 0;

    errno_t res_strerror_s = strerror_s(buf, buflen, errnum);
    if (res_strerror_s != 0)
    {
        const char * dumb_err = "{unknown err - srterror}";
        if (res_strerror_s == EINVAL)
            dumb_err = "{invalid errnum - srterror}";
        else if (res_strerror_s == ERANGE)
            dumb_err = "{small buf - srterror}";

        PS_STRLCPY(buf, dumb_err, buflen);
    }

    return(buf);
}

/* ------------------------------------------------------------------------- */

#elif !defined(__GNUC__) || defined(__clang__)

#include <pistache/ps_strl.h>

/* ------------------------------------------------------------------------- */

// Note: We use the GNU-specific definition (which returns char *), not the
// XSI-compliant definition (which returns int) even in the non-GNU case.

// Since this is not GNUC, we assume native strerror_r is the XSI form
// (returns int)

static const char * const_bad_strerror_parms = "{Invalid strerror_r parms}";
static char bad_strerror_parms_buff[128];

extern "C" char * pist_strerror_r(int errnum, char *buf, size_t buflen)
{
    if ((!buf) || (buflen <= 1))
    {
        if (strcmp(&(bad_strerror_parms_buff[0]), const_bad_strerror_parms))
            strcpy(&(bad_strerror_parms_buff[0]), const_bad_strerror_parms);

        return(&(bad_strerror_parms_buff[0]));
    }

    buf[0] = 0;

    // Since it's not GNUC, we assume native strerror_r is the XSI form
    // (returns int)
    int res_strerror_r = strerror_r(errnum, buf, buflen);
    if (res_strerror_r != 0)
    {
        const char * dumb_err = "{unknown err - srterror}";
        if (res_strerror_r == EINVAL)
            dumb_err = "{invalid errnum - srterror}";
        else if (res_strerror_r == ERANGE)
            dumb_err = "{small buf - srterror}";

        PS_STRLCPY(buf, dumb_err, buflen);
    }

    return(buf);
}

/* ------------------------------------------------------------------------- */

#endif // of ifdef _IS_WINDOWS... elsif ! defined(__GNUC__)
