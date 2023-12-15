#pragma once

#define SK_YEAR  23
#define SK_MONTH 12
#define SK_DATE  15
#define SK_REV_N 0
#define SK_REV   0

#ifndef _A2
#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w)   _L2(w)
#endif

#if (defined (SK_REV) && SK_REV_N > 0)
#define SK_VERSION_STR_A    _A(SK_YEAR) "." _A(SK_MONTH) "." _A(SK_DATE) "." _A(SK_REV)
#else
#define SK_VERSION_STR_A    _A(SK_YEAR) "." _A(SK_MONTH) "." _A(SK_DATE)
#endif

#define SK_VERSION_STR_W _L(SK_VERSION_STR_A)

#define SK_FILE_VERSION     SK_YEAR,SK_MONTH,SK_DATE,SK_REV_N
#define SK_PRODUCT_VERSION  SK_YEAR,SK_MONTH,SK_DATE,SK_REV_N
