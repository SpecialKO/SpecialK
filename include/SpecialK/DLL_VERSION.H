#pragma once


#define SK_MAJOR 0
#define SK_MINOR 9
#define SK_BUILD 20
#define SK_REV_N 0
#define SK_REV   0





#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w) _L2(w)


#if (defined (SK_REV) && SK_REV_N > 0)
#define SK_VERSION_STR_A    _A(SK_MAJOR) "." _A(SK_MINOR) "." _A(SK_BUILD) "." _A(SK_REV)
#else
#define SK_VERSION_STR_A    _A(SK_MAJOR) "." _A(SK_MINOR) "." _A(SK_BUILD)
#endif

#define SK_VERSION_STR_W _L(SK_VERSION_STR_A)


#define SK_FILE_VERSION     SK_MAJOR,SK_MINOR,SK_BUILD,SK_REV_N
#define SK_PRODUCT_VERSION  SK_MAJOR,SK_MINOR,SK_BUILD,SK_REV_N
