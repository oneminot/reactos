/*
 * UrlMon IUri tests
 *
 * Copyright 2010 Thomas Mullaly
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "urlmon.h"
#include "shlwapi.h"
#include "wininet.h"
#include "strsafe.h"

#define URI_STR_PROPERTY_COUNT Uri_PROPERTY_STRING_LAST+1
#define URI_DWORD_PROPERTY_COUNT (Uri_PROPERTY_DWORD_LAST - Uri_PROPERTY_DWORD_START)+1
#define URI_BUILDER_STR_PROPERTY_COUNT 7

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(CombineUrl);
DEFINE_EXPECT(ParseUrl);

static HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, IUri**);
static HRESULT (WINAPI *pCreateUriWithFragment)(LPCWSTR, LPCWSTR, DWORD, DWORD_PTR, IUri**);
static HRESULT (WINAPI *pCreateIUriBuilder)(IUri*, DWORD, DWORD_PTR, IUriBuilder**);
static HRESULT (WINAPI *pCoInternetCombineIUri)(IUri*,IUri*,DWORD,IUri**,DWORD_PTR);
static HRESULT (WINAPI *pCoInternetGetSession)(DWORD,IInternetSession**,DWORD);
static HRESULT (WINAPI *pCoInternetCombineUrlEx)(IUri*,LPCWSTR,DWORD,IUri**,DWORD_PTR);
static HRESULT (WINAPI *pCoInternetParseIUri)(IUri*,PARSEACTION,DWORD,LPWSTR,DWORD,DWORD*,DWORD_PTR);

static const WCHAR http_urlW[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g','/',0};
static const WCHAR http_url_fragW[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g','/','#','F','r','a','g',0};

static const WCHAR combine_baseW[] = {'w','i','n','e','t','e','s','t',':','?','t',
        'e','s','t','i','n','g',0};
static const WCHAR combine_relativeW[] = {'?','t','e','s','t',0};
static const WCHAR combine_resultW[] = {'z','i','p',':','t','e','s','t',0};

static const WCHAR winetestW[] = {'w','i','n','e','t','e','s','t',0};

static const WCHAR parse_urlW[] = {'w','i','n','e','t','e','s','t',':','t','e','s','t',0};
static const WCHAR parse_resultW[] = {'z','i','p',':','t','e','s','t',0};

static PARSEACTION parse_action;
static DWORD parse_flags;

typedef struct _uri_create_flag_test {
    DWORD   flags;
    HRESULT expected;
} uri_create_flag_test;

static const uri_create_flag_test invalid_flag_tests[] = {
    /* Set of invalid flag combinations to test for. */
    {Uri_CREATE_DECODE_EXTRA_INFO | Uri_CREATE_NO_DECODE_EXTRA_INFO, E_INVALIDARG},
    {Uri_CREATE_CANONICALIZE | Uri_CREATE_NO_CANONICALIZE, E_INVALIDARG},
    {Uri_CREATE_CRACK_UNKNOWN_SCHEMES | Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES, E_INVALIDARG},
    {Uri_CREATE_PRE_PROCESS_HTML_URI | Uri_CREATE_NO_PRE_PROCESS_HTML_URI, E_INVALIDARG},
    {Uri_CREATE_IE_SETTINGS | Uri_CREATE_NO_IE_SETTINGS, E_INVALIDARG}
};

typedef struct _uri_str_property {
    const char* value;
    HRESULT     expected;
    BOOL        todo;
    const char* broken_value;
} uri_str_property;

typedef struct _uri_dword_property {
    DWORD   value;
    HRESULT expected;
    BOOL    todo;
} uri_dword_property;

typedef struct _uri_properties {
    const char*         uri;
    DWORD               create_flags;
    HRESULT             create_expected;
    BOOL                create_todo;

    uri_str_property    str_props[URI_STR_PROPERTY_COUNT];
    uri_dword_property  dword_props[URI_DWORD_PROPERTY_COUNT];
} uri_properties;

static const uri_properties uri_tests[] = {
    {   "http://www.winehq.org/tests/../tests/../..", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/",S_OK,FALSE},                      /* ABSOLUTE_URI */
            {"www.winehq.org",S_OK,FALSE},                              /* AUTHORITY */
            {"http://www.winehq.org/",S_OK,FALSE},                      /* DISPLAY_URI */
            {"winehq.org",S_OK,FALSE},                                  /* DOMAIN */
            {"",S_FALSE,FALSE},                                         /* EXTENSION */
            {"",S_FALSE,FALSE},                                         /* FRAGMENT */
            {"www.winehq.org",S_OK,FALSE},                              /* HOST */
            {"",S_FALSE,FALSE},                                         /* PASSWORD */
            {"/",S_OK,FALSE},                                           /* PATH */
            {"/",S_OK,FALSE},                                           /* PATH_AND_QUERY */
            {"",S_FALSE,FALSE},                                         /* QUERY */
            {"http://www.winehq.org/tests/../tests/../..",S_OK,FALSE},  /* RAW_URI */
            {"http",S_OK,FALSE},                                        /* SCHEME_NAME */
            {"",S_FALSE,FALSE},                                         /* USER_INFO */
            {"",S_FALSE,FALSE}                                          /* USER_NAME */
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},                                  /* HOST_TYPE */
            {80,S_OK,FALSE},                                            /* PORT */
            {URL_SCHEME_HTTP,S_OK,FALSE},                               /* SCHEME */
            {URLZONE_INVALID,E_NOTIMPL,FALSE}                           /* ZONE */
        }
    },
    {   "http://winehq.org/tests/.././tests", 0, S_OK, FALSE,
        {
            {"http://winehq.org/tests",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"http://winehq.org/tests",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests",S_OK,FALSE},
            {"/tests",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://winehq.org/tests/.././tests",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "HtTp://www.winehq.org/tests/..?query=x&return=y", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=x&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=x&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=x&return=y",S_OK,FALSE},
            {"?query=x&return=y",S_OK,FALSE},
            {"HtTp://www.winehq.org/tests/..?query=x&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    {   "hTTp://us%45r%3Ainfo@examp%4CE.com:80/path/a/b/./c/../%2E%2E/Forbidden'<|> Characters", 0, S_OK, FALSE,
        {
            {"http://usEr%3Ainfo@example.com/path/a/Forbidden'%3C%7C%3E%20Characters",S_OK,FALSE},
            {"usEr%3Ainfo@example.com",S_OK,FALSE},
            {"http://example.com/path/a/Forbidden'%3C%7C%3E%20Characters",S_OK,FALSE},
            {"example.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"example.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/path/a/Forbidden'%3C%7C%3E%20Characters",S_OK,FALSE},
            {"/path/a/Forbidden'%3C%7C%3E%20Characters",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"hTTp://us%45r%3Ainfo@examp%4CE.com:80/path/a/b/./c/../%2E%2E/Forbidden'<|> Characters",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"usEr%3Ainfo",S_OK,FALSE},
            {"usEr%3Ainfo",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    {   "ftp://winepass:wine@ftp.winehq.org:9999/dir/foo bar.txt", 0, S_OK, FALSE,
        {
            {"ftp://winepass:wine@ftp.winehq.org:9999/dir/foo%20bar.txt",S_OK,FALSE},
            {"winepass:wine@ftp.winehq.org:9999",S_OK,FALSE},
            {"ftp://ftp.winehq.org:9999/dir/foo%20bar.txt",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {".txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.winehq.org",S_OK,FALSE},
            {"wine",S_OK,FALSE},
            {"/dir/foo%20bar.txt",S_OK,FALSE},
            {"/dir/foo%20bar.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://winepass:wine@ftp.winehq.org:9999/dir/foo bar.txt",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"winepass:wine",S_OK,FALSE},
            {"winepass",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {9999,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file://c:\\tests\\../tests/foo%20bar.mp3", 0, S_OK, FALSE,
        {
            {"file:///c:/tests/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///c:/tests/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/c:/tests/foo%2520bar.mp3",S_OK,FALSE},
            {"/c:/tests/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\tests\\../tests/foo%20bar.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "FILE://localhost/test dir\\../tests/test%20file.README.txt", 0, S_OK, FALSE,
        {
            {"file:///tests/test%20file.README.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///tests/test%20file.README.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/test%20file.README.txt",S_OK,FALSE},
            {"/tests/test%20file.README.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"FILE://localhost/test dir\\../tests/test%20file.README.txt",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "urn:nothing:should:happen here", 0, S_OK, FALSE,
        {
            {"urn:nothing:should:happen here",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:nothing:should:happen here",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"nothing:should:happen here",S_OK,FALSE},
            {"nothing:should:happen here",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:nothing:should:happen here",S_OK,FALSE},
            {"urn",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://127.0.0.1/tests/../test dir/./test.txt", 0, S_OK, FALSE,
        {
            {"http://127.0.0.1/test%20dir/test.txt",S_OK,FALSE},
            {"127.0.0.1",S_OK,FALSE},
            {"http://127.0.0.1/test%20dir/test.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"127.0.0.1",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test%20dir/test.txt",S_OK,FALSE},
            {"/test%20dir/test.txt",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://127.0.0.1/tests/../test dir/./test.txt",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]", 0, S_OK, FALSE,
        {
            {"http://[fedc:ba98:7654:3210:fedc:ba98:7654:3210]/",S_OK,FALSE},
            {"[fedc:ba98:7654:3210:fedc:ba98:7654:3210]",S_OK,FALSE},
            {"http://[fedc:ba98:7654:3210:fedc:ba98:7654:3210]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"fedc:ba98:7654:3210:fedc:ba98:7654:3210",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "ftp://[::13.1.68.3]", 0, S_OK, FALSE,
        {
            {"ftp://[::13.1.68.3]/",S_OK,FALSE},
            {"[::13.1.68.3]",S_OK,FALSE},
            {"ftp://[::13.1.68.3]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"::13.1.68.3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://[::13.1.68.3]",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[FEDC:BA98:0:0:0:0:0:3210]", 0, S_OK, FALSE,
        {
            {"http://[fedc:ba98::3210]/",S_OK,FALSE},
            {"[fedc:ba98::3210]",S_OK,FALSE},
            {"http://[fedc:ba98::3210]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"fedc:ba98::3210",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[FEDC:BA98:0:0:0:0:0:3210]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "1234://www.winehq.org", 0, S_OK, FALSE,
        {
            {"1234://www.winehq.org/",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"1234://www.winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"1234://www.winehq.org",S_OK,FALSE},
            {"1234",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Test's to make sure the parser/canonicalizer handles implicit file schemes correctly. */
    {   "C:/test/test.mp3", Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, S_OK, FALSE,
        {
            {"file:///C:/test/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/test/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/test/test.mp3",S_OK,FALSE},
            {"/C:/test/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"C:/test/test.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Test's to make sure the parser/canonicalizer handles implicit file schemes correctly. */
    {   "\\\\Server/test.mp3", Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, S_OK, FALSE,
        {
            {"file://server/test.mp3",S_OK,FALSE},
            {"server",S_OK,FALSE},
            {"file://server/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"server",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test.mp3",S_OK,FALSE},
            {"/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"\\\\Server/test.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "www.winehq.org/test", Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME, S_OK, FALSE,
        {
            {"*:www.winehq.org/test",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"*:www.winehq.org/test",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test",S_OK,FALSE},
            {"/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org/test",S_OK,FALSE},
            {"*",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_WILDCARD,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Valid since the '*' is the only character in the scheme name. */
    {   "*:www.winehq.org/test", 0, S_OK, FALSE,
        {
            {"*:www.winehq.org/test",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"*:www.winehq.org/test",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test",S_OK,FALSE},
            {"/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"*:www.winehq.org/test",S_OK,FALSE},
            {"*",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_WILDCARD,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "/../some dir/test.ext", Uri_CREATE_ALLOW_RELATIVE, S_OK, FALSE,
        {
            {"/../some dir/test.ext",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/../some dir/test.ext",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".ext",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/../some dir/test.ext",S_OK,FALSE},
            {"/../some dir/test.ext",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/../some dir/test.ext",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "//implicit/wildcard/uri scheme", Uri_CREATE_ALLOW_RELATIVE|Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME, S_OK, FALSE,
        {
            {"*://implicit/wildcard/uri%20scheme",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"*://implicit/wildcard/uri%20scheme",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"//implicit/wildcard/uri%20scheme",S_OK,FALSE},
            {"//implicit/wildcard/uri%20scheme",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"//implicit/wildcard/uri scheme",S_OK,FALSE},
            {"*",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_WILDCARD,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* URI is considered opaque since CREATE_NO_CRACK_UNKNOWN_SCHEMES is set and its an unknown scheme. */
    {   "zip://google.com", Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES, S_OK, FALSE,
        {
            {"zip:/.//google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip:/.//google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/.//google.com",S_OK,FALSE},
            {"/.//google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://google.com",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Windows uses the first occurrence of ':' to delimit the userinfo. */
    {   "ftp://user:pass:word@winehq.org/", 0, S_OK, FALSE,
        {
            {"ftp://user:pass:word@winehq.org/",S_OK,FALSE},
            {"user:pass:word@winehq.org",S_OK,FALSE},
            {"ftp://winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"pass:word",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://user:pass:word@winehq.org/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"user:pass:word",S_OK,FALSE},
            {"user",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure % encoded unreserved characters are decoded. */
    {   "ftp://w%49%4Ee:PA%53%53@ftp.google.com/", 0, S_OK, FALSE,
        {
            {"ftp://wINe:PASS@ftp.google.com/",S_OK,FALSE},
            {"wINe:PASS@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"PASS",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://w%49%4Ee:PA%53%53@ftp.google.com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"wINe:PASS",S_OK,FALSE},
            {"wINe",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure % encoded characters which are NOT unreserved are NOT decoded. */
    {   "ftp://w%5D%5Be:PA%7B%7D@ftp.google.com/", 0, S_OK, FALSE,
        {
            {"ftp://w%5D%5Be:PA%7B%7D@ftp.google.com/",S_OK,FALSE},
            {"w%5D%5Be:PA%7B%7D@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"PA%7B%7D",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://w%5D%5Be:PA%7B%7D@ftp.google.com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"w%5D%5Be:PA%7B%7D",S_OK,FALSE},
            {"w%5D%5Be",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* You're allowed to have an empty password portion in the userinfo section. */
    {   "ftp://empty:@ftp.google.com/", 0, S_OK, FALSE,
        {
            {"ftp://empty:@ftp.google.com/",S_OK,FALSE},
            {"empty:@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://empty:@ftp.google.com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"empty:",S_OK,FALSE},
            {"empty",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure forbidden characters in "userinfo" get encoded. */
    {   "ftp://\" \"weird@ftp.google.com/", 0, S_OK, FALSE,
        {
            {"ftp://%22%20%22weird@ftp.google.com/",S_OK,FALSE},
            {"%22%20%22weird@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://\" \"weird@ftp.google.com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"%22%20%22weird",S_OK,FALSE},
            {"%22%20%22weird",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure the forbidden characters don't get percent encoded. */
    {   "ftp://\" \"weird@ftp.google.com/", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"ftp://\" \"weird@ftp.google.com/",S_OK,FALSE},
            {"\" \"weird@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://\" \"weird@ftp.google.com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"\" \"weird",S_OK,FALSE},
            {"\" \"weird",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure already percent encoded characters don't get unencoded. */
    {   "ftp://\"%20\"weird@ftp.google.com/\"%20\"weird", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"ftp://\"%20\"weird@ftp.google.com/\"%20\"weird",S_OK,FALSE},
            {"\"%20\"weird@ftp.google.com",S_OK,FALSE},
            {"ftp://ftp.google.com/\"%20\"weird",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp.google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/\"%20\"weird",S_OK,FALSE},
            {"/\"%20\"weird",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://\"%20\"weird@ftp.google.com/\"%20\"weird",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"\"%20\"weird",S_OK,FALSE},
            {"\"%20\"weird",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Allowed to have invalid % encoded because its an unknown scheme type. */
    {   "zip://%xy:word@winehq.org/", 0, S_OK, FALSE,
        {
            {"zip://%xy:word@winehq.org/",S_OK,FALSE},
            {"%xy:word@winehq.org",S_OK,FALSE},
            {"zip://%xy:word@winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"word",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://%xy:word@winehq.org/",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"%xy:word",S_OK,FALSE},
            {"%xy",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Unreserved, percent encoded characters aren't decoded in the userinfo becuase the scheme
     * isn't known.
     */
    {   "zip://%2E:%52%53ord@winehq.org/", 0, S_OK, FALSE,
        {
            {"zip://%2E:%52%53ord@winehq.org/",S_OK,FALSE},
            {"%2E:%52%53ord@winehq.org",S_OK,FALSE},
            {"zip://%2E:%52%53ord@winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"%52%53ord",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://%2E:%52%53ord@winehq.org/",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"%2E:%52%53ord",S_OK,FALSE},
            {"%2E",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "ftp://[](),'test':word@winehq.org/", 0, S_OK, FALSE,
        {
            {"ftp://[](),'test':word@winehq.org/",S_OK,FALSE},
            {"[](),'test':word@winehq.org",S_OK,FALSE},
            {"ftp://winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"word",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://[](),'test':word@winehq.org/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"[](),'test':word",S_OK,FALSE},
            {"[](),'test'",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "ftp://test?:word@winehq.org/", 0, S_OK, FALSE,
        {
            {"ftp://test/?:word@winehq.org/",S_OK,FALSE},
            {"test",S_OK,FALSE},
            {"ftp://test/?:word@winehq.org/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?:word@winehq.org/",S_OK,FALSE},
            {"?:word@winehq.org/",S_OK,FALSE},
            {"ftp://test?:word@winehq.org/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "ftp://test#:word@winehq.org/", 0, S_OK, FALSE,
        {
            {"ftp://test/#:word@winehq.org/",S_OK,FALSE},
            {"test",S_OK,FALSE},
            {"ftp://test/#:word@winehq.org/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"#:word@winehq.org/",S_OK,FALSE},
            {"test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://test#:word@winehq.org/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Allowed to have a backslash in the userinfo since it's an unknown scheme. */
    {   "zip://test\\:word@winehq.org/", 0, S_OK, FALSE,
        {
            {"zip://test\\:word@winehq.org/",S_OK,FALSE},
            {"test\\:word@winehq.org",S_OK,FALSE},
            {"zip://test\\:word@winehq.org/",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"word",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://test\\:word@winehq.org/",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"test\\:word",S_OK,FALSE},
            {"test\\",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* It normalizes IPv4 addresses correctly. */
    {   "http://127.000.000.100/", 0, S_OK, FALSE,
        {
            {"http://127.0.0.100/",S_OK,FALSE},
            {"127.0.0.100",S_OK,FALSE},
            {"http://127.0.0.100/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"127.0.0.100",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://127.000.000.100/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure it normalizes partial IPv4 addresses correctly. */
    {   "http://127.0/", 0, S_OK, FALSE,
        {
            {"http://127.0.0.0/",S_OK,FALSE},
            {"127.0.0.0",S_OK,FALSE},
            {"http://127.0.0.0/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"127.0.0.0",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://127.0/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure it converts implicit IPv4's correctly. */
    {   "http://123456/", 0, S_OK, FALSE,
        {
            {"http://0.1.226.64/",S_OK,FALSE},
            {"0.1.226.64",S_OK,FALSE},
            {"http://0.1.226.64/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"0.1.226.64",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://123456/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* UINT_MAX */
    {   "http://4294967295/", 0, S_OK, FALSE,
        {
            {"http://255.255.255.255/",S_OK,FALSE},
            {"255.255.255.255",S_OK,FALSE},
            {"http://255.255.255.255/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"255.255.255.255",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://4294967295/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* UINT_MAX+1 */
    {   "http://4294967296/", 0, S_OK, FALSE,
        {
            {"http://4294967296/",S_OK,FALSE},
            {"4294967296",S_OK,FALSE},
            {"http://4294967296/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"4294967296",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://4294967296/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Window's doesn't normalize IP address for unknown schemes. */
    {   "1234://4294967295/", 0, S_OK, FALSE,
        {
            {"1234://4294967295/",S_OK,FALSE},
            {"4294967295",S_OK,FALSE},
            {"1234://4294967295/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"4294967295",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"1234://4294967295/",S_OK,FALSE},
            {"1234",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Window's doesn't normalize IP address for unknown schemes. */
    {   "1234://127.001/", 0, S_OK, FALSE,
        {
            {"1234://127.001/",S_OK,FALSE},
            {"127.001",S_OK,FALSE},
            {"1234://127.001/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"127.001",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"1234://127.001/",S_OK,FALSE},
            {"1234",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[FEDC:BA98::3210]", 0, S_OK, FALSE,
        {
            {"http://[fedc:ba98::3210]/",S_OK,FALSE},
            {"[fedc:ba98::3210]",S_OK,FALSE},
            {"http://[fedc:ba98::3210]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"fedc:ba98::3210",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[FEDC:BA98::3210]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[::]", 0, S_OK, FALSE,
        {
            {"http://[::]/",S_OK,FALSE},
            {"[::]",S_OK,FALSE},
            {"http://[::]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"::",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[::]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[FEDC:BA98::]", 0, S_OK, FALSE,
        {
            {"http://[fedc:ba98::]/",S_OK,FALSE},
            {"[fedc:ba98::]",S_OK,FALSE},
            {"http://[fedc:ba98::]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"fedc:ba98::",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[FEDC:BA98::]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Valid even with 2 byte elision because it doesn't appear the beginning or end. */
    {   "http://[1::3:4:5:6:7:8]", 0, S_OK, FALSE,
        {
            {"http://[1:0:3:4:5:6:7:8]/",S_OK,FALSE},
            {"[1:0:3:4:5:6:7:8]",S_OK,FALSE},
            {"http://[1:0:3:4:5:6:7:8]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"1:0:3:4:5:6:7:8",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[1::3:4:5:6:7:8]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://[v2.34]/", 0, S_OK, FALSE,
        {
            {"http://[v2.34]/",S_OK,FALSE},
            {"[v2.34]",S_OK,FALSE},
            {"http://[v2.34]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"[v2.34]",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[v2.34]/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Windows ignores ':' if they appear after a '[' on a non-IPLiteral host. */
    {   "http://[xyz:12345.com/test", 0, S_OK, FALSE,
        {
            {"http://[xyz:12345.com/test",S_OK,FALSE},
            {"[xyz:12345.com",S_OK,FALSE},
            {"http://[xyz:12345.com/test",S_OK,FALSE},
            {"[xyz:12345.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"[xyz:12345.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test",S_OK,FALSE},
            {"/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[xyz:12345.com/test",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Valid URI since the '[' and ']' don't appear at the beginning and end
     * of the host name (respectively).
     */
    {   "ftp://www.[works].com/", 0, S_OK, FALSE,
        {
            {"ftp://www.[works].com/",S_OK,FALSE},
            {"www.[works].com",S_OK,FALSE},
            {"ftp://www.[works].com/",S_OK,FALSE},
            {"[works].com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.[works].com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp://www.[works].com/",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Considers ':' a delimiter since it appears after the ']'. */
    {   "http://www.google.com]:12345/", 0, S_OK, FALSE,
        {
            {"http://www.google.com]:12345/",S_OK,FALSE},
            {"www.google.com]:12345",S_OK,FALSE},
            {"http://www.google.com]:12345/",S_OK,FALSE},
            {"google.com]",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.google.com]",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.google.com]:12345/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {12345,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Unknown scheme types can have invalid % encoded data in the hostname. */
    {   "zip://w%XXw%GEw.google.com/", 0, S_OK, FALSE,
        {
            {"zip://w%XXw%GEw.google.com/",S_OK,FALSE},
            {"w%XXw%GEw.google.com",S_OK,FALSE},
            {"zip://w%XXw%GEw.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"w%XXw%GEw.google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://w%XXw%GEw.google.com/",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Unknown scheme types hostname doesn't get lower cased. */
    {   "zip://GOOGLE.com/", 0, S_OK, FALSE,
        {
            {"zip://GOOGLE.com/",S_OK,FALSE},
            {"GOOGLE.com",S_OK,FALSE},
            {"zip://GOOGLE.com/",S_OK,FALSE},
            {"GOOGLE.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"GOOGLE.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://GOOGLE.com/",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Hostname get's lower cased for known scheme types. */
    {   "http://WWW.GOOGLE.com/", 0, S_OK, FALSE,
        {
            {"http://www.google.com/",S_OK,FALSE},
            {"www.google.com",S_OK,FALSE},
            {"http://www.google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://WWW.GOOGLE.com/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Characters that get % encoded in the hostname also have their percent
     * encoded forms lower cased.
     */
    {   "http://www.%7Cgoogle|.com/", 0, S_OK, FALSE,
        {
            {"http://www.%7cgoogle%7c.com/",S_OK,FALSE},
            {"www.%7cgoogle%7c.com",S_OK,FALSE},
            {"http://www.%7cgoogle%7c.com/",S_OK,FALSE},
            {"%7cgoogle%7c.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.%7cgoogle%7c.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.%7Cgoogle|.com/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* IPv4 addresses attached to IPv6 can be included in elisions. */
    {   "http://[1:2:3:4:5:6:0.0.0.0]", 0, S_OK, FALSE,
        {
            {"http://[1:2:3:4:5:6::]/",S_OK,FALSE},
            {"[1:2:3:4:5:6::]",S_OK,FALSE},
            {"http://[1:2:3:4:5:6::]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"1:2:3:4:5:6::",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[1:2:3:4:5:6:0.0.0.0]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* IPv4 addresses get normalized. */
    {   "http://[::001.002.003.000]", 0, S_OK, FALSE,
        {
            {"http://[::1.2.3.0]/",S_OK,FALSE},
            {"[::1.2.3.0]",S_OK,FALSE},
            {"http://[::1.2.3.0]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"::1.2.3.0",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[::001.002.003.000]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Windows doesn't do anything to IPv6's in unknown schemes. */
    {   "zip://[0001:0:000:0004:0005:0006:001.002.003.000]", 0, S_OK, FALSE,
        {
            {"zip://[0001:0:000:0004:0005:0006:001.002.003.000]/",S_OK,FALSE},
            {"[0001:0:000:0004:0005:0006:001.002.003.000]",S_OK,FALSE},
            {"zip://[0001:0:000:0004:0005:0006:001.002.003.000]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"0001:0:000:0004:0005:0006:001.002.003.000",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://[0001:0:000:0004:0005:0006:001.002.003.000]",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* IPv4 address is converted into 2 h16 components. */
    {   "http://[ffff::192.222.111.32]", 0, S_OK, FALSE,
        {
            {"http://[ffff::c0de:6f20]/",S_OK,FALSE},
            {"[ffff::c0de:6f20]",S_OK,FALSE},
            {"http://[ffff::c0de:6f20]/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ffff::c0de:6f20",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[ffff::192.222.111.32]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Max value for a port. */
    {   "http://google.com:65535", 0, S_OK, FALSE,
        {
            {"http://google.com:65535/",S_OK,FALSE},
            {"google.com:65535",S_OK,FALSE},
            {"http://google.com:65535/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com:65535",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {65535,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://google.com:65536", 0, S_OK, FALSE,
        {
            {"zip://google.com:65536/",S_OK,FALSE},
            {"google.com:65536",S_OK,FALSE},
            {"zip://google.com:65536/",S_OK,FALSE},
            {"google.com:65536",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com:65536",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://google.com:65536",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://google.com:65536:25", 0, S_OK, FALSE,
        {
            {"zip://google.com:65536:25/",S_OK,FALSE},
            {"google.com:65536:25",S_OK,FALSE},
            {"zip://google.com:65536:25/",S_OK,FALSE},
            {"google.com:65536:25",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com:65536:25",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://google.com:65536:25",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://[::ffff]:abcd", 0, S_OK, FALSE,
        {
            {"zip://[::ffff]:abcd/",S_OK,FALSE},
            {"[::ffff]:abcd",S_OK,FALSE},
            {"zip://[::ffff]:abcd/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"[::ffff]:abcd",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://[::ffff]:abcd",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://127.0.0.1:abcd", 0, S_OK, FALSE,
        {
            {"zip://127.0.0.1:abcd/",S_OK,FALSE},
            {"127.0.0.1:abcd",S_OK,FALSE},
            {"zip://127.0.0.1:abcd/",S_OK,FALSE},
            {"0.1:abcd",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"127.0.0.1:abcd",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://127.0.0.1:abcd",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Port is just copied over. */
    {   "http://google.com:00035", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://google.com:00035",S_OK,FALSE},
            {"google.com:00035",S_OK,FALSE},
            {"http://google.com:00035",S_OK,FALSE,"http://google.com:35"},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com:00035",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {35,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Default port is copied over. */
    {   "http://google.com:80", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://google.com:80",S_OK,FALSE},
            {"google.com:80",S_OK,FALSE},
            {"http://google.com:80",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com:80",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://google.com.uk", 0, S_OK, FALSE,
        {
            {"http://google.com.uk/",S_OK,FALSE},
            {"google.com.uk",S_OK,FALSE},
            {"http://google.com.uk/",S_OK,FALSE},
            {"google.com.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://google.com.com", 0, S_OK, FALSE,
        {
            {"http://google.com.com/",S_OK,FALSE},
            {"google.com.com",S_OK,FALSE},
            {"http://google.com.com/",S_OK,FALSE},
            {"com.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com.com",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://google.uk.1", 0, S_OK, FALSE,
        {
            {"http://google.uk.1/",S_OK,FALSE},
            {"google.uk.1",S_OK,FALSE},
            {"http://google.uk.1/",S_OK,FALSE},
            {"google.uk.1",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.uk.1",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.uk.1",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Since foo isn't a recognized 3 character TLD its considered the domain name. */
    {   "http://google.foo.uk", 0, S_OK, FALSE,
        {
            {"http://google.foo.uk/",S_OK,FALSE},
            {"google.foo.uk",S_OK,FALSE},
            {"http://google.foo.uk/",S_OK,FALSE},
            {"foo.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.foo.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.foo.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://.com", 0, S_OK, FALSE,
        {
            {"http://.com/",S_OK,FALSE},
            {".com",S_OK,FALSE},
            {"http://.com/",S_OK,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://.com",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://.uk", 0, S_OK, FALSE,
        {
            {"http://.uk/",S_OK,FALSE},
            {".uk",S_OK,FALSE},
            {"http://.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {".uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://www.co.google.com.[]", 0, S_OK, FALSE,
        {
            {"http://www.co.google.com.[]/",S_OK,FALSE},
            {"www.co.google.com.[]",S_OK,FALSE},
            {"http://www.co.google.com.[]/",S_OK,FALSE},
            {"google.com.[]",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.co.google.com.[]",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.co.google.com.[]",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://co.uk", 0, S_OK, FALSE,
        {
            {"http://co.uk/",S_OK,FALSE},
            {"co.uk",S_OK,FALSE},
            {"http://co.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"co.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://co.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://www.co.google.us.test", 0, S_OK, FALSE,
        {
            {"http://www.co.google.us.test/",S_OK,FALSE},
            {"www.co.google.us.test",S_OK,FALSE},
            {"http://www.co.google.us.test/",S_OK,FALSE},
            {"us.test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.co.google.us.test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.co.google.us.test",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://gov.uk", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://www.google.com\\test", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"zip://www.google.com\\test",S_OK,FALSE},
            {"www.google.com\\test",S_OK,FALSE},
            {"zip://www.google.com\\test",S_OK,FALSE},
            {"google.com\\test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.google.com\\test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://www.google.com\\test",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "urn:excepts:bad:%XY:encoded", 0, S_OK, FALSE,
        {
            {"urn:excepts:bad:%XY:encoded",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:excepts:bad:%XY:encoded",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"excepts:bad:%XY:encoded",S_OK,FALSE},
            {"excepts:bad:%XY:encoded",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:excepts:bad:%XY:encoded",S_OK,FALSE},
            {"urn",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Since the original URI doesn't contain an extra '/' before the path no % encoded values
     * are decoded and all '%' are encoded.
     */
    {   "file://C:/te%3Es%2Et/tes%t.mp3", 0, S_OK, FALSE,
        {
            {"file:///C:/te%253Es%252Et/tes%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/te%253Es%252Et/tes%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/te%253Es%252Et/tes%25t.mp3",S_OK,FALSE},
            {"/C:/te%253Es%252Et/tes%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://C:/te%3Es%2Et/tes%t.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Since there's a '/' in front of the drive letter, any percent encoded, non-forbidden character
     * is decoded and only %'s in front of invalid hex digits are encoded.
     */
    {   "file:///C:/te%3Es%2Et/t%23es%t.mp3", 0, S_OK, FALSE,
        {
            {"file:///C:/te%3Es.t/t#es%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/te%3Es.t/t#es%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/te%3Es.t/t#es%25t.mp3",S_OK,FALSE},
            {"/C:/te%3Es.t/t#es%25t.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/te%3Es%2Et/t%23es%t.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Only unreserved percent encoded characters are decoded for known schemes that aren't file. */
    {   "http://[::001.002.003.000]/%3F%23%2E%54/test", 0, S_OK, FALSE,
        {
            {"http://[::1.2.3.0]/%3F%23.T/test",S_OK,FALSE},
            {"[::1.2.3.0]",S_OK,FALSE},
            {"http://[::1.2.3.0]/%3F%23.T/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"::1.2.3.0",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/%3F%23.T/test",S_OK,FALSE},
            {"/%3F%23.T/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://[::001.002.003.000]/%3F%23%2E%54/test",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
        },
        {
            {Uri_HOST_IPV6,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters are always encoded for file URIs. */
    {   "file:///C:/\"test\"/test.mp3", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"file:///C:/%22test%22/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/%22test%22/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/%22test%22/test.mp3",S_OK,FALSE},
            {"/C:/%22test%22/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/\"test\"/test.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters are never encoded for unknown scheme types. */
    {   "1234://4294967295/<|>\" test<|>", 0, S_OK, FALSE,
        {
            {"1234://4294967295/<|>\" test<|>",S_OK,FALSE},
            {"4294967295",S_OK,FALSE},
            {"1234://4294967295/<|>\" test<|>",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"4294967295",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/<|>\" test<|>",S_OK,FALSE},
            {"/<|>\" test<|>",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"1234://4294967295/<|>\" test<|>",S_OK,FALSE},
            {"1234",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_IPV4,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Make sure forbidden characters are percent encoded. */
    {   "http://gov.uk/<|> test<|>", 0, S_OK, FALSE,
        {
            {"http://gov.uk/%3C%7C%3E%20test%3C%7C%3E",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/%3C%7C%3E%20test%3C%7C%3E",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/%3C%7C%3E%20test%3C%7C%3E",S_OK,FALSE},
            {"/%3C%7C%3E%20test%3C%7C%3E",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk/<|> test<|>",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://gov.uk/test/../test2/././../test3/.././././", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk/test/../test2/././../test3/.././././",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://gov.uk/test/test2/../../..", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk/test/test2/../../..",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://gov.uk/test/test2/../../.", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk/test/test2/../../.",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file://c:\\tests\\../tests\\./.\\..\\foo%20bar.mp3", 0, S_OK, FALSE,
        {
            {"file:///c:/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///c:/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/c:/foo%2520bar.mp3",S_OK,FALSE},
            {"/c:/foo%2520bar.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\tests\\../tests\\./.\\..\\foo%20bar.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Dot removal happens for unknown scheme types. */
    {   "zip://gov.uk/test/test2/../../.", 0, S_OK, FALSE,
        {
            {"zip://gov.uk/",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"zip://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://gov.uk/test/test2/../../.",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Dot removal doesn't happen if NO_CANONICALIZE is set. */
    {   "http://gov.uk/test/test2/../../.", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://gov.uk/test/test2/../../.",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"http://gov.uk/test/test2/../../.",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test/test2/../../.",S_OK,FALSE},
            {"/test/test2/../../.",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://gov.uk/test/test2/../../.",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Dot removal doesn't happen for wildcard scheme types. */
    {   "*:gov.uk/test/test2/../../.", 0, S_OK, FALSE,
        {
            {"*:gov.uk/test/test2/../../.",S_OK,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"*:gov.uk/test/test2/../../.",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test/test2/../../.",S_OK,FALSE},
            {"/test/test2/../../.",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"*:gov.uk/test/test2/../../.",S_OK,FALSE},
            {"*",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_WILDCARD,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters are encoded for opaque known scheme types. */
    {   "mailto:\"acco<|>unt@example.com\"", 0, S_OK, FALSE,
        {
            {"mailto:%22acco%3C%7C%3Eunt@example.com%22",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mailto:%22acco%3C%7C%3Eunt@example.com%22",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com%22",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"%22acco%3C%7C%3Eunt@example.com%22",S_OK,FALSE},
            {"%22acco%3C%7C%3Eunt@example.com%22",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mailto:\"acco<|>unt@example.com\"",S_OK,FALSE},
            {"mailto",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MAILTO,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "news:test.tes<|>t.com", 0, S_OK, FALSE,
        {
            {"news:test.tes%3C%7C%3Et.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.tes%3C%7C%3Et.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.tes%3C%7C%3Et.com",S_OK,FALSE},
            {"test.tes%3C%7C%3Et.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.tes<|>t.com",S_OK,FALSE},
            {"news",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_NEWS,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Don't encode forbidden characters. */
    {   "news:test.tes<|>t.com", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"news:test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.tes<|>t.com",S_OK,FALSE},
            {"test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.tes<|>t.com",S_OK,FALSE},
            {"news",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_NEWS,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters aren't encoded for unknown, opaque URIs. */
    {   "urn:test.tes<|>t.com", 0, S_OK, FALSE,
        {
            {"urn:test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.tes<|>t.com",S_OK,FALSE},
            {"test.tes<|>t.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:test.tes<|>t.com",S_OK,FALSE},
            {"urn",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Percent encoded unreserved characters are decoded for known opaque URIs. */
    {   "news:test.%74%65%73%74.com", 0, S_OK, FALSE,
        {
            {"news:test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.test.com",S_OK,FALSE},
            {"test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.%74%65%73%74.com",S_OK,FALSE},
            {"news",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_NEWS,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Percent encoded characters are still decoded for known scheme types. */
    {   "news:test.%74%65%73%74.com", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"news:test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.test.com",S_OK,FALSE},
            {"test.test.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"news:test.%74%65%73%74.com",S_OK,FALSE},
            {"news",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_NEWS,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Percent encoded characters aren't decoded for unknown scheme types. */
    {   "urn:test.%74%65%73%74.com", 0, S_OK, FALSE,
        {
            {"urn:test.%74%65%73%74.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:test.%74%65%73%74.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.%74%65%73%74.com",S_OK,FALSE},
            {"test.%74%65%73%74.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"urn:test.%74%65%73%74.com",S_OK,FALSE},
            {"urn",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Unknown scheme types can have invalid % encoded data in query string. */
    {   "zip://www.winehq.org/tests/..?query=%xx&return=y", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/?query=%xx&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/?query=%xx&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=%xx&return=y",S_OK,FALSE},
            {"?query=%xx&return=y",S_OK,FALSE},
            {"zip://www.winehq.org/tests/..?query=%xx&return=y",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Known scheme types can have invalid % encoded data with the right flags. */
    {   "http://www.winehq.org/tests/..?query=%xx&return=y", Uri_CREATE_NO_DECODE_EXTRA_INFO, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=%xx&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=%xx&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=%xx&return=y",S_OK,FALSE},
            {"?query=%xx&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=%xx&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters in query aren't percent encoded for known scheme types with this flag. */
    {   "http://www.winehq.org/tests/..?query=<|>&return=y", Uri_CREATE_NO_DECODE_EXTRA_INFO, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=<|>&return=y",S_OK,FALSE},
            {"?query=<|>&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=<|>&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters in query aren't percent encoded for known scheme types with this flag. */
    {   "http://www.winehq.org/tests/..?query=<|>&return=y", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=<|>&return=y",S_OK,FALSE},
            {"?query=<|>&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=<|>&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters are encoded for known scheme types. */
    {   "http://www.winehq.org/tests/..?query=<|>&return=y", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=%3C%7C%3E&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=%3C%7C%3E&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=%3C%7C%3E&return=y",S_OK,FALSE},
            {"?query=%3C%7C%3E&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=<|>&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters are not encoded for unknown scheme types. */
    {   "zip://www.winehq.org/tests/..?query=<|>&return=y", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/?query=<|>&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=<|>&return=y",S_OK,FALSE},
            {"?query=<|>&return=y",S_OK,FALSE},
            {"zip://www.winehq.org/tests/..?query=<|>&return=y",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters are decoded for known scheme types. */
    {   "http://www.winehq.org/tests/..?query=%30%31&return=y", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=01&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=01&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=01&return=y",S_OK,FALSE},
            {"?query=01&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=%30%31&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters aren't decoded for unknown scheme types. */
    {   "zip://www.winehq.org/tests/..?query=%30%31&return=y", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/?query=%30%31&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/?query=%30%31&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=%30%31&return=y",S_OK,FALSE},
            {"?query=%30%31&return=y",S_OK,FALSE},
            {"zip://www.winehq.org/tests/..?query=%30%31&return=y",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded characters aren't decoded when NO_DECODE_EXTRA_INFO is set. */
    {   "http://www.winehq.org/tests/..?query=%30%31&return=y", Uri_CREATE_NO_DECODE_EXTRA_INFO, S_OK, FALSE,
        {
            {"http://www.winehq.org/?query=%30%31&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/?query=%30%31&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/?query=%30%31&return=y",S_OK,FALSE},
            {"?query=%30%31&return=y",S_OK,FALSE},
            {"http://www.winehq.org/tests/..?query=%30%31&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    {   "http://www.winehq.org?query=12&return=y", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://www.winehq.org?query=12&return=y",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org?query=12&return=y",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"?query=12&return=y",S_OK,FALSE},
            {"?query=12&return=y",S_OK,FALSE},
            {"http://www.winehq.org?query=12&return=y",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Unknown scheme types can have invalid % encoded data in fragments. */
    {   "zip://www.winehq.org/tests/#Te%xx", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/tests/#Te%xx",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/tests/#Te%xx",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te%xx",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://www.winehq.org/tests/#Te%xx",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters in fragment aren't encoded for unknown schemes. */
    {   "zip://www.winehq.org/tests/#Te<|>", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters in the fragment are percent encoded for known schemes. */
    {   "http://www.winehq.org/tests/#Te<|>", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te%3C%7C%3E",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te%3C%7C%3E",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te%3C%7C%3E",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters aren't encoded in the fragment with this flag. */
    {   "http://www.winehq.org/tests/#Te<|>", Uri_CREATE_NO_DECODE_EXTRA_INFO, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Forbidden characters aren't encoded in the fragment with this flag. */
    {   "http://www.winehq.org/tests/#Te<|>", Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te<|>",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te<|>",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters aren't decoded for known scheme types. */
    {   "zip://www.winehq.org/tests/#Te%30%31%32", 0, S_OK, FALSE,
        {
            {"zip://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"zip://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te%30%31%32",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters are decoded for known schemes. */
    {   "http://www.winehq.org/tests/#Te%30%31%32", 0, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te012",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te012",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te012",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters are decoded even if NO_CANONICALIZE is set. */
    {   "http://www.winehq.org/tests/#Te%30%31%32", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te012",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te012",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te012",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Percent encoded, unreserved characters aren't decoded when NO_DECODE_EXTRA is set. */
    {   "http://www.winehq.org/tests/#Te%30%31%32", Uri_CREATE_NO_DECODE_EXTRA_INFO, S_OK, FALSE,
        {
            {"http://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"http://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"#Te%30%31%32",S_OK,FALSE},
            {"www.winehq.org",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tests/",S_OK,FALSE},
            {"/tests/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://www.winehq.org/tests/#Te%30%31%32",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Leading/Trailing whitespace is removed. */
    {   "    http://google.com/     ", 0, S_OK, FALSE,
        {
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "\t\t\r\nhttp\n://g\noogle.co\rm/\n\n\n", 0, S_OK, FALSE,
        {
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http://g\noogle.co\rm/\n\n\n", Uri_CREATE_NO_PRE_PROCESS_HTML_URI, S_OK, FALSE,
        {
            {"http://g%0aoogle.co%0dm/%0A%0A%0A",S_OK,FALSE},
            {"g%0aoogle.co%0dm",S_OK,FALSE},
            {"http://g%0aoogle.co%0dm/%0A%0A%0A",S_OK,FALSE},
            {"g%0aoogle.co%0dm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"g%0aoogle.co%0dm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/%0A%0A%0A",S_OK,FALSE},
            {"/%0A%0A%0A",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://g\noogle.co\rm/\n\n\n",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "zip://g\noogle.co\rm/\n\n\n", Uri_CREATE_NO_PRE_PROCESS_HTML_URI, S_OK, FALSE,
        {
            {"zip://g\noogle.co\rm/\n\n\n",S_OK,FALSE},
            {"g\noogle.co\rm",S_OK,FALSE},
            {"zip://g\noogle.co\rm/\n\n\n",S_OK,FALSE},
            {"g\noogle.co\rm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"g\noogle.co\rm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/\n\n\n",S_OK,FALSE},
            {"/\n\n\n",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://g\noogle.co\rm/\n\n\n",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Since file URLs are usually hierarchical, it returns an empty string
     * for the absolute URI property since it was declared as an opaque URI.
     */
    {   "file:index.html", 0, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"file:index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"index.html",S_OK,FALSE},
            {"index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Doesn't have an absolute since it's opaque, but gets it port set. */
    {   "http:test.com/index.html", 0, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"http:test.com/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.com/index.html",S_OK,FALSE},
            {"test.com/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http:test.com/index.html",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "ftp:test.com/index.html", 0, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp:test.com/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"test.com/index.html",S_OK,FALSE},
            {"test.com/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"ftp:test.com/index.html",S_OK,FALSE},
            {"ftp",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {21,S_OK,FALSE},
            {URL_SCHEME_FTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file://C|/test.mp3", 0, S_OK, FALSE,
        {
            {"file:///C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/test.mp3",S_OK,FALSE},
            {"/C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://C|/test.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file:///C|/test.mp3", 0, S_OK, FALSE,
        {
            {"file:///C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/C:/test.mp3",S_OK,FALSE},
            {"/C:/test.mp3",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///C|/test.mp3",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Extra '/' isn't added before "c:" since USE_DOS_PATH is set and '/' are converted
     * to '\\'.
     */
    {   "file://c:/dir/index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:/dir/index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Extra '/' after "file://" is removed. */
    {   "file:///c:/dir/index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///c:/dir/index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Allow more characters when Uri_CREATE_FILE_USE_DOS_PATH is specified */
    {   "file:///c:/dir\\%%61%20%5Fname/file%2A.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\%a _name\\file*.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\%a _name\\file*.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\%a _name\\file*.html",S_OK,FALSE},
            {"c:\\dir\\%a _name\\file*.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///c:/dir\\%%61%20%5Fname/file%2A.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file://c|/dir\\index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c|/dir\\index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* The backslashes after the scheme name are converted to forward slashes. */
    {   "file:\\\\c:\\dir\\index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"c:\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:\\\\c:\\dir\\index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "file:\\\\c:/dir/index.html", 0, S_OK, FALSE,
        {
            {"file:///c:/dir/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:///c:/dir/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"/c:/dir/index.html",S_OK,FALSE},
            {"/c:/dir/index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:\\\\c:/dir/index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "http:\\\\google.com", 0, S_OK, FALSE,
        {
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"http://google.com/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http:\\\\google.com",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* the "\\\\" aren't converted to "//" for unknown scheme types and it's considered opaque. */
    {   "zip:\\\\google.com", 0, S_OK, FALSE,
        {
            {"zip:\\\\google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip:\\\\google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"\\\\google.com",S_OK,FALSE},
            {"\\\\google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip:\\\\google.com",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Dot segments aren't removed. */
    {   "file://c:\\dir\\../..\\./index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\..\\..\\.\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\..\\..\\.\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\..\\..\\.\\index.html",S_OK,FALSE},
            {"c:\\dir\\..\\..\\.\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\../..\\./index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters aren't percent encoded. */
    {   "file://c:\\dir\\i^|ndex.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://c:\\dir\\i^|ndex.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\i^|ndex.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\i^|ndex.html",S_OK,FALSE},
            {"c:\\dir\\i^|ndex.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://c:\\dir\\i^|ndex.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* The '\' are still converted to '/' even though it's an opaque file URI. */
    {   "file:c:\\dir\\../..\\index.html", 0, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:/dir/../../index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:/dir/../../index.html",S_OK,FALSE},
            {"c:/dir/../../index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:\\dir\\../..\\index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* '/' are still converted to '\' even though it's an opaque URI. */
    {   "file:c:/dir\\../..\\index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:\\dir\\..\\..\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir\\..\\..\\index.html",S_OK,FALSE},
            {"c:\\dir\\..\\..\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:/dir\\../..\\index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Forbidden characters aren't percent encoded. */
    {   "file:c:\\in^|dex.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:\\in^|dex.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\in^|dex.html",S_OK,FALSE},
            {"c:\\in^|dex.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file:c:\\in^|dex.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Doesn't have a UserName since the ':' appears at the beginning of the
     * userinfo section.
     */
    {   "http://:password@gov.uk", 0, S_OK, FALSE,
        {
            {"http://:password@gov.uk/",S_OK,FALSE},
            {":password@gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"password",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://:password@gov.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {":password",S_OK,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Has a UserName since the userinfo section doesn't contain a password. */
    {   "http://@gov.uk", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE,"http://@gov.uk/"},
            {"@gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://@gov.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* ":@" not included in the absolute URI. */
    {   "http://:@gov.uk", 0, S_OK, FALSE,
        {
            {"http://gov.uk/",S_OK,FALSE,"http://:@gov.uk/"},
            {":@gov.uk",S_OK,FALSE},
            {"http://gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://:@gov.uk",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {":",S_OK,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* '@' is included because it's an unknown scheme type. */
    {   "zip://@gov.uk", 0, S_OK, FALSE,
        {
            {"zip://@gov.uk/",S_OK,FALSE},
            {"@gov.uk",S_OK,FALSE},
            {"zip://@gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://@gov.uk",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"",S_OK,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* ":@" are included because it's an unknown scheme type. */
    {   "zip://:@gov.uk", 0, S_OK, FALSE,
        {
            {"zip://:@gov.uk/",S_OK,FALSE},
            {":@gov.uk",S_OK,FALSE},
            {"zip://:@gov.uk/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"gov.uk",S_OK,FALSE},
            {"",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"zip://:@gov.uk",S_OK,FALSE},
            {"zip",S_OK,FALSE},
            {":",S_OK,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "about:blank", 0, S_OK, FALSE,
        {
            {"about:blank",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"about:blank",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"blank",S_OK,FALSE},
            {"blank",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"about:blank",S_OK,FALSE},
            {"about",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_ABOUT,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",0,S_OK,FALSE,
        {
            {"mk:@MSITStore:C:\\Program%20Files/AutoCAD%202008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:C:\\Program%20Files/AutoCAD%202008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:C:\\Program%20Files/AutoCAD%202008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",S_OK,FALSE},
            {"@MSITStore:C:\\Program%20Files/AutoCAD%202008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",0,S_OK,FALSE,
        {
            {"mk:@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",S_OK,FALSE},
            {"@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\home\\test\\chm\\silqhelp.chm::/thesilqquickstartguide.htm",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Two '\' are added to the URI when USE_DOS_PATH is set, and it's a UNC path. */
    {   "file://server/dir/index.html", Uri_CREATE_FILE_USE_DOS_PATH, S_OK, FALSE,
        {
            {"file://\\\\server\\dir\\index.html",S_OK,FALSE},
            {"server",S_OK,FALSE},
            {"file://\\\\server\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"server",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"\\dir\\index.html",S_OK,FALSE},
            {"\\dir\\index.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"file://server/dir/index.html",S_OK,FALSE},
            {"file",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_FILE,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* When CreateUri generates an IUri, it still displays the default port in the
     * authority.
     */
    {   "http://google.com:80/", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"http://google.com:80/",S_OK,FALSE},
            {"google.com:80",S_OK,FALSE},
            {"http://google.com:80/",S_OK,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"google.com",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/",S_OK,FALSE},
            {"/",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"http://google.com:80/",S_OK,FALSE},
            {"http",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK,FALSE},
            {80,S_OK,FALSE},
            {URL_SCHEME_HTTP,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* For res URIs the host is everything up until the first '/'. */
    {   "res://C:\\dir\\file.exe/DATA/test.html", 0, S_OK, FALSE,
        {
            {"res://C:\\dir\\file.exe/DATA/test.html",S_OK,FALSE},
            {"C:\\dir\\file.exe",S_OK,FALSE},
            {"res://C:\\dir\\file.exe/DATA/test.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"C:\\dir\\file.exe",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/DATA/test.html",S_OK,FALSE},
            {"/DATA/test.html",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"res://C:\\dir\\file.exe/DATA/test.html",S_OK,FALSE},
            {"res",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_RES,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Res URI can contain a '|' in the host name. */
    {   "res://c:\\di|r\\file.exe/test", 0, S_OK, FALSE,
        {
            {"res://c:\\di|r\\file.exe/test",S_OK,FALSE},
            {"c:\\di|r\\file.exe",S_OK,FALSE},
            {"res://c:\\di|r\\file.exe/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\di|r\\file.exe",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test",S_OK,FALSE},
            {"/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"res://c:\\di|r\\file.exe/test",S_OK,FALSE},
            {"res",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_RES,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE},
        }
    },
    /* Res URIs can have invalid percent encoded values. */
    {   "res://c:\\dir%xx\\file.exe/test", 0, S_OK, FALSE,
        {
            {"res://c:\\dir%xx\\file.exe/test",S_OK,FALSE},
            {"c:\\dir%xx\\file.exe",S_OK,FALSE},
            {"res://c:\\dir%xx\\file.exe/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\dir%xx\\file.exe",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/test",S_OK,FALSE},
            {"/test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"res://c:\\dir%xx\\file.exe/test",S_OK,FALSE},
            {"res",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_RES,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    /* Res doesn't get forbidden characters percent encoded in it's path. */
    {   "res://c:\\test/tes<|>t", 0, S_OK, FALSE,
        {
            {"res://c:\\test/tes<|>t",S_OK,FALSE},
            {"c:\\test",S_OK,FALSE},
            {"res://c:\\test/tes<|>t",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"c:\\test",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"/tes<|>t",S_OK,FALSE},
            {"/tes<|>t",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"res://c:\\test/tes<|>t",S_OK,FALSE},
            {"res",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_RES,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg", 0, S_OK, FALSE,
        {
            {"mk:@MSITStore:Z:\\dir\\test.chm::/images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/images/xxx.jpg",S_OK,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg", Uri_CREATE_NO_CANONICALIZE, S_OK, FALSE,
        {
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "xx:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg", 0, S_OK, FALSE,
        {
            {"xx:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"xx:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"xx:@MSITStore:Z:\\dir\\test.chm::/html/../images/xxx.jpg",S_OK,FALSE},
            {"xx",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_UNKNOWN,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\dir\\test.chm::/html/../../images/xxx.jpg", 0, S_OK, FALSE,
        {
            {"mk:@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../../images/xxx.jpg",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\dir\\dir2\\..\\test.chm::/html/../../images/xxx.jpg", 0, S_OK, FALSE,
        {
            {"mk:@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"@MSITStore:Z:\\dir\\images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\dir2\\..\\test.chm::/html/../../images/xxx.jpg",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    },
    {   "mk:@MSITStore:Z:\\dir\\test.chm::/html/../../../../images/xxx.jpg", 0, S_OK, FALSE,
        {
            {"mk:images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {".jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE},
            {"images/xxx.jpg",S_OK,FALSE},
            {"images/xxx.jpg",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"mk:@MSITStore:Z:\\dir\\test.chm::/html/../../../../images/xxx.jpg",S_OK,FALSE},
            {"mk",S_OK,FALSE},
            {"",S_FALSE,FALSE},
            {"",S_FALSE,FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK,FALSE},
            {0,S_FALSE,FALSE},
            {URL_SCHEME_MK,S_OK,FALSE},
            {URLZONE_INVALID,E_NOTIMPL,FALSE}
        }
    }
};

typedef struct _invalid_uri {
    const char* uri;
    DWORD       flags;
    BOOL        todo;
} invalid_uri;

static const invalid_uri invalid_uri_tests[] = {
    /* Has to have a scheme name. */
    {"://www.winehq.org",0,FALSE},
    /* Window's doesn't like URI's which are implicitly file paths without the
     * ALLOW_IMPLICIT_FILE_SCHEME flag set.
     */
    {"C:/test/test.mp3",0,FALSE},
    {"\\\\Server/test/test.mp3",0,FALSE},
    {"C:/test/test.mp3",Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME,FALSE},
    {"\\\\Server/test/test.mp3",Uri_CREATE_ALLOW_RELATIVE,FALSE},
    /* Invalid schemes. */
    {"*abcd://not.valid.com",0,FALSE},
    {"*a*b*c*d://not.valid.com",0,FALSE},
    /* Not allowed to have invalid % encoded data. */
    {"ftp://google.co%XX/",0,FALSE},
    /* To many h16 components. */
    {"http://[1:2:3:4:5:6:7:8:9]",0,FALSE},
    /* Not enough room for IPv4 address. */
    {"http://[1:2:3:4:5:6:7:192.0.1.0]",0,FALSE},
    /* Not enough h16 components. */
    {"http://[1:2:3:4]",0,FALSE},
    /* Not enough components including IPv4. */
    {"http://[1:192.0.1.0]",0,FALSE},
    /* Not allowed to have partial IPv4 in IPv6. */
    {"http://[::192.0]",0,FALSE},
    /* Can't have elision of 1 h16 at beginning of address. */
    {"http://[::2:3:4:5:6:7:8]",0,FALSE},
    /* Can't have elision of 1 h16 at end of address. */
    {"http://[1:2:3:4:5:6:7::]",0,FALSE},
    /* Expects a valid IP Literal. */
    {"ftp://[not.valid.uri]/",0,FALSE},
    /* Expects valid port for a known scheme type. */
    {"ftp://www.winehq.org:123fgh",0,FALSE},
    /* Port exceeds USHORT_MAX for known scheme type. */
    {"ftp://www.winehq.org:65536",0,FALSE},
    /* Invalid port with IPv4 address. */
    {"http://www.winehq.org:1abcd",0,FALSE},
    /* Invalid port with IPv6 address. */
    {"http://[::ffff]:32xy",0,FALSE},
    /* Not allowed to have backslashes with NO_CANONICALIZE. */
    {"gopher://www.google.com\\test",Uri_CREATE_NO_CANONICALIZE,FALSE},
    /* Not allowed to have invalid % encoded data in opaque URI path. */
    {"news:test%XX",0,FALSE},
    {"mailto:wine@winehq%G8.com",0,FALSE},
    /* Known scheme types can't have invalid % encoded data in query string. */
    {"http://google.com/?query=te%xx",0,FALSE},
    /* Invalid % encoded data in fragment of know scheme type. */
    {"ftp://google.com/#Test%xx",0,FALSE},
    {"  http://google.com/",Uri_CREATE_NO_PRE_PROCESS_HTML_URI,FALSE},
    {"\n\nhttp://google.com/",Uri_CREATE_NO_PRE_PROCESS_HTML_URI,FALSE},
    {"file://c:\\test<test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    {"file://c:\\test>test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    {"file://c:\\test\"test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    {"file:c:\\test<test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    {"file:c:\\test>test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    {"file:c:\\test\"test",Uri_CREATE_FILE_USE_DOS_PATH,FALSE},
    /* res URIs aren't allowed to have forbidden dos path characters in the
     * hostname.
     */
    {"res://c:\\te<st\\test/test",0,FALSE},
    {"res://c:\\te>st\\test/test",0,FALSE},
    {"res://c:\\te\"st\\test/test",0,FALSE},
    {"res://c:\\test/te%xxst",0,FALSE}
};

typedef struct _uri_equality {
    const char* a;
    DWORD       create_flags_a;
    BOOL        create_todo_a;
    const char* b;
    DWORD       create_flags_b;
    BOOL        create_todo_b;
    BOOL        equal;
    BOOL        todo;
} uri_equality;

static const uri_equality equality_tests[] = {
    {
        "HTTP://www.winehq.org/test dir/./",0,FALSE,
        "http://www.winehq.org/test dir/../test dir/",0,FALSE,
        TRUE, FALSE
    },
    {
        /* http://www.winehq.org/test%20dir */
        "http://%77%77%77%2E%77%69%6E%65%68%71%2E%6F%72%67/%74%65%73%74%20%64%69%72",0,FALSE,
        "http://www.winehq.org/test dir",0,FALSE,
        TRUE, FALSE
    },
    {
        "c:\\test.mp3",Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME,FALSE,
        "file:///c:/test.mp3",0,FALSE,
        TRUE, FALSE
    },
    {
        "ftp://ftp.winehq.org/",0,FALSE,
        "ftp://ftp.winehq.org",0,FALSE,
        TRUE, FALSE
    },
    {
        "ftp://ftp.winehq.org/test/test2/../../testB/",0,FALSE,
        "ftp://ftp.winehq.org/t%45stB/",0,FALSE,
        FALSE, FALSE
    },
    {
        "http://google.com/TEST",0,FALSE,
        "http://google.com/test",0,FALSE,
        FALSE, FALSE
    },
    {
        "http://GOOGLE.com/",0,FALSE,
        "http://google.com/",0,FALSE,
        TRUE, FALSE
    },
    /* Performs case insensitive compare of host names (for known scheme types). */
    {
        "ftp://GOOGLE.com/",Uri_CREATE_NO_CANONICALIZE,FALSE,
        "ftp://google.com/",0,FALSE,
        TRUE, FALSE
    },
    {
        "zip://GOOGLE.com/",0,FALSE,
        "zip://google.com/",0,FALSE,
        FALSE, FALSE
    },
    {
        "file:///c:/TEST/TeST/",0,FALSE,
        "file:///c:/test/test/",0,FALSE,
        TRUE, FALSE
    },
    {
        "file:///server/TEST",0,FALSE,
        "file:///SERVER/TEST",0,FALSE,
        TRUE, FALSE
    },
    {
        "http://google.com",Uri_CREATE_NO_CANONICALIZE,FALSE,
        "http://google.com/",0,FALSE,
        TRUE, FALSE
    },
    {
        "ftp://google.com:21/",0,FALSE,
        "ftp://google.com/",0,FALSE,
        TRUE, FALSE
    },
    {
        "http://google.com:80/",Uri_CREATE_NO_CANONICALIZE,FALSE,
        "http://google.com/",0,FALSE,
        TRUE, FALSE
    },
    {
        "http://google.com:70/",0,FALSE,
        "http://google.com:71/",0,FALSE,
        FALSE, FALSE
    }
};

typedef struct _uri_with_fragment {
    const char* uri;
    const char* fragment;
    DWORD       create_flags;
    HRESULT     create_expected;
    BOOL        create_todo;

    const char* expected_uri;
    BOOL        expected_todo;
} uri_with_fragment;

static const uri_with_fragment uri_fragment_tests[] = {
    {
        "http://google.com/","#fragment",0,S_OK,FALSE,
        "http://google.com/#fragment",FALSE
    },
    {
        "http://google.com/","fragment",0,S_OK,FALSE,
        "http://google.com/#fragment",FALSE
    },
    {
        "zip://test.com/","?test",0,S_OK,FALSE,
        "zip://test.com/#?test",FALSE
    },
    /* The fragment can be empty. */
    {
        "ftp://ftp.google.com/","",0,S_OK,FALSE,
        "ftp://ftp.google.com/#",FALSE
    }
};

typedef struct _uri_builder_property {
    BOOL            change;
    const char      *value;
    const char      *expected_value;
    Uri_PROPERTY    property;
    HRESULT         expected;
    BOOL            todo;
} uri_builder_property;

typedef struct _uri_builder_port {
    BOOL    change;
    BOOL    set;
    DWORD   value;
    HRESULT expected;
    BOOL    todo;
} uri_builder_port;

typedef struct _uri_builder_str_property {
    const char* expected;
    HRESULT     result;
    BOOL        todo;
} uri_builder_str_property;

typedef struct _uri_builder_dword_property {
    DWORD   expected;
    HRESULT result;
    BOOL    todo;
} uri_builder_dword_property;

typedef struct _uri_builder_test {
    const char                  *uri;
    DWORD                       create_flags;
    HRESULT                     create_builder_expected;
    BOOL                        create_builder_todo;

    uri_builder_property        properties[URI_BUILDER_STR_PROPERTY_COUNT];

    uri_builder_port            port_prop;

    DWORD                       uri_flags;
    HRESULT                     uri_hres;
    BOOL                        uri_todo;

    DWORD                       uri_simple_encode_flags;
    HRESULT                     uri_simple_hres;
    BOOL                        uri_simple_todo;

    DWORD                       uri_with_flags;
    DWORD                       uri_with_builder_flags;
    DWORD                       uri_with_encode_flags;
    HRESULT                     uri_with_hres;
    BOOL                        uri_with_todo;

    uri_builder_str_property    expected_str_props[URI_STR_PROPERTY_COUNT];
    uri_builder_dword_property  expected_dword_props[URI_DWORD_PROPERTY_COUNT];
} uri_builder_test;

static const uri_builder_test uri_builder_tests[] = {
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"#fragment",NULL,Uri_PROPERTY_FRAGMENT,S_OK,FALSE},
            {TRUE,"password",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE},
            {TRUE,"?query=x",NULL,Uri_PROPERTY_QUERY,S_OK,FALSE},
            {TRUE,"username",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://username:password@google.com/?query=x#fragment",S_OK},
            {"username:password@google.com",S_OK},
            {"http://google.com/?query=x#fragment",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#fragment",S_OK},
            {"google.com",S_OK},
            {"password",S_OK},
            {"/",S_OK},
            {"/?query=x",S_OK},
            {"?query=x",S_OK},
            {"http://username:password@google.com/?query=x#fragment",S_OK},
            {"http",S_OK},
            {"username:password",S_OK},
            {"username",S_OK}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"test",NULL,Uri_PROPERTY_SCHEME_NAME,S_OK,FALSE}
        },
        {TRUE,TRUE,120,S_OK,FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"test://google.com:120/",S_OK},
            {"google.com:120",S_OK},
            {"test://google.com:120/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"test://google.com:120/",S_OK},
            {"test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {120,S_OK},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "/Test/test dir",Uri_CREATE_ALLOW_RELATIVE,S_OK,FALSE,
        {
            {TRUE,"http",NULL,Uri_PROPERTY_SCHEME_NAME,S_OK,FALSE},
            {TRUE,"::192.2.3.4",NULL,Uri_PROPERTY_HOST,S_OK,FALSE},
            {TRUE,NULL,NULL,Uri_PROPERTY_PATH,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://[::192.2.3.4]/",S_OK},
            {"[::192.2.3.4]",S_OK},
            {"http://[::192.2.3.4]/",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"::192.2.3.4",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://[::192.2.3.4]/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_IPV6,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"Frag","#Frag",Uri_PROPERTY_FRAGMENT,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/#Frag",S_OK},
            {"google.com",S_OK},
            {"http://google.com/#Frag",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#Frag",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/#Frag",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"","#",Uri_PROPERTY_FRAGMENT,S_OK,FALSE},
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/#",S_OK},
            {"google.com",S_OK},
            {"http://google.com/#",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/#",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,":password",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://::password@google.com/",S_OK},
            {"::password@google.com",S_OK},
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {":password",S_OK},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://::password@google.com/",S_OK},
            {"http",S_OK},
            {"::password",S_OK},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "test/test",Uri_CREATE_ALLOW_RELATIVE,S_OK,FALSE,
        {
            {TRUE,"password",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        Uri_CREATE_ALLOW_RELATIVE,S_OK,FALSE,
        0,S_OK,FALSE,
        Uri_CREATE_ALLOW_RELATIVE,0,0,S_OK,FALSE,
        {
            {":password@test/test",S_OK},
            {":password@",S_OK},
            {":password@test/test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"password",S_OK},
            {"test/test",S_OK},
            {"test/test",S_OK},
            {"",S_FALSE},
            {":password@test/test",S_OK},
            {"",S_FALSE},
            {":password",S_OK},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"test/test",NULL,Uri_PROPERTY_PATH,S_OK,FALSE},
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/test/test",S_OK},
            {"google.com",S_OK},
            {"http://google.com/test/test",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/test/test",S_OK},
            {"/test/test",S_OK},
            {"",S_FALSE},
            {"http://google.com/test/test",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "zip:testing/test",0,S_OK,FALSE,
        {
            {TRUE,"test",NULL,Uri_PROPERTY_PATH,S_OK,FALSE},
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"zip:test",S_OK},
            {"",S_FALSE},
            {"zip:test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"test",S_OK},
            {"test",S_OK},
            {"",S_FALSE},
            {"zip:test",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {FALSE},
        },
        /* 555 will be returned from GetPort even though FALSE was passed as the hasPort parameter. */
        {TRUE,FALSE,555,S_OK,FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            /* Still returns 80, even though earlier the port was disabled. */
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {FALSE},
        },
        /* Instead of getting "TRUE" back as the "hasPort" parameter in GetPort,
         * you'll get 122345 instead.
         */
        {TRUE,122345,222,S_OK,FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com:222/",S_OK},
            {"google.com:222",S_OK},
            {"http://google.com:222/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com:222/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {222,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* IUri's created with the IUriBuilder can have ports that exceed USHORT_MAX. */
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {FALSE},
        },
        {TRUE,TRUE,999999,S_OK,FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com:999999/",S_OK},
            {"google.com:999999",S_OK},
            {"http://google.com:999999/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com:999999/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {999999,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"test","?test",Uri_PROPERTY_QUERY,S_OK,FALSE},
        },

        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"http://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/?test",S_OK},
            {"?test",S_OK},
            {"http://google.com/?test",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://:password@google.com/",0,S_OK,FALSE,
        {
            {FALSE},
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://:password@google.com/",S_OK},
            {":password@google.com",S_OK},
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"password",S_OK},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://:password@google.com/",S_OK},
            {"http",S_OK},
            {":password",S_OK},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* IUriBuilder doesn't need a base IUri to build a IUri. */
    {   NULL,0,S_OK,FALSE,
        {
            {TRUE,"http",NULL,Uri_PROPERTY_SCHEME_NAME,S_OK,FALSE},
            {TRUE,"google.com",NULL,Uri_PROPERTY_HOST,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Can't set the scheme name to NULL. */
    {   "zip://google.com/",0,S_OK,FALSE,
        {
            {TRUE,NULL,"zip",Uri_PROPERTY_SCHEME_NAME,E_INVALIDARG,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"zip://google.com/",S_OK},
            {"google.com",S_OK},
            {"zip://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"zip://google.com/",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Can't set the scheme name to an empty string. */
    {   "zip://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"","zip",Uri_PROPERTY_SCHEME_NAME,E_INVALIDARG,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"zip://google.com/",S_OK},
            {"google.com",S_OK},
            {"zip://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"zip://google.com/",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* -1 to CreateUri makes it use the same flags as the base IUri was created with.
     * CreateUriSimple always uses the flags the base IUri was created with (if any).
     */
    {   "http://google.com/../../",Uri_CREATE_NO_CANONICALIZE,S_OK,FALSE,
        {{FALSE}},
        {FALSE},
        -1,S_OK,FALSE,
        0,S_OK,FALSE,
        0,UriBuilder_USE_ORIGINAL_FLAGS,0,S_OK,FALSE,
        {
            {"http://google.com/../../",S_OK},
            {"google.com",S_OK},
            {"http://google.com/../../",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/../../",S_OK},
            {"/../../",S_OK},
            {"",S_FALSE},
            {"http://google.com/../../",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"#Fr<|>g",NULL,Uri_PROPERTY_FRAGMENT,S_OK,FALSE}
        },
        {FALSE},
        -1,S_OK,FALSE,
        0,S_OK,FALSE,
        Uri_CREATE_NO_DECODE_EXTRA_INFO,UriBuilder_USE_ORIGINAL_FLAGS,0,S_OK,FALSE,
        {
            {"http://google.com/#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"http://google.com/#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/#Fr<|>g",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"#Fr<|>g",NULL,Uri_PROPERTY_FRAGMENT,S_OK,FALSE}
        },
        {FALSE},
        Uri_CREATE_CANONICALIZE|Uri_CREATE_NO_CANONICALIZE,E_INVALIDARG,FALSE,
        0,S_OK,FALSE,
        Uri_CREATE_CANONICALIZE|Uri_CREATE_NO_CANONICALIZE,UriBuilder_USE_ORIGINAL_FLAGS,0,S_OK,FALSE,
        {
            {"http://google.com/#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"http://google.com/#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#Fr%3C%7C%3Eg",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/#Fr<|>g",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   NULL,0,S_OK,FALSE,
        {
            {TRUE,"/test/test/",NULL,Uri_PROPERTY_PATH,S_OK,FALSE},
            {TRUE,"#Fr<|>g",NULL,Uri_PROPERTY_FRAGMENT,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"ht%xxtp",NULL,Uri_PROPERTY_SCHEME_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    /* File scheme's can't have a username set. */
    {   "file://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"username",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    /* File schemes can't have a password set. */
    {   "file://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"password",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    /* UserName can't contain any character that is a delimeter for another
     * component that appears after it in a normal URI.
     */
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"user:pass",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"user@google.com",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"user/path",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"user?Query",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"user#Frag",NULL,Uri_PROPERTY_USER_NAME,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"pass@google.com",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"pass/path",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"pass?query",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
       0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"pass#frag",NULL,Uri_PROPERTY_PASSWORD,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"winehq.org/test",NULL,Uri_PROPERTY_HOST,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"winehq.org?test",NULL,Uri_PROPERTY_HOST,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"winehq.org#test",NULL,Uri_PROPERTY_HOST,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    /* Hostname is allowed to contain a ':' (even for known scheme types). */
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"winehq.org:test",NULL,Uri_PROPERTY_HOST,S_OK,FALSE},
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://winehq.org:test/",S_OK},
            {"winehq.org:test",S_OK},
            {"http://winehq.org:test/",S_OK},
            {"winehq.org:test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org:test",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://winehq.org:test/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Can't set the host name to NULL. */
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,NULL,"google.com",Uri_PROPERTY_HOST,E_INVALIDARG,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"http://google.com/",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http://google.com/",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Can set the host name to an empty string. */
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"",NULL,Uri_PROPERTY_HOST,S_OK,FALSE}
        },
        {FALSE},
        0,S_OK,FALSE,
        0,S_OK,FALSE,
        0,0,0,S_OK,FALSE,
        {
            {"http:///",S_OK},
            {"",S_OK},
            {"http:///",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/",S_OK},
            {"",S_FALSE},
            {"http:///",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"/path?query",NULL,Uri_PROPERTY_PATH,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"/path#test",NULL,Uri_PROPERTY_PATH,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    },
    {   "http://google.com/",0,S_OK,FALSE,
        {
            {TRUE,"?path#test",NULL,Uri_PROPERTY_QUERY,S_OK,FALSE}
        },
        {FALSE},
        0,INET_E_INVALID_URL,FALSE,
        0,INET_E_INVALID_URL,FALSE,
        0,0,0,INET_E_INVALID_URL,FALSE
    }
};

typedef struct _uri_builder_remove_test {
    const char  *uri;
    DWORD       create_flags;
    HRESULT     create_builder_expected;
    BOOL        create_builder_todo;

    DWORD       remove_properties;
    HRESULT     remove_expected;
    BOOL        remove_todo;

    const char  *expected_uri;
    DWORD       expected_flags;
    HRESULT     expected_hres;
    BOOL        expected_todo;
} uri_builder_remove_test;

static const uri_builder_remove_test uri_builder_remove_tests[] = {
    {   "http://google.com/test?test=y#Frag",0,S_OK,FALSE,
        Uri_HAS_FRAGMENT|Uri_HAS_PATH|Uri_HAS_QUERY,S_OK,FALSE,
        "http://google.com/",0,S_OK,FALSE
    },
    {   "http://user:pass@winehq.org/",0,S_OK,FALSE,
        Uri_HAS_USER_NAME|Uri_HAS_PASSWORD,S_OK,FALSE,
        "http://winehq.org/",0,S_OK,FALSE
    },
    {   "zip://google.com?Test=x",0,S_OK,FALSE,
        Uri_HAS_HOST,S_OK,FALSE,
        "zip:/?Test=x",0,S_OK,FALSE
    },
    /* Doesn't remove the whole userinfo component. */
    {   "http://username:pass@google.com/",0,S_OK,FALSE,
        Uri_HAS_USER_INFO,S_OK,FALSE,
        "http://username:pass@google.com/",0,S_OK,FALSE
    },
    /* Doesn't remove the domain. */
    {   "http://google.com/",0,S_OK,FALSE,
        Uri_HAS_DOMAIN,S_OK,FALSE,
        "http://google.com/",0,S_OK,FALSE
    },
    {   "http://google.com:120/",0,S_OK,FALSE,
        Uri_HAS_AUTHORITY,S_OK,FALSE,
        "http://google.com:120/",0,S_OK,FALSE
    },
    {   "http://google.com/test.com/",0,S_OK,FALSE,
        Uri_HAS_EXTENSION,S_OK,FALSE,
        "http://google.com/test.com/",0,S_OK,FALSE
    },
    {   "http://google.com/?test=x",0,S_OK,FALSE,
        Uri_HAS_PATH_AND_QUERY,S_OK,FALSE,
        "http://google.com/?test=x",0,S_OK,FALSE
    },
    /* Can't remove the scheme name. */
    {   "http://google.com/?test=x",0,S_OK,FALSE,
        Uri_HAS_SCHEME_NAME|Uri_HAS_QUERY,E_INVALIDARG,FALSE,
        "http://google.com/?test=x",0,S_OK,FALSE
    }
};

typedef struct _uri_combine_str_property {
    const char  *value;
    HRESULT     expected;
    BOOL        todo;
    const char  *broken_value;
    const char  *value_ex;
} uri_combine_str_property;

typedef struct _uri_combine_test {
    const char  *base_uri;
    DWORD       base_create_flags;
    const char  *relative_uri;
    DWORD       relative_create_flags;
    DWORD       combine_flags;
    HRESULT     expected;
    BOOL        todo;

    uri_combine_str_property    str_props[URI_STR_PROPERTY_COUNT];
    uri_dword_property          dword_props[URI_DWORD_PROPERTY_COUNT];
} uri_combine_test;

static const uri_combine_test uri_combine_tests[] = {
    {   "http://google.com/fun/stuff",0,
        "../not/fun/stuff",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/not/fun/stuff",S_OK},
            {"google.com",S_OK},
            {"http://google.com/not/fun/stuff",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/not/fun/stuff",S_OK},
            {"/not/fun/stuff",S_OK},
            {"",S_FALSE},
            {"http://google.com/not/fun/stuff",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/test",0,
        "zip://test.com/cool",0,
        0,S_OK,FALSE,
        {
            {"zip://test.com/cool",S_OK},
            {"test.com",S_OK},
            {"zip://test.com/cool",S_OK},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"/cool",S_OK},
            {"/cool",S_OK},
            {"",S_FALSE},
            {"zip://test.com/cool",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/use/base/path",0,
        "?relative",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/use/base/path?relative",S_OK},
            {"google.com",S_OK},
            {"http://google.com/use/base/path?relative",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/use/base/path",S_OK},
            {"/use/base/path?relative",S_OK},
            {"?relative",S_OK},
            {"http://google.com/use/base/path?relative",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/path",0,
        "/test/../test/.././testing",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/testing",S_OK},
            {"google.com",S_OK},
            {"http://google.com/testing",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/testing",S_OK},
            {"/testing",S_OK},
            {"",S_FALSE},
            {"http://google.com/testing",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/path",0,
        "/test/../test/.././testing",Uri_CREATE_ALLOW_RELATIVE,
        URL_DONT_SIMPLIFY,S_OK,FALSE,
        {
            {"http://google.com:80/test/../test/.././testing",S_OK},
            {"google.com",S_OK},
            {"http://google.com:80/test/../test/.././testing",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/test/../test/.././testing",S_OK},
            {"/test/../test/.././testing",S_OK},
            {"",S_FALSE},
            {"http://google.com:80/test/../test/.././testing",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/test/abc",0,
        "testing/abc/../test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/test/testing/test",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/test/testing/test",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/test/testing/test",S_OK},
            {"/test/testing/test",S_OK},
            {"",S_FALSE},
            {"http://winehq.org/test/testing/test",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/test/abc",0,
        "testing/abc/../test",Uri_CREATE_ALLOW_RELATIVE,
        URL_DONT_SIMPLIFY,S_OK,FALSE,
        {
            {"http://winehq.org:80/test/testing/abc/../test",S_OK},
            /* Default port is hidden in the authority. */
            {"winehq.org",S_OK},
            {"http://winehq.org:80/test/testing/abc/../test",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/test/testing/abc/../test",S_OK},
            {"/test/testing/abc/../test",S_OK},
            {"",S_FALSE},
            {"http://winehq.org:80/test/testing/abc/../test",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/test?query",0,
        "testing",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/testing",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/testing",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/testing",S_OK},
            {"/testing",S_OK},
            {"",S_FALSE},
            {"http://winehq.org/testing",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/test#frag",0,
        "testing",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/testing",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/testing",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/testing",S_OK},
            {"/testing",S_OK},
            {"",S_FALSE},
            {"http://winehq.org/testing",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "testing?query#frag",Uri_CREATE_ALLOW_RELATIVE,
        "test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"test",S_OK},
            {"",S_FALSE},
            {"test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"test",S_OK},
            {"test",S_OK},
            {"",S_FALSE},
            {"test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "file:///c:/test/test",0,
        "/testing.mp3",Uri_CREATE_ALLOW_RELATIVE,
        URL_FILE_USE_PATHURL,S_OK,FALSE,
        {
            {"file://c:\\testing.mp3",S_OK},
            {"",S_FALSE},
            {"file://c:\\testing.mp3",S_OK},
            {"",S_FALSE},
            {".mp3",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"c:\\testing.mp3",S_OK},
            {"c:\\testing.mp3",S_OK},
            {"",S_FALSE},
            {"file://c:\\testing.mp3",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "file:///c:/test/test",0,
        "/testing.mp3",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"file:///c:/testing.mp3",S_OK},
            {"",S_FALSE},
            {"file:///c:/testing.mp3",S_OK},
            {"",S_FALSE},
            {".mp3",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"/c:/testing.mp3",S_OK},
            {"/c:/testing.mp3",S_OK},
            {"",S_FALSE},
            {"file:///c:/testing.mp3",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "file://test.com/test/test",0,
        "/testing.mp3",Uri_CREATE_ALLOW_RELATIVE,
        URL_FILE_USE_PATHURL,S_OK,FALSE,
        {
            {"file://\\\\test.com\\testing.mp3",S_OK},
            {"test.com",S_OK},
            {"file://\\\\test.com\\testing.mp3",S_OK},
            {"test.com",S_OK},
            {".mp3",S_OK},
            {"",S_FALSE},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"\\testing.mp3",S_OK},
            {"\\testing.mp3",S_OK},
            {"",S_FALSE},
            {"file://\\\\test.com\\testing.mp3",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* URL_DONT_SIMPLIFY has no effect. */
    {   "http://google.com/test",0,
        "zip://test.com/cool/../cool/test",0,
        URL_DONT_SIMPLIFY,S_OK,FALSE,
        {
            {"zip://test.com/cool/test",S_OK,FALSE,NULL,"zip://test.com/cool/../cool/test"},
            {"test.com",S_OK},
            {"zip://test.com/cool/test",S_OK,FALSE,NULL,"zip://test.com/cool/../cool/test"},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"/cool/test",S_OK,FALSE,NULL,"/cool/../cool/test"},
            {"/cool/test",S_OK,FALSE,NULL,"/cool/../cool/test"},
            {"",S_FALSE},
            /* The resulting IUri has the same Raw URI as the relative URI (only IE 8).
             * On IE 7 it reduces the path in the Raw URI.
             */
            {"zip://test.com/cool/../cool/test",S_OK,FALSE,"zip://test.com/cool/test"},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* FILE_USE_PATHURL has no effect in IE 8, in IE 7 the
     * resulting URI is converted into a dos path.
     */
    {   "http://google.com/test",0,
        "file:///c:/test/",0,
        URL_FILE_USE_PATHURL,S_OK,FALSE,
        {
            {"file:///c:/test/",S_OK,FALSE,"file://c:\\test\\"},
            {"",S_FALSE},
            {"file:///c:/test/",S_OK,FALSE,"file://c:\\test\\"},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"/c:/test/",S_OK,FALSE,"c:\\test\\"},
            {"/c:/test/",S_OK,FALSE,"c:\\test\\"},
            {"",S_FALSE},
            {"file:///c:/test/",S_OK,FALSE,"file://c:\\test\\"},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://google.com/test",0,
        "http://test.com/test#%30test",0,
        URL_DONT_UNESCAPE_EXTRA_INFO,S_OK,FALSE,
        {
            {"http://test.com/test#0test",S_OK,FALSE,NULL,"http://test.com/test#%30test"},
            {"test.com",S_OK},
            {"http://test.com/test#0test",S_OK,FALSE,NULL,"http://test.com/test#%30test"},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"#0test",S_OK,FALSE,NULL,"#%30test"},
            {"test.com",S_OK},
            {"",S_FALSE},
            {"/test",S_OK},
            {"/test",S_OK},
            {"",S_FALSE},
            /* IE 7 decodes the %30 to a 0 in the Raw URI. */
            {"http://test.com/test#%30test",S_OK,FALSE,"http://test.com/test#0test"},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Windows validates the path component from the relative Uri. */
    {   "http://google.com/test",0,
        "/Te%XXst",Uri_CREATE_ALLOW_RELATIVE,
        0,E_INVALIDARG,FALSE
    },
    /* Windows doesn't validate the query from the relative Uri. */
    {   "http://google.com/test",0,
        "?Tes%XXt",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/test?Tes%XXt",S_OK},
            {"google.com",S_OK},
            {"http://google.com/test?Tes%XXt",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/test",S_OK},
            {"/test?Tes%XXt",S_OK},
            {"?Tes%XXt",S_OK},
            {"http://google.com/test?Tes%XXt",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Windows doesn't validate the fragment from the relative Uri. */
    {   "http://google.com/test",0,
        "#Tes%XXt",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/test#Tes%XXt",S_OK},
            {"google.com",S_OK},
            {"http://google.com/test#Tes%XXt",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"#Tes%XXt",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/test",S_OK},
            {"/test",S_OK},
            {"",S_FALSE},
            {"http://google.com/test#Tes%XXt",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Creates an IUri which contains an invalid dos path char. */
    {   "file:///c:/test",0,
        "/test<ing",Uri_CREATE_ALLOW_RELATIVE,
        URL_FILE_USE_PATHURL,S_OK,FALSE,
        {
            {"file://c:\\test<ing",S_OK},
            {"",S_FALSE},
            {"file://c:\\test<ing",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"c:\\test<ing",S_OK},
            {"c:\\test<ing",S_OK},
            {"",S_FALSE},
            {"file://c:\\test<ing",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* Appends the path after the drive letter (if any). */
    {   "file:///c:/test",0,
        "/c:/testing",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"file:///c:/c:/testing",S_OK},
            {"",S_FALSE},
            {"file:///c:/c:/testing",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"/c:/c:/testing",S_OK},
            {"/c:/c:/testing",S_OK},
            {"",S_FALSE},
            {"file:///c:/c:/testing",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* A '/' is added if the base URI doesn't have a path and the
     * relative URI doesn't contain a path (since the base URI is
     * hierarchical.
     */
    {   "http://google.com",Uri_CREATE_NO_CANONICALIZE,
        "?test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"http://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/?test",S_OK},
            {"?test",S_OK},
            {"http://google.com/?test",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "zip://google.com",Uri_CREATE_NO_CANONICALIZE,
        "?test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"zip://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"zip://google.com/?test",S_OK},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"google.com",S_OK},
            {"",S_FALSE},
            {"/",S_OK},
            {"/?test",S_OK},
            {"?test",S_OK},
            {"zip://google.com/?test",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    /* No path is appended since the base URI is opaque. */
    {   "zip:?testing",0,
        "?test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"zip:?test",S_OK},
            {"",S_FALSE},
            {"zip:?test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_OK},
            {"?test",S_OK},
            {"?test",S_OK},
            {"zip:?test",S_OK},
            {"zip",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_UNKNOWN,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "file:///c:/",0,
        "../testing/test",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"file:///c:/testing/test",S_OK},
            {"",S_FALSE},
            {"file:///c:/testing/test",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"",S_FALSE},
            {"/c:/testing/test",S_OK},
            {"/c:/testing/test",S_OK},
            {"",S_FALSE},
            {"file:///c:/testing/test",S_OK},
            {"file",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_UNKNOWN,S_OK},
            {0,S_FALSE},
            {URL_SCHEME_FILE,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/dir/testfile",0,
        "test?querystring",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/dir/test",S_OK},
            {"/dir/test?querystring",S_OK},
            {"?querystring",S_OK},
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/dir/test",0,
        "test?querystring",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"",S_FALSE},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/dir/test",S_OK},
            {"/dir/test?querystring",S_OK},
            {"?querystring",S_OK},
            {"http://winehq.org/dir/test?querystring",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    },
    {   "http://winehq.org/dir/test?querystring",0,
        "#hash",Uri_CREATE_ALLOW_RELATIVE,
        0,S_OK,FALSE,
        {
            {"http://winehq.org/dir/test?querystring#hash",S_OK},
            {"winehq.org",S_OK},
            {"http://winehq.org/dir/test?querystring#hash",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"#hash",S_OK},
            {"winehq.org",S_OK},
            {"",S_FALSE},
            {"/dir/test",S_OK},
            {"/dir/test?querystring",S_OK},
            {"?querystring",S_OK},
            {"http://winehq.org/dir/test?querystring#hash",S_OK},
            {"http",S_OK},
            {"",S_FALSE},
            {"",S_FALSE}
        },
        {
            {Uri_HOST_DNS,S_OK},
            {80,S_OK},
            {URL_SCHEME_HTTP,S_OK},
            {URLZONE_INVALID,E_NOTIMPL}
        }
    }
};

typedef struct _uri_parse_test {
    const char  *uri;
    DWORD       uri_flags;
    PARSEACTION action;
    DWORD       flags;
    const char  *property;
    HRESULT     expected;
    BOOL        todo;
} uri_parse_test;

static const uri_parse_test uri_parse_tests[] = {
    /* PARSE_CANONICALIZE tests. */
    {"zip://google.com/test<|>",0,PARSE_CANONICALIZE,0,"zip://google.com/test<|>",S_OK,FALSE},
    {"http://google.com/test<|>",0,PARSE_CANONICALIZE,0,"http://google.com/test%3C%7C%3E",S_OK,FALSE},
    {"http://google.com/%30%23%3F",0,PARSE_CANONICALIZE,URL_UNESCAPE,"http://google.com/0#?",S_OK,FALSE},
    {"test <|>",Uri_CREATE_ALLOW_RELATIVE,PARSE_CANONICALIZE,URL_ESCAPE_UNSAFE,"test %3C%7C%3E",S_OK,FALSE},
    {"test <|>",Uri_CREATE_ALLOW_RELATIVE,PARSE_CANONICALIZE,URL_ESCAPE_SPACES_ONLY,"test%20<|>",S_OK,FALSE},
    {"test%20<|>",Uri_CREATE_ALLOW_RELATIVE,PARSE_CANONICALIZE,URL_UNESCAPE|URL_ESCAPE_UNSAFE,"test%20%3C%7C%3E",S_OK,FALSE},
    {"http://google.com/%20",0,PARSE_CANONICALIZE,URL_ESCAPE_PERCENT,"http://google.com/%2520",S_OK,FALSE},
    {"http://google.com/test/../",Uri_CREATE_NO_CANONICALIZE,PARSE_CANONICALIZE,URL_DONT_SIMPLIFY,"http://google.com/test/../",S_OK,FALSE},
    {"http://google.com/test/../",Uri_CREATE_NO_CANONICALIZE,PARSE_CANONICALIZE,URL_NO_META,"http://google.com/test/../",S_OK,FALSE},
    {"http://google.com/test/../",Uri_CREATE_NO_CANONICALIZE,PARSE_CANONICALIZE,0,"http://google.com/",S_OK,FALSE},
    {"zip://google.com/test/../",Uri_CREATE_NO_CANONICALIZE,PARSE_CANONICALIZE,0,"zip://google.com/",S_OK,FALSE},
    {"file:///c:/test/../test",Uri_CREATE_NO_CANONICALIZE,PARSE_CANONICALIZE,URL_DONT_SIMPLIFY,"file:///c:/test/../test",S_OK,FALSE},

    /* PARSE_FRIENDLY tests. */
    {"http://test@google.com/test#test",0,PARSE_FRIENDLY,0,"http://google.com/test#test",S_OK,FALSE},
    {"zip://test@google.com/test",0,PARSE_FRIENDLY,0,"zip://test@google.com/test",S_OK,FALSE},

    /* PARSE_ROOTDOCUMENT tests. */
    {"http://google.com:200/test/test",0,PARSE_ROOTDOCUMENT,0,"http://google.com:200/",S_OK,FALSE},
    {"http://google.com",Uri_CREATE_NO_CANONICALIZE,PARSE_ROOTDOCUMENT,0,"http://google.com/",S_OK,FALSE},
    {"zip://google.com/",0,PARSE_ROOTDOCUMENT,0,"",S_OK,FALSE},
    {"file:///c:/testing/",0,PARSE_ROOTDOCUMENT,0,"",S_OK,FALSE},
    {"file://server/test",0,PARSE_ROOTDOCUMENT,0,"",S_OK,FALSE},
    {"zip:test/test",0,PARSE_ROOTDOCUMENT,0,"",S_OK,FALSE},

    /* PARSE_DOCUMENT tests. */
    {"http://test@google.com/test?query#frag",0,PARSE_DOCUMENT,0,"http://test@google.com/test?query",S_OK,FALSE},
    {"http:testing#frag",0,PARSE_DOCUMENT,0,"",S_OK,FALSE},
    {"file:///c:/test#frag",0,PARSE_DOCUMENT,0,"",S_OK,FALSE},
    {"zip://google.com/#frag",0,PARSE_DOCUMENT,0,"",S_OK,FALSE},
    {"zip:test#frag",0,PARSE_DOCUMENT,0,"",S_OK,FALSE},
    {"testing#frag",Uri_CREATE_ALLOW_RELATIVE,PARSE_DOCUMENT,0,"",S_OK,FALSE},

    /* PARSE_PATH_FROM_URL tests. */
    {"file:///c:/test.mp3",0,PARSE_PATH_FROM_URL,0,"c:\\test.mp3",S_OK,FALSE},
    {"file:///c:/t<|>est.mp3",0,PARSE_PATH_FROM_URL,0,"c:\\t<|>est.mp3",S_OK,FALSE},
    {"file:///c:/te%XX t/",0,PARSE_PATH_FROM_URL,0,"c:\\te%XX t\\",S_OK,FALSE},
    {"file://server/test",0,PARSE_PATH_FROM_URL,0,"\\\\server\\test",S_OK,FALSE},
    {"http://google.com/",0,PARSE_PATH_FROM_URL,0,"",E_INVALIDARG,FALSE},

    /* PARSE_URL_FROM_PATH tests. */
    /* This function almost seems to useless (just returns the absolute uri). */
    {"test.com",Uri_CREATE_ALLOW_RELATIVE,PARSE_URL_FROM_PATH,0,"test.com",S_OK,FALSE},
    {"/test/test",Uri_CREATE_ALLOW_RELATIVE,PARSE_URL_FROM_PATH,0,"/test/test",S_OK,FALSE},
    {"file://c:\\test\\test",Uri_CREATE_FILE_USE_DOS_PATH,PARSE_URL_FROM_PATH,0,"file://c:\\test\\test",S_OK,FALSE},
    {"file:c:/test",0,PARSE_URL_FROM_PATH,0,"",S_OK,FALSE},
    {"http:google.com/",0,PARSE_URL_FROM_PATH,0,"",S_OK,FALSE},

    /* PARSE_SCHEMA tests. */
    {"http://google.com/test",0,PARSE_SCHEMA,0,"http",S_OK,FALSE},
    {"test",Uri_CREATE_ALLOW_RELATIVE,PARSE_SCHEMA,0,"",S_OK,FALSE},

    /* PARSE_SITE tests. */
    {"http://google.uk.com/",0,PARSE_SITE,0,"google.uk.com",S_OK,FALSE},
    {"http://google.com.com/",0,PARSE_SITE,0,"google.com.com",S_OK,FALSE},
    {"google.com",Uri_CREATE_ALLOW_RELATIVE,PARSE_SITE,0,"",S_OK,FALSE},
    {"file://server/test",0,PARSE_SITE,0,"server",S_OK,FALSE},

    /* PARSE_DOMAIN tests. */
    {"http://google.com.uk/",0,PARSE_DOMAIN,0,"google.com.uk",S_OK,FALSE},
    {"http://google.com.com/",0,PARSE_DOMAIN,0,"com.com",S_OK,FALSE},
    {"test/test",Uri_CREATE_ALLOW_RELATIVE,PARSE_DOMAIN,0,"",S_OK,FALSE},
    {"file://server/test",0,PARSE_DOMAIN,0,"",S_OK,FALSE},

    /* PARSE_LOCATION and PARSE_ANCHOR tests. */
    {"http://google.com/test#Test",0,PARSE_ANCHOR,0,"#Test",S_OK,FALSE},
    {"http://google.com/test#Test",0,PARSE_LOCATION,0,"#Test",S_OK,FALSE},
    {"test",Uri_CREATE_ALLOW_RELATIVE,PARSE_ANCHOR,0,"",S_OK,FALSE},
    {"test",Uri_CREATE_ALLOW_RELATIVE,PARSE_LOCATION,0,"",S_OK,FALSE}
};

static inline LPWSTR a2w(LPCSTR str) {
    LPWSTR ret = NULL;

    if(str) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static inline BOOL heap_free(void* mem) {
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline DWORD strcmp_aw(LPCSTR strA, LPCWSTR strB) {
    LPWSTR strAW = a2w(strA);
    DWORD ret = lstrcmpW(strAW, strB);
    heap_free(strAW);
    return ret;
}

static inline ULONG get_refcnt(IUri *uri) {
    IUri_AddRef(uri);
    return IUri_Release(uri);
}

static void change_property(IUriBuilder *builder, const uri_builder_property *prop,
                            DWORD test_index) {
    HRESULT hr;
    LPWSTR valueW;

    valueW = a2w(prop->value);
    switch(prop->property) {
    case Uri_PROPERTY_FRAGMENT:
        hr = IUriBuilder_SetFragment(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_HOST:
        hr = IUriBuilder_SetHost(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_PASSWORD:
        hr = IUriBuilder_SetPassword(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_PATH:
        hr = IUriBuilder_SetPath(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_QUERY:
        hr = IUriBuilder_SetQuery(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_SCHEME_NAME:
        hr = IUriBuilder_SetSchemeName(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    case Uri_PROPERTY_USER_NAME:
        hr = IUriBuilder_SetUserName(builder, valueW);
        if(prop->todo) {
            todo_wine {
                ok(hr == prop->expected,
                    "Error: IUriBuilder_SetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, prop->expected, test_index);
            }
        } else {
            ok(hr == prop->expected,
                "Error: IUriBuilder_SetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, prop->expected, test_index);
        }
        break;
    default:
        trace("Unsupported operation for %d on uri_builder_tests[%d].\n", prop->property, test_index);
    }

    heap_free(valueW);
}

/*
 * Simple tests to make sure the CreateUri function handles invalid flag combinations
 * correctly.
 */
static void test_CreateUri_InvalidFlags(void) {
    DWORD i;

    for(i = 0; i < sizeof(invalid_flag_tests)/sizeof(invalid_flag_tests[0]); ++i) {
        HRESULT hr;
        IUri *uri = (void*) 0xdeadbeef;

        hr = pCreateUri(http_urlW, invalid_flag_tests[i].flags, 0, &uri);
        ok(hr == invalid_flag_tests[i].expected, "Error: CreateUri returned 0x%08x, expected 0x%08x, flags=0x%08x\n",
                hr, invalid_flag_tests[i].expected, invalid_flag_tests[i].flags);
        ok(uri == NULL, "Error: expected the IUri to be NULL, but it was %p instead\n", uri);
    }
}

static void test_CreateUri_InvalidArgs(void) {
    HRESULT hr;
    IUri *uri = (void*) 0xdeadbeef;

    const WCHAR invalidW[] = {'i','n','v','a','l','i','d',0};
    static const WCHAR emptyW[] = {0};

    hr = pCreateUri(http_urlW, 0, 0, NULL);
    ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x\n", hr, E_INVALIDARG);

    hr = pCreateUri(NULL, 0, 0, &uri);
    ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x\n", hr, E_INVALIDARG);
    ok(uri == NULL, "Error: Expected the IUri to be NULL, but it was %p instead\n", uri);

    uri = (void*) 0xdeadbeef;
    hr = pCreateUri(invalidW, 0, 0, &uri);
    ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    ok(uri == NULL, "Error: Expected the IUri to be NULL, but it was %p instead\n", uri);

    uri = (void*) 0xdeadbeef;
    hr = pCreateUri(emptyW, 0, 0, &uri);
    ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    ok(uri == NULL, "Error: Expected the IUri to be NULL, but it was %p instead\n", uri);
}

static void test_CreateUri_InvalidUri(void) {
    DWORD i;

    for(i = 0; i < sizeof(invalid_uri_tests)/sizeof(invalid_uri_tests[0]); ++i) {
        invalid_uri test = invalid_uri_tests[i];
        IUri *uri = NULL;
        LPWSTR uriW;
        HRESULT hr;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.flags, 0, &uri);
        if(test.todo) {
            todo_wine {
                ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x on invalid_uri_tests[%d].\n",
                    hr, E_INVALIDARG, i);
            }
        } else {
            ok(hr == E_INVALIDARG, "Error: CreateUri returned 0x%08x, expected 0x%08x on invalid_uri_tests[%d].\n",
                hr, E_INVALIDARG, i);
        }
        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_GetPropertyBSTR(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    /* Make sure GetPropertyBSTR handles invalid args correctly. */
    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        BSTR received = NULL;

        hr = IUri_GetPropertyBSTR(uri, Uri_PROPERTY_RAW_URI, NULL, 0);
        ok(hr == E_POINTER, "Error: GetPropertyBSTR returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        /* Make sure it handles a invalid Uri_PROPERTY's correctly. */
        hr = IUri_GetPropertyBSTR(uri, Uri_PROPERTY_PORT, &received, 0);
        ok(hr == S_OK, "Error: GetPropertyBSTR returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
        ok(received != NULL, "Error: Expected the string not to be NULL.\n");
        ok(!SysStringLen(received), "Error: Expected the string to be of len=0 but it was %d instead.\n", SysStringLen(received));
        SysFreeString(received);

        /* Make sure it handles the ZONE property correctly. */
        received = NULL;
        hr = IUri_GetPropertyBSTR(uri, Uri_PROPERTY_ZONE, &received, 0);
        ok(hr == S_FALSE, "Error: GetPropertyBSTR returned 0x%08x, expected 0x%08x.\n", hr, S_FALSE);
        ok(received != NULL, "Error: Expected the string not to be NULL.\n");
        ok(!SysStringLen(received), "Error: Expected the string to be of len=0 but it was %d instead.\n", SysStringLen(received));
        SysFreeString(received);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x. Failed on uri_tests[%d].\n",
                        hr, test.create_expected, i);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x. Failed on uri_tests[%d].\n",
                    hr, test.create_expected, i);
        }

        if(SUCCEEDED(hr)) {
            DWORD j;

            /* Checks all the string properties of the uri. */
            for(j = Uri_PROPERTY_STRING_START; j <= Uri_PROPERTY_STRING_LAST; ++j) {
                BSTR received = NULL;
                uri_str_property prop = test.str_props[j];

                hr = IUri_GetPropertyBSTR(uri, j, &received, 0);
                if(prop.todo) {
                    todo_wine {
                        ok(hr == prop.expected, "GetPropertyBSTR returned 0x%08x, expected 0x%08x. On uri_tests[%d].str_props[%d].\n",
                                hr, prop.expected, i, j);
                    }
                    todo_wine {
                        ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                                "Expected %s but got %s on uri_tests[%d].str_props[%d].\n",
                                prop.value, wine_dbgstr_w(received), i, j);
                    }
                } else {
                    ok(hr == prop.expected, "GetPropertyBSTR returned 0x%08x, expected 0x%08x. On uri_tests[%d].str_props[%d].\n",
                            hr, prop.expected, i, j);
                    ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                            "Expected %s but got %s on uri_tests[%d].str_props[%d].\n",
                            prop.value, wine_dbgstr_w(received), i, j);
                }

                SysFreeString(received);
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_GetPropertyDWORD(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        DWORD received = 0xdeadbeef;

        hr = IUri_GetPropertyDWORD(uri, Uri_PROPERTY_DWORD_START, NULL, 0);
        ok(hr == E_INVALIDARG, "Error: GetPropertyDWORD returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

        hr = IUri_GetPropertyDWORD(uri, Uri_PROPERTY_ABSOLUTE_URI, &received, 0);
        ok(hr == E_INVALIDARG, "Error: GetPropertyDWORD returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
        ok(received == 0, "Error: Expected received=%d but instead received=%d.\n", 0, received);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x. Failed on uri_tests[%d].\n",
                        hr, test.create_expected, i);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x. Failed on uri_tests[%d].\n",
                    hr, test.create_expected, i);
        }

        if(SUCCEEDED(hr)) {
            DWORD j;

            /* Checks all the DWORD properties of the uri. */
            for(j = 0; j < sizeof(test.dword_props)/sizeof(test.dword_props[0]); ++j) {
                DWORD received;
                uri_dword_property prop = test.dword_props[j];

                hr = IUri_GetPropertyDWORD(uri, j+Uri_PROPERTY_DWORD_START, &received, 0);
                if(prop.todo) {
                    todo_wine {
                        ok(hr == prop.expected, "GetPropertyDWORD returned 0x%08x, expected 0x%08x. On uri_tests[%d].dword_props[%d].\n",
                                hr, prop.expected, i, j);
                    }
                    todo_wine {
                        ok(prop.value == received, "Expected %d but got %d on uri_tests[%d].dword_props[%d].\n",
                                prop.value, received, i, j);
                    }
                } else {
                    ok(hr == prop.expected, "GetPropertyDWORD returned 0x%08x, expected 0x%08x. On uri_tests[%d].dword_props[%d].\n",
                            hr, prop.expected, i, j);
                    ok(prop.value == received, "Expected %d but got %d on uri_tests[%d].dword_props[%d].\n",
                            prop.value, received, i, j);
                }
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

/* Tests all the 'Get*' property functions which deal with strings. */
static void test_IUri_GetStrProperties(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    /* Make sure all the 'Get*' string property functions handle invalid args correctly. */
    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        hr = IUri_GetAbsoluteUri(uri, NULL);
        ok(hr == E_POINTER, "Error: GetAbsoluteUri returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetAuthority(uri, NULL);
        ok(hr == E_POINTER, "Error: GetAuthority returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetDisplayUri(uri, NULL);
        ok(hr == E_POINTER, "Error: GetDisplayUri returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetDomain(uri, NULL);
        ok(hr == E_POINTER, "Error: GetDomain returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetExtension(uri, NULL);
        ok(hr == E_POINTER, "Error: GetExtension returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetFragment(uri, NULL);
        ok(hr == E_POINTER, "Error: GetFragment returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetHost(uri, NULL);
        ok(hr == E_POINTER, "Error: GetHost returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetPassword(uri, NULL);
        ok(hr == E_POINTER, "Error: GetPassword returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetPath(uri, NULL);
        ok(hr == E_POINTER, "Error: GetPath returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetPathAndQuery(uri, NULL);
        ok(hr == E_POINTER, "Error: GetPathAndQuery returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetQuery(uri, NULL);
        ok(hr == E_POINTER, "Error: GetQuery returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetRawUri(uri, NULL);
        ok(hr == E_POINTER, "Error: GetRawUri returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetSchemeName(uri, NULL);
        ok(hr == E_POINTER, "Error: GetSchemeName returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetUserInfo(uri, NULL);
        ok(hr == E_POINTER, "Error: GetUserInfo returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        hr = IUri_GetUserName(uri, NULL);
        ok(hr == E_POINTER, "Error: GetUserName returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, test.create_expected, i);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                    hr, test.create_expected, i);
        }

        if(SUCCEEDED(hr)) {
            uri_str_property prop;
            BSTR received = NULL;

            /* GetAbsoluteUri() tests. */
            prop = test.str_props[Uri_PROPERTY_ABSOLUTE_URI];
            hr = IUri_GetAbsoluteUri(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetAbsoluteUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                            "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetAbsoluteUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                        "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetAuthority() tests. */
            prop = test.str_props[Uri_PROPERTY_AUTHORITY];
            hr = IUri_GetAuthority(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetAuthority returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetAuthority returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetDisplayUri() tests. */
            prop = test.str_props[Uri_PROPERTY_DISPLAY_URI];
            hr = IUri_GetDisplayUri(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetDisplayUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                            "Error: Expected %s but got %s on uri_test[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetDisplayUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received) || broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                        "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetDomain() tests. */
            prop = test.str_props[Uri_PROPERTY_DOMAIN];
            hr = IUri_GetDomain(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetDomain returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetDomain returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetExtension() tests. */
            prop = test.str_props[Uri_PROPERTY_EXTENSION];
            hr = IUri_GetExtension(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetExtension returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetExtension returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetFragment() tests. */
            prop = test.str_props[Uri_PROPERTY_FRAGMENT];
            hr = IUri_GetFragment(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetFragment returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetFragment returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetHost() tests. */
            prop = test.str_props[Uri_PROPERTY_HOST];
            hr = IUri_GetHost(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetHost returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetHost returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetPassword() tests. */
            prop = test.str_props[Uri_PROPERTY_PASSWORD];
            hr = IUri_GetPassword(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetPassword returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetPassword returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetPath() tests. */
            prop = test.str_props[Uri_PROPERTY_PATH];
            hr = IUri_GetPath(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetPath returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetPath returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetPathAndQuery() tests. */
            prop = test.str_props[Uri_PROPERTY_PATH_AND_QUERY];
            hr = IUri_GetPathAndQuery(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetPathAndQuery returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetPathAndQuery returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetQuery() tests. */
            prop = test.str_props[Uri_PROPERTY_QUERY];
            hr = IUri_GetQuery(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetQuery returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetQuery returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetRawUri() tests. */
            prop = test.str_props[Uri_PROPERTY_RAW_URI];
            hr = IUri_GetRawUri(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetRawUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetRawUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetSchemeName() tests. */
            prop = test.str_props[Uri_PROPERTY_SCHEME_NAME];
            hr = IUri_GetSchemeName(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetSchemeName returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetSchemeName returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetUserInfo() tests. */
            prop = test.str_props[Uri_PROPERTY_USER_INFO];
            hr = IUri_GetUserInfo(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetUserInfo returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetUserInfo returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
            received = NULL;

            /* GetUserName() tests. */
            prop = test.str_props[Uri_PROPERTY_USER_NAME];
            hr = IUri_GetUserName(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetUserName returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                            prop.value, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetUserName returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(!strcmp_aw(prop.value, received), "Error: Expected %s but got %s on uri_tests[%d].\n",
                        prop.value, wine_dbgstr_w(received), i);
            }
            SysFreeString(received);
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_GetDwordProperties(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    /* Make sure all the 'Get*' dword property functions handle invalid args correctly. */
    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        hr = IUri_GetHostType(uri, NULL);
        ok(hr == E_INVALIDARG, "Error: GetHostType returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

        hr = IUri_GetPort(uri, NULL);
        ok(hr == E_INVALIDARG, "Error: GetPort returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

        hr = IUri_GetScheme(uri, NULL);
        ok(hr == E_INVALIDARG, "Error: GetScheme returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

        hr = IUri_GetZone(uri, NULL);
        ok(hr == E_INVALIDARG, "Error: GetZone returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, test.create_expected, i);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                    hr, test.create_expected, i);
        }

        if(SUCCEEDED(hr)) {
            uri_dword_property prop;
            DWORD received;

            /* Assign an insane value so tests don't accidentally pass when
             * they shouldn't!
             */
            received = -9999999;

            /* GetHostType() tests. */
            prop = test.dword_props[Uri_PROPERTY_HOST_TYPE-Uri_PROPERTY_DWORD_START];
            hr = IUri_GetHostType(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetHostType returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetHostType returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
            }
            received = -9999999;

            /* GetPort() tests. */
            prop = test.dword_props[Uri_PROPERTY_PORT-Uri_PROPERTY_DWORD_START];
            hr = IUri_GetPort(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetPort returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetPort returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
            }
            received = -9999999;

            /* GetScheme() tests. */
            prop = test.dword_props[Uri_PROPERTY_SCHEME-Uri_PROPERTY_DWORD_START];
            hr = IUri_GetScheme(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetScheme returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetScheme returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
            }
            received = -9999999;

            /* GetZone() tests. */
            prop = test.dword_props[Uri_PROPERTY_ZONE-Uri_PROPERTY_DWORD_START];
            hr = IUri_GetZone(uri, &received);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.expected, "Error: GetZone returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                            hr, prop.expected, i);
                }
                todo_wine {
                    ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
                }
            } else {
                ok(hr == prop.expected, "Error: GetZone returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, prop.expected, i);
                ok(received == prop.value, "Error: Expected %d but got %d on uri_tests[%d].\n", prop.value, received, i);
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_GetPropertyLength(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    /* Make sure it handles invalid args correctly. */
    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        DWORD received = 0xdeadbeef;

        hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_STRING_START, NULL, 0);
        ok(hr == E_INVALIDARG, "Error: GetPropertyLength returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

        hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_DWORD_START, &received, 0);
        ok(hr == E_INVALIDARG, "Error: GetPropertyLength return 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
        ok(received == 0xdeadbeef, "Error: Expected 0xdeadbeef but got 0x%08x.\n", received);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_tests[%d].\n",
                        hr, test.create_expected, i);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_test[%d].\n",
                    hr, test.create_expected, i);
        }

        if(SUCCEEDED(hr)) {
            DWORD j;

            for(j = Uri_PROPERTY_STRING_START; j <= Uri_PROPERTY_STRING_LAST; ++j) {
                DWORD expectedLen, brokenLen, receivedLen;
                uri_str_property prop = test.str_props[j];

                expectedLen = lstrlen(prop.value);
                brokenLen = lstrlen(prop.broken_value);

                /* This won't be necessary once GetPropertyLength is implemented. */
                receivedLen = -1;

                hr = IUri_GetPropertyLength(uri, j, &receivedLen, 0);
                if(prop.todo) {
                    todo_wine {
                        ok(hr == prop.expected, "Error: GetPropertyLength returned 0x%08x, expected 0x%08x on uri_tests[%d].str_props[%d].\n",
                                hr, prop.expected, i, j);
                    }
                    todo_wine {
                        ok(receivedLen == expectedLen || broken(receivedLen == brokenLen),
                                "Error: Expected a length of %d but got %d on uri_tests[%d].str_props[%d].\n",
                                expectedLen, receivedLen, i, j);
                    }
                } else {
                    ok(hr == prop.expected, "Error: GetPropertyLength returned 0x%08x, expected 0x%08x on uri_tests[%d].str_props[%d].\n",
                            hr, prop.expected, i, j);
                    ok(receivedLen == expectedLen || broken(receivedLen == brokenLen),
                            "Error: Expected a length of %d but got %d on uri_tests[%d].str_props[%d].\n",
                            expectedLen, receivedLen, i, j);
                }
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static DWORD compute_expected_props(uri_properties *test)
{
    DWORD ret = 0, i;

    for(i=Uri_PROPERTY_STRING_START; i <= Uri_PROPERTY_STRING_LAST; i++) {
        if(test->str_props[i-Uri_PROPERTY_STRING_START].expected == S_OK)
            ret |= 1<<i;
    }

    for(i=Uri_PROPERTY_DWORD_START; i <= Uri_PROPERTY_DWORD_LAST; i++) {
        if(test->dword_props[i-Uri_PROPERTY_DWORD_START].expected == S_OK)
            ret |= 1<<i;
    }

    return ret;
}

static void test_IUri_GetProperties(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        hr = IUri_GetProperties(uri, NULL);
        ok(hr == E_INVALIDARG, "Error: GetProperties returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, test.create_expected);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, test.create_expected);
        }

        if(SUCCEEDED(hr)) {
            DWORD received = 0, expected_props;
            DWORD j;

            hr = IUri_GetProperties(uri, &received);
            ok(hr == S_OK, "Error: GetProperties returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            expected_props = compute_expected_props(&test);

            for(j = 0; j <= Uri_PROPERTY_DWORD_LAST; ++j) {
                /* (1 << j) converts a Uri_PROPERTY to its corresponding Uri_HAS_* flag mask. */
                if(expected_props & (1 << j))
                    ok(received & (1 << j), "Error: Expected flag for property %d on uri_tests[%d].\n", j, i);
                else
                    ok(!(received & (1 << j)), "Error: Received flag for property %d when not expected on uri_tests[%d].\n", j, i);
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_HasProperty(void) {
    IUri *uri = NULL;
    HRESULT hr;
    DWORD i;

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        hr = IUri_HasProperty(uri, Uri_PROPERTY_RAW_URI, NULL);
        ok(hr == E_INVALIDARG, "Error: HasProperty returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    }
    if(uri) IUri_Release(uri);

    for(i = 0; i < sizeof(uri_tests)/sizeof(uri_tests[0]); ++i) {
        uri_properties test = uri_tests[i];
        LPWSTR uriW;
        uri = NULL;

        uriW = a2w(test.uri);

        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(test.create_todo) {
            todo_wine {
                ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, test.create_expected);
            }
        } else {
            ok(hr == test.create_expected, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, test.create_expected);
        }

        if(SUCCEEDED(hr)) {
            DWORD expected_props, j;

            expected_props = compute_expected_props(&test);

            for(j = 0; j <= Uri_PROPERTY_DWORD_LAST; ++j) {
                /* Assign -1, then explicitly test for TRUE or FALSE later. */
                BOOL received = -1;

                hr = IUri_HasProperty(uri, j, &received);
                ok(hr == S_OK, "Error: HasProperty returned 0x%08x, expected 0x%08x for property %d on uri_tests[%d].\n",
                        hr, S_OK, j, i);

                if(expected_props & (1 << j)) {
                    ok(received == TRUE, "Error: Expected to have property %d on uri_tests[%d].\n", j, i);
                } else {
                    ok(received == FALSE, "Error: Wasn't expecting to have property %d on uri_tests[%d].\n", j, i);
                }
            }
        }

        if(uri) IUri_Release(uri);

        heap_free(uriW);
    }
}

static void test_IUri_IsEqual(void) {
    IUri *uriA, *uriB;
    HRESULT hrA, hrB;
    DWORD i;

    uriA = uriB = NULL;

    /* Make sure IsEqual handles invalid args correctly. */
    hrA = pCreateUri(http_urlW, 0, 0, &uriA);
    hrB = pCreateUri(http_urlW, 0, 0, &uriB);
    ok(hrA == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hrA, S_OK);
    ok(hrB == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hrB, S_OK);
    if(SUCCEEDED(hrA) && SUCCEEDED(hrB)) {
        BOOL equal = -1;
        hrA = IUri_IsEqual(uriA, NULL, &equal);
        ok(hrA == S_OK, "Error: IsEqual returned 0x%08x, expected 0x%08x.\n", hrA, S_OK);
        ok(equal == FALSE, "Error: Expected equal to be FALSE, but was %d instead.\n", equal);

        hrA = IUri_IsEqual(uriA, uriB, NULL);
        ok(hrA == E_POINTER, "Error: IsEqual returned 0x%08x, expected 0x%08x.\n", hrA, E_POINTER);
    }
    if(uriA) IUri_Release(uriA);
    if(uriB) IUri_Release(uriB);

    for(i = 0; i < sizeof(equality_tests)/sizeof(equality_tests[0]); ++i) {
        uri_equality test = equality_tests[i];
        LPWSTR uriA_W, uriB_W;

        uriA = uriB = NULL;

        uriA_W = a2w(test.a);
        uriB_W = a2w(test.b);

        hrA = pCreateUri(uriA_W, test.create_flags_a, 0, &uriA);
        if(test.create_todo_a) {
            todo_wine {
                ok(hrA == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x on equality_tests[%d].a\n",
                        hrA, S_OK, i);
            }
        } else {
            ok(hrA == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x on equality_tests[%d].a\n",
                    hrA, S_OK, i);
        }

        hrB = pCreateUri(uriB_W, test.create_flags_b, 0, &uriB);
        if(test.create_todo_b) {
            todo_wine {
                ok(hrB == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x on equality_tests[%d].b\n",
                        hrB, S_OK, i);
            }
        } else {
            ok(hrB == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x on equality_tests[%d].b\n",
                    hrB, S_OK, i);
        }

        if(SUCCEEDED(hrA) && SUCCEEDED(hrB)) {
            BOOL equal = -1;

            hrA = IUri_IsEqual(uriA, uriB, &equal);
            if(test.todo) {
                todo_wine {
                    ok(hrA == S_OK, "Error: IsEqual returned 0x%08x, expected 0x%08x on equality_tests[%d].\n",
                            hrA, S_OK, i);
                }
                todo_wine {
                    ok(equal == test.equal, "Error: Expected the comparison to be %d on equality_tests[%d].\n", test.equal, i);
                }
            } else {
                ok(hrA == S_OK, "Error: IsEqual returned 0x%08x, expected 0x%08x on equality_tests[%d].\n", hrA, S_OK, i);
                ok(equal == test.equal, "Error: Expected the comparison to be %d on equality_tests[%d].\n", test.equal, i);
            }
        }
        if(uriA) IUri_Release(uriA);
        if(uriB) IUri_Release(uriB);

        heap_free(uriA_W);
        heap_free(uriB_W);
    }
}

static void test_CreateUriWithFragment_InvalidArgs(void) {
    HRESULT hr;
    IUri *uri = (void*) 0xdeadbeef;
    const WCHAR fragmentW[] = {'#','f','r','a','g','m','e','n','t',0};

    hr = pCreateUriWithFragment(NULL, fragmentW, 0, 0, &uri);
    ok(hr == E_INVALIDARG, "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    ok(uri == NULL, "Error: Expected uri to be NULL, but was %p instead.\n", uri);

    hr = pCreateUriWithFragment(http_urlW, fragmentW, 0, 0, NULL);
    ok(hr == E_INVALIDARG, "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

    /* Original URI can't already contain a fragment component. */
    uri = (void*) 0xdeadbeef;
    hr = pCreateUriWithFragment(http_url_fragW, fragmentW, 0, 0, &uri);
    ok(hr == E_INVALIDARG, "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
    ok(uri == NULL, "Error: Expected uri to be NULL, but was %p instead.\n", uri);
}

/* CreateUriWithFragment has the same invalid flag combinations as CreateUri. */
static void test_CreateUriWithFragment_InvalidFlags(void) {
    DWORD i;

    for(i = 0; i < sizeof(invalid_flag_tests)/sizeof(invalid_flag_tests[0]); ++i) {
        HRESULT hr;
        IUri *uri = (void*) 0xdeadbeef;

        hr = pCreateUriWithFragment(http_urlW, NULL, invalid_flag_tests[i].flags, 0, &uri);
        ok(hr == invalid_flag_tests[i].expected, "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x. flags=0x%08x.\n",
            hr, invalid_flag_tests[i].expected, invalid_flag_tests[i].flags);
        ok(uri == NULL, "Error: Expected uri to be NULL, but was %p instead.\n", uri);
    }
}

static void test_CreateUriWithFragment(void) {
    DWORD i;

    for(i = 0; i < sizeof(uri_fragment_tests)/sizeof(uri_fragment_tests[0]); ++i) {
        HRESULT hr;
        IUri *uri = NULL;
        LPWSTR uriW, fragW;
        uri_with_fragment test = uri_fragment_tests[i];

        uriW = a2w(test.uri);
        fragW = a2w(test.fragment);

        hr = pCreateUriWithFragment(uriW, fragW, test.create_flags, 0, &uri);
        if(test.expected_todo) {
            todo_wine {
                ok(hr == test.create_expected,
                    "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x on uri_fragment_tests[%d].\n",
                    hr, test.create_expected, i);
            }
        } else
            ok(hr == test.create_expected,
                "Error: CreateUriWithFragment returned 0x%08x, expected 0x%08x on uri_fragment_tests[%d].\n",
                hr, test.create_expected, i);

        if(SUCCEEDED(hr)) {
            BSTR received = NULL;

            hr = IUri_GetAbsoluteUri(uri, &received);
            if(test.expected_todo) {
                todo_wine {
                    ok(hr == S_OK,
                        "Error: GetAbsoluteUri returned 0x%08x, expected 0x%08x on uri_fragment_tests[%d].\n",
                        hr, S_OK, i);
                }
                todo_wine {
                    ok(!strcmp_aw(test.expected_uri, received),
                        "Error: Expected %s but got %s on uri_fragment_tests[%d].\n",
                        test.expected_uri, wine_dbgstr_w(received), i);
                }
            } else {
                ok(hr == S_OK, "Error: GetAbsoluteUri returned 0x%08x, expected 0x%08x on uri_fragment_tests[%d].\n",
                    hr, S_OK, i);
                ok(!strcmp_aw(test.expected_uri, received), "Error: Expected %s but got %s on uri_fragment_tests[%d].\n",
                    test.expected_uri, wine_dbgstr_w(received), i);
            }

            SysFreeString(received);
        }

        if(uri) IUri_Release(uri);
        heap_free(uriW);
        heap_free(fragW);
    }
}

static void test_CreateIUriBuilder(void) {
    HRESULT hr;
    IUriBuilder *builder = NULL;
    IUri *uri;

    hr = pCreateIUriBuilder(NULL, 0, 0, NULL);
    ok(hr == E_POINTER, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x\n",
        hr, E_POINTER);

    /* CreateIUriBuilder increases the ref count of the IUri it receives. */
    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ULONG cur_count, orig_count;

        orig_count = get_refcnt(uri);
        hr = pCreateIUriBuilder(uri, 0, 0, &builder);
        ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
        ok(builder != NULL, "Error: Expecting builder not to be NULL\n");

        cur_count = get_refcnt(uri);
        ok(cur_count == orig_count+1, "Error: Expected the ref count to be %u, but was %u instead.\n", orig_count+1, cur_count);

        if(builder) IUriBuilder_Release(builder);
        cur_count = get_refcnt(uri);
        ok(cur_count == orig_count, "Error: Expected the ref count to be %u, but was %u instead.\n", orig_count, cur_count);
    }
    if(uri) IUri_Release(uri);
}

static void test_IUriBuilder_CreateUri(IUriBuilder *builder, const uri_builder_test *test,
                                       DWORD test_index) {
    HRESULT hr;
    IUri *uri = NULL;

    hr = IUriBuilder_CreateUri(builder, test->uri_flags, 0, 0, &uri);
    if(test->uri_todo) {
        todo_wine {
            ok(hr == test->uri_hres,
                "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, test->uri_hres, test_index);
        }
    } else {
        ok(hr == test->uri_hres,
            "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, test->uri_hres, test_index);
    }

    if(SUCCEEDED(hr)) {
        DWORD i;

        for(i = 0; i < sizeof(test->expected_str_props)/sizeof(test->expected_str_props[0]); ++i) {
            uri_builder_str_property prop = test->expected_str_props[i];
            BSTR received = NULL;

            hr = IUri_GetPropertyBSTR(uri, i, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(!strcmp_aw(prop.expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                            prop.expected, wine_dbgstr_w(received), test_index, i);
                    }
                } else {
                    ok(!strcmp_aw(prop.expected, received),
                        "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                        prop.expected, wine_dbgstr_w(received), test_index, i);
                }
            }
            SysFreeString(received);
        }

        for(i = 0; i < sizeof(test->expected_dword_props)/sizeof(test->expected_dword_props[0]); ++i) {
            uri_builder_dword_property prop = test->expected_dword_props[i];
            DWORD received = -2;

            hr = IUri_GetPropertyDWORD(uri, i+Uri_PROPERTY_DWORD_START, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(received == prop.expected,
                            "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                            prop.expected, received, test_index, i);
                    }
                } else {
                    ok(received == prop.expected,
                        "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        prop.expected, received, test_index, i);
                }
            }
        }
    }
    if(uri) IUri_Release(uri);
}

static void test_IUriBuilder_CreateUriSimple(IUriBuilder *builder, const uri_builder_test *test,
                                       DWORD test_index) {
    HRESULT hr;
    IUri *uri = NULL;

    hr = IUriBuilder_CreateUriSimple(builder, test->uri_simple_encode_flags, 0, &uri);
    if(test->uri_simple_todo) {
        todo_wine {
            ok(hr == test->uri_simple_hres,
                "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, test->uri_simple_hres, test_index);
        }
    } else {
        ok(hr == test->uri_simple_hres,
            "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, test->uri_simple_hres, test_index);
    }

    if(SUCCEEDED(hr)) {
        DWORD i;

        for(i = 0; i < sizeof(test->expected_str_props)/sizeof(test->expected_str_props[0]); ++i) {
            uri_builder_str_property prop = test->expected_str_props[i];
            BSTR received = NULL;

            hr = IUri_GetPropertyBSTR(uri, i, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(!strcmp_aw(prop.expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                            prop.expected, wine_dbgstr_w(received), test_index, i);
                    }
                } else {
                    ok(!strcmp_aw(prop.expected, received),
                        "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                        prop.expected, wine_dbgstr_w(received), test_index, i);
                }
            }
            SysFreeString(received);
        }

        for(i = 0; i < sizeof(test->expected_dword_props)/sizeof(test->expected_dword_props[0]); ++i) {
            uri_builder_dword_property prop = test->expected_dword_props[i];
            DWORD received = -2;

            hr = IUri_GetPropertyDWORD(uri, i+Uri_PROPERTY_DWORD_START, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(received == prop.expected,
                            "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                            prop.expected, received, test_index, i);
                    }
                } else {
                    ok(received == prop.expected,
                        "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        prop.expected, received, test_index, i);
                }
            }
        }
    }
    if(uri) IUri_Release(uri);
}

static void test_IUriBuilder_CreateUriWithFlags(IUriBuilder *builder, const uri_builder_test *test,
                                                DWORD test_index) {
    HRESULT hr;
    IUri *uri = NULL;

    hr = IUriBuilder_CreateUriWithFlags(builder, test->uri_with_flags, test->uri_with_builder_flags,
                                        test->uri_with_encode_flags, 0, &uri);
    if(test->uri_with_todo) {
        todo_wine {
            ok(hr == test->uri_with_hres,
                "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, test->uri_with_hres, test_index);
        }
    } else {
        ok(hr == test->uri_with_hres,
            "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, test->uri_with_hres, test_index);
    }

    if(SUCCEEDED(hr)) {
        DWORD i;

        for(i = 0; i < sizeof(test->expected_str_props)/sizeof(test->expected_str_props[0]); ++i) {
            uri_builder_str_property prop = test->expected_str_props[i];
            BSTR received = NULL;

            hr = IUri_GetPropertyBSTR(uri, i, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_str_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(!strcmp_aw(prop.expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                            prop.expected, wine_dbgstr_w(received), test_index, i);
                    }
                } else {
                    ok(!strcmp_aw(prop.expected, received),
                        "Error: Expected %s but got %s instead on uri_builder_tests[%d].expected_str_props[%d].\n",
                        prop.expected, wine_dbgstr_w(received), test_index, i);
                }
            }
            SysFreeString(received);
        }

        for(i = 0; i < sizeof(test->expected_dword_props)/sizeof(test->expected_dword_props[0]); ++i) {
            uri_builder_dword_property prop = test->expected_dword_props[i];
            DWORD received = -2;

            hr = IUri_GetPropertyDWORD(uri, i+Uri_PROPERTY_DWORD_START, &received, 0);
            if(prop.todo) {
                todo_wine {
                    ok(hr == prop.result,
                        "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        hr, prop.result, test_index, i);
                }
            } else {
                ok(hr == prop.result,
                    "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].expected_dword_props[%d].\n",
                    hr, prop.result, test_index, i);
            }
            if(SUCCEEDED(hr)) {
                if(prop.todo) {
                    todo_wine {
                        ok(received == prop.expected,
                            "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                            prop.expected, received, test_index, i);
                    }
                } else {
                    ok(received == prop.expected,
                        "Error: Expected %d but got %d instead on uri_builder_tests[%d].expected_dword_props[%d].\n",
                        prop.expected, received, test_index, i);
                }
            }
        }
    }
    if(uri) IUri_Release(uri);
}

static void test_IUriBuilder_CreateInvalidArgs(void) {
    IUriBuilder *builder;
    HRESULT hr;

    hr = pCreateIUriBuilder(NULL, 0, 0, &builder);
    ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        IUri *test = NULL, *uri = (void*) 0xdeadbeef;

        /* Test what happens if the IUriBuilder doesn't have a IUri set. */
        hr = IUriBuilder_CreateUri(builder, 0, 0, 0, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, E_POINTER);

        uri = (void*) 0xdeadbeef;
        hr = IUriBuilder_CreateUri(builder, 0, Uri_HAS_USER_NAME, 0, &uri);
        ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, E_NOTIMPL);
        ok(uri == NULL, "Error: expected uri to be NULL, but was %p instead.\n", uri);

        hr = IUriBuilder_CreateUriSimple(builder, 0, 0, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);

        uri = (void*) 0xdeadbeef;
        hr = IUriBuilder_CreateUriSimple(builder, Uri_HAS_USER_NAME, 0, &uri);
        ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x.\n",
            hr, E_NOTIMPL);
        ok(!uri, "Error: Expected uri to NULL, but was %p instead.\n", uri);

        hr = IUriBuilder_CreateUriWithFlags(builder, 0, 0, 0, 0, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);

        uri = (void*) 0xdeadbeef;
        hr = IUriBuilder_CreateUriWithFlags(builder, 0, 0, Uri_HAS_USER_NAME, 0, &uri);
        ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n",
            hr, E_NOTIMPL);
        ok(!uri, "Error: Expected uri to be NULL, but was %p instead.\n", uri);

        hr = pCreateUri(http_urlW, 0, 0, &test);
        ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
        if(SUCCEEDED(hr)) {
            hr = IUriBuilder_SetIUri(builder, test);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            /* No longer returns E_NOTIMPL, since a IUri has been set and hasn't been modified. */
            uri = NULL;
            hr = IUriBuilder_CreateUri(builder, 0, Uri_HAS_USER_NAME, 0, &uri);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            ok(uri != NULL, "Error: The uri was NULL.\n");
            if(uri) IUri_Release(uri);

            uri = NULL;
            hr = IUriBuilder_CreateUriSimple(builder, Uri_HAS_USER_NAME, 0, &uri);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            ok(uri != NULL, "Error: uri was NULL.\n");
            if(uri) IUri_Release(uri);

            uri = NULL;
            hr = IUriBuilder_CreateUriWithFlags(builder, 0, 0, 0, 0, &uri);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            ok(uri != NULL, "Error: uri was NULL.\n");
            if(uri) IUri_Release(uri);

            hr = IUriBuilder_SetFragment(builder, NULL);
            ok(hr == S_OK, "Error: IUriBuilder_SetFragment returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            /* The IUriBuilder is changed, so it returns E_NOTIMPL again. */
            uri = (void*) 0xdeadbeef;
            hr = IUriBuilder_CreateUri(builder, 0, Uri_HAS_USER_NAME, 0, &uri);
            ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            ok(!uri, "Error: Expected uri to be NULL but was %p instead.\n", uri);

            uri = (void*) 0xdeadbeef;
            hr = IUriBuilder_CreateUriSimple(builder, Uri_HAS_USER_NAME, 0, &uri);
            ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            ok(!uri, "Error: Expected uri to be NULL, but was %p instead.\n", uri);

            uri = (void*) 0xdeadbeef;
            hr = IUriBuilder_CreateUriWithFlags(builder, 0, 0, Uri_HAS_USER_NAME, 0, &uri);
            ok(hr == E_NOTIMPL, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n",
                hr, E_NOTIMPL);
            ok(!uri, "Error: Expected uri to be NULL, but was %p instead.\n", uri);
        }
        if(test) IUri_Release(test);
    }
    if(builder) IUriBuilder_Release(builder);
}

/* Tests invalid args to the "Get*" functions. */
static void test_IUriBuilder_GetInvalidArgs(void) {
    IUriBuilder *builder = NULL;
    HRESULT hr;

    hr = pCreateIUriBuilder(NULL, 0, 0, &builder);
    ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        LPCWSTR received = (void*) 0xdeadbeef;
        DWORD len = -1, port = -1;
        BOOL set = -1;

        hr = IUriBuilder_GetFragment(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        hr = IUriBuilder_GetFragment(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        hr = IUriBuilder_GetFragment(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetHost(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetHost(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetHost(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetPassword(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetPassword(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetPassword(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetPath(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetPath(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetPath(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetPort(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        hr = IUriBuilder_GetPort(builder, NULL, &port);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!port, "Error: Expected port to be 0, but was %d instead.\n", port);
        hr = IUriBuilder_GetPort(builder, &set, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!set, "Error: Expected set to be FALSE, but was %d instead.\n", set);

        hr = IUriBuilder_GetQuery(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetQuery(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetQuery(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetSchemeName(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetSchemeName(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetSchemeName(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);

        hr = IUriBuilder_GetUserName(builder, NULL, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        received = (void*) 0xdeadbeef;
        hr = IUriBuilder_GetUserName(builder, NULL, &received);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!received, "Error: Expected received to be NULL, but was %p instead.\n", received);
        len = -1;
        hr = IUriBuilder_GetUserName(builder, &len, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);
        ok(!len, "Error: Expected len to be 0, but was %d instead.\n", len);
    }
    if(builder) IUriBuilder_Release(builder);
}

static void test_IUriBuilder_GetFragment(IUriBuilder *builder, const uri_builder_test *test,
                                         DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_FRAGMENT)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetFragment(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetFragment(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetFragment(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetFragment to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetFragment(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetFragment returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetHost(IUriBuilder *builder, const uri_builder_test *test,
                                     DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_HOST)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetHost(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetHost(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetHost(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetHost to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetHost(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetPassword(IUriBuilder *builder, const uri_builder_test *test,
                                         DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_PASSWORD)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetPassword(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetPassword(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetPassword(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetPassword to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetPassword(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetPassword returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetPath(IUriBuilder *builder, const uri_builder_test *test,
                                     DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_PATH)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetPath(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetPath(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetPath(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetPath to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetPath(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetPath returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetPort(IUriBuilder *builder, const uri_builder_test *test,
                                     DWORD test_index) {
    HRESULT hr;
    BOOL has_port = FALSE;
    DWORD received = -1;

    if(test->port_prop.change) {
        DWORD expected = test->port_prop.value;

        hr = IUriBuilder_GetPort(builder, &has_port, &received);
        if(test->port_prop.todo) {
            todo_wine {
                ok(hr == S_OK,
                    "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_OK, test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(has_port == test->port_prop.set,
                        "Error: Expected has_port to be %d but was %d instead on uri_builder_tests[%d].\n",
                        test->port_prop.set, has_port, test_index);
                }
                todo_wine {
                    ok(received == expected,
                        "Error: Expected received to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        expected, received, test_index);
                }
            }
        } else {
            ok(hr == S_OK,
                "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, S_OK, test_index);
            ok(has_port == test->port_prop.set,
                "Error: Expected has_port to be %d, but was %d instead on uri_builder_tests[%d].\n",
                test->port_prop.set, has_port, test_index);
            ok(received == test->port_prop.value,
                "Error: Expected port to be %d, but was %d instead on uri_builder_tests[%d].\n",
                test->port_prop.value, received, test_index);
        }
    } else {
        IUri *uri = NULL;

        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                hr = IUriBuilder_GetPort(builder, &has_port, &received);
                ok(hr == S_OK,
                    "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_OK, test_index);
                if(SUCCEEDED(hr)) {
                    ok(has_port == FALSE,
                        "Error: Expected has_port to be FALSE, but was %d instead on uri_builder_tests[%d].\n",
                        has_port, test_index);
                    ok(!received, "Error: Expected received to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                DWORD expected;

                hr = IUri_GetPort(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_Port to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);

                hr = IUriBuilder_GetPort(builder, &has_port, &received);
                ok(hr == S_OK,
                    "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_OK, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!has_port,
                        "Error: Expected has_port to be FALSE but was TRUE instead on uri_builder_tests[%d].\n",
                        test_index);
                    ok(received == expected,
                        "Error: Expected received to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        expected, received, test_index);
                }
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetQuery(IUriBuilder *builder, const uri_builder_test *test,
                                      DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_QUERY)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetQuery(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetQuery(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetQuery(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetQuery to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetQuery(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetQuery returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetSchemeName(IUriBuilder *builder, const uri_builder_test *test,
                                           DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_SCHEME_NAME)
            prop = &(test->properties[i]);
    }

    if(prop) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetSchemeName(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetSchemeName(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BOOL has_prop = FALSE;
                BSTR expected = NULL;

                hr = IUri_GetSchemeName(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetSchemeName to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetSchemeName(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetSchemeName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

static void test_IUriBuilder_GetUserName(IUriBuilder *builder, const uri_builder_test *test,
                                         DWORD test_index) {
    HRESULT hr;
    DWORD i;
    LPCWSTR received = NULL;
    DWORD len = -1;
    const uri_builder_property *prop = NULL;

    /* Check if the property was set earlier. */
    for(i = 0; i < sizeof(test->properties)/sizeof(test->properties[0]); ++i) {
        if(test->properties[i].change && test->properties[i].property == Uri_PROPERTY_USER_NAME)
            prop = &(test->properties[i]);
    }

    if(prop && prop->value && *prop->value) {
        /* Use expected_value unless it's NULL, then use value. */
        LPCSTR expected = prop->expected_value ? prop->expected_value : prop->value;
        hr = IUriBuilder_GetUserName(builder, &len, &received);
        if(prop->todo) {
            todo_wine {
                ok(hr == (expected ? S_OK : S_FALSE),
                    "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, (expected ? S_OK : S_FALSE), test_index);
            }
            if(SUCCEEDED(hr)) {
                todo_wine {
                    ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                        expected, wine_dbgstr_w(received), test_index);
                }
                todo_wine {
                    ok(lstrlen(expected) == len,
                        "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                        lstrlen(expected), len, test_index);
                }
            }
        } else {
            ok(hr == (expected ? S_OK : S_FALSE),
                "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, (expected ? S_OK : S_FALSE), test_index);
            ok(!strcmp_aw(expected, received), "Error: Expected %s but got %s on uri_builder_tests[%d].\n",
                expected, wine_dbgstr_w(received), test_index);
            ok(lstrlen(expected) == len,
                "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                lstrlen(expected), len, test_index);
        }
    } else {
        /* The property wasn't set earlier, so it should return whatever
         * the base IUri contains (if anything).
         */
        IUri *uri = NULL;
        hr = IUriBuilder_GetIUri(builder, &uri);
        ok(hr == S_OK,
            "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
            hr, S_OK, test_index);
        if(SUCCEEDED(hr)) {
            if(!uri) {
                received = (void*) 0xdeadbeef;
                len = -1;

                hr = IUriBuilder_GetUserName(builder, &len, &received);
                ok(hr == S_FALSE,
                    "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, S_FALSE, test_index);
                if(SUCCEEDED(hr)) {
                    ok(!len, "Error: Expected len to be 0, but was %d instead on uri_builder_tests[%d].\n",
                        len, test_index);
                    ok(!received, "Error: Expected received to be NULL, but was %p instead on uri_builder_tests[%d].\n",
                        received, test_index);
                }
            } else {
                BSTR expected = NULL;
                BOOL has_prop = FALSE;

                hr = IUri_GetUserName(uri, &expected);
                ok(SUCCEEDED(hr),
                    "Error: Expected IUri_GetUserName to succeed, but got 0x%08x instead on uri_builder_tests[%d].\n",
                    hr, test_index);
                has_prop = hr == S_OK;

                hr = IUriBuilder_GetUserName(builder, &len, &received);
                if(has_prop) {
                    ok(hr == S_OK,
                        "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_OK, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!lstrcmpW(expected, received),
                            "Error: Expected %s but got %s instead on uri_builder_tests[%d].\n",
                            wine_dbgstr_w(expected), wine_dbgstr_w(received), test_index);
                        ok(lstrlenW(expected) == len,
                            "Error: Expected the length to be %d, but was %d instead on uri_builder_tests[%d].\n",
                            lstrlenW(expected), len, test_index);
                    }
                } else {
                    ok(hr == S_FALSE,
                        "Error: IUriBuilder_GetUserName returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, S_FALSE, test_index);
                    if(SUCCEEDED(hr)) {
                        ok(!received, "Error: Expected received to be NULL on uri_builder_tests[%d].\n", test_index);
                        ok(!len, "Error: Expected the length to be 0, but was %d instead on uri_builder_tests[%d].\n",
                            len, test_index);
                    }
                }
                SysFreeString(expected);
            }
        }
        if(uri) IUri_Release(uri);
    }
}

/* Tests IUriBuilder functions. */
static void test_IUriBuilder(void) {
    HRESULT hr;
    IUriBuilder *builder;
    DWORD i;

    for(i = 0; i < sizeof(uri_builder_tests)/sizeof(uri_builder_tests[0]); ++i) {
        IUri *uri = NULL;
        uri_builder_test test = uri_builder_tests[i];
        LPWSTR uriW = NULL;

        if(test.uri) {
            uriW = a2w(test.uri);
            hr = pCreateUri(uriW, test.create_flags, 0, &uri);
            ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, S_OK, i);
            if(FAILED(hr)) continue;
        }
        hr = pCreateIUriBuilder(uri, 0, 0, &builder);
        if(test.create_builder_todo) {
            todo_wine {
                ok(hr == test.create_builder_expected,
                    "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                    hr, test.create_builder_expected, i);
            }
        } else {
            ok(hr == test.create_builder_expected,
                "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, test.create_builder_expected, i);
        }
        if(SUCCEEDED(hr)) {
            DWORD j;
            BOOL modified = FALSE, received = FALSE;

            /* Perform all the string property changes. */
            for(j = 0; j < URI_BUILDER_STR_PROPERTY_COUNT; ++j) {
                uri_builder_property prop = test.properties[j];
                if(prop.change) {
                    change_property(builder, &prop, i);
                    if(prop.property != Uri_PROPERTY_SCHEME_NAME &&
                       prop.property != Uri_PROPERTY_HOST)
                        modified = TRUE;
                    else if(prop.value && *prop.value)
                        modified = TRUE;
                    else if(prop.value && !*prop.value && prop.property == Uri_PROPERTY_HOST)
                        /* Host name property can't be NULL, but it can be empty. */
                        modified = TRUE;
                }
            }

            if(test.port_prop.change) {
                hr = IUriBuilder_SetPort(builder, test.port_prop.set, test.port_prop.value);
                modified = TRUE;
                if(test.port_prop.todo) {
                    todo_wine {
                        ok(hr == test.port_prop.expected,
                            "Error: IUriBuilder_SetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                            hr, test.port_prop.expected, i);
                    }
                } else {
                    ok(hr == test.port_prop.expected,
                        "Error: IUriBuilder_SetPort returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                        hr, test.port_prop.expected, i);
                }
            }

            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK,
                "Error IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x on uri_builder_tests[%d].\n",
                hr, S_OK, i);
            if(SUCCEEDED(hr))
                ok(received == modified,
                    "Error: Expected received to be %d but was %d instead on uri_builder_tests[%d].\n",
                    modified, received, i);

            /* Test the "Get*" functions. */
            test_IUriBuilder_GetFragment(builder, &test, i);
            test_IUriBuilder_GetHost(builder, &test, i);
            test_IUriBuilder_GetPassword(builder, &test, i);
            test_IUriBuilder_GetPath(builder, &test, i);
            test_IUriBuilder_GetPort(builder, &test, i);
            test_IUriBuilder_GetQuery(builder, &test, i);
            test_IUriBuilder_GetSchemeName(builder, &test, i);
            test_IUriBuilder_GetUserName(builder, &test, i);

            test_IUriBuilder_CreateUri(builder, &test, i);
            test_IUriBuilder_CreateUriSimple(builder, &test, i);
            test_IUriBuilder_CreateUriWithFlags(builder, &test, i);
        }
        if(builder) IUriBuilder_Release(builder);
        if(uri) IUri_Release(uri);
        heap_free(uriW);
    }
}

static void test_IUriBuilder_HasBeenModified(void) {
    HRESULT hr;
    IUriBuilder *builder = NULL;

    hr = pCreateIUriBuilder(NULL, 0, 0, &builder);
    ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        static const WCHAR hostW[] = {'g','o','o','g','l','e','.','c','o','m',0};
        IUri *uri = NULL;
        BOOL received;

        hr = IUriBuilder_HasBeenModified(builder, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);

        hr = IUriBuilder_SetHost(builder, hostW);
        ok(hr == S_OK, "Error: IUriBuilder_SetHost returned 0x%08x, expected 0x%08x.\n",
            hr, S_OK);

        hr = IUriBuilder_HasBeenModified(builder, &received);
        ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
            hr, S_OK);
        if(SUCCEEDED(hr))
            ok(received == TRUE, "Error: Expected received to be TRUE.\n");

        hr = pCreateUri(http_urlW, 0, 0, &uri);
        ok(hr == S_OK, "Error: CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
        if(SUCCEEDED(hr)) {
            LPCWSTR prop;
            DWORD len = -1;

            hr = IUriBuilder_SetIUri(builder, uri);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);

            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr))
                ok(received == FALSE, "Error: Expected received to be FALSE.\n");

            /* Test what happens with you call SetIUri with the same IUri again. */
            hr = IUriBuilder_SetHost(builder, hostW);
            ok(hr == S_OK, "Error: IUriBuilder_SetHost returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr))
                ok(received == TRUE, "Error: Expected received to be TRUE.\n");

            hr = IUriBuilder_SetIUri(builder, uri);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            /* IUriBuilder already had 'uri' as it's IUri property and so Windows doesn't
             * reset any of the changes that were made to the IUriBuilder.
             */
            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr))
                ok(received == TRUE, "Error: Expected received to be TRUE.\n");

            hr = IUriBuilder_GetHost(builder, &len, &prop);
            ok(hr == S_OK, "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                ok(!lstrcmpW(prop, hostW), "Error: Expected %s but got %s instead.\n",
                    wine_dbgstr_w(hostW), wine_dbgstr_w(prop));
                ok(len == lstrlenW(hostW), "Error: Expected len to be %d, but was %d instead.\n",
                    lstrlenW(hostW), len);
            }

            hr = IUriBuilder_SetIUri(builder, NULL);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            hr = IUriBuilder_SetHost(builder, hostW);
            ok(hr == S_OK, "Error: IUriBuilder_SetHost returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr))
                ok(received == TRUE, "Error: Expected received to be TRUE.\n");

            hr = IUriBuilder_SetIUri(builder, NULL);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%09x.\n", hr, S_OK);

            hr = IUriBuilder_HasBeenModified(builder, &received);
            ok(hr == S_OK, "Error: IUriBuilder_HasBeenModified returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr))
                ok(received == TRUE, "Error: Expected received to be TRUE.\n");

            hr = IUriBuilder_GetHost(builder, &len, &prop);
            ok(hr == S_OK, "Error: IUriBuilder_GetHost returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                ok(!lstrcmpW(prop, hostW), "Error: Expected %s but got %s instead.\n",
                    wine_dbgstr_w(hostW), wine_dbgstr_w(prop));
                ok(len == lstrlenW(hostW), "Error: Expected len to %d, but was %d instead.\n",
                    lstrlenW(hostW), len);
            }
        }
        if(uri) IUri_Release(uri);
    }
    if(builder) IUriBuilder_Release(builder);
}

/* Test IUriBuilder {Get,Set}IUri functions. */
static void test_IUriBuilder_IUriProperty(void) {
    IUriBuilder *builder = NULL;
    HRESULT hr;

    hr = pCreateIUriBuilder(NULL, 0, 0, &builder);
    ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        IUri *uri = NULL;

        hr = IUriBuilder_GetIUri(builder, NULL);
        ok(hr == E_POINTER, "Error: IUriBuilder_GetIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);

        hr = pCreateUri(http_urlW, 0, 0, &uri);
        if(SUCCEEDED(hr)) {
            IUri *test = NULL;
            ULONG cur_count, orig_count;

            /* IUriBuilder doesn't clone the IUri, it use the same IUri. */
            orig_count = get_refcnt(uri);
            hr = IUriBuilder_SetIUri(builder, uri);
            cur_count = get_refcnt(uri);
            if(SUCCEEDED(hr))
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);

            hr = IUriBuilder_SetIUri(builder, NULL);
            cur_count = get_refcnt(uri);
            if(SUCCEEDED(hr))
                ok(cur_count == orig_count, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count, cur_count);

            /* CreateUri* functions will return back the same IUri if nothing has changed. */
            hr = IUriBuilder_SetIUri(builder, uri);
            ok(hr == S_OK, "Error: IUriBuilder_SetIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            orig_count = get_refcnt(uri);

            hr = IUriBuilder_CreateUri(builder, 0, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected test to be %p, but was %p instead.\n",
                    uri, test);
            }
            if(test) IUri_Release(test);

            test = NULL;
            hr = IUriBuilder_CreateUri(builder, -1, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected test to be %p, but was %p instead.\n", uri, test);
            }
            if(test) IUri_Release(test);

            /* Doesn't return the same IUri, if the flag combination is different then the one that created
             * the base IUri.
             */
            test = NULL;
            hr = IUriBuilder_CreateUri(builder, Uri_CREATE_ALLOW_RELATIVE, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr))
                ok(test != uri, "Error: Wasn't expecting 'test' to be 'uri'\n");

            if(test) IUri_Release(test);

            /* Still returns the same IUri, even though the base one wasn't created with CREATE_CANONICALIZE
             * explicitly set (because it's a default flags).
             */
            test = NULL;
            hr = IUriBuilder_CreateUri(builder, Uri_CREATE_CANONICALIZE, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected 'test' to be %p, but was %p instead.\n", uri, test);
            }
            if(test) IUri_Release(test);

            test = NULL;
            hr = IUriBuilder_CreateUriSimple(builder, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriSimple returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected test to be %p, but was %p instead.\n", uri, test);
            }
            if(test) IUri_Release(test);

            test = NULL;
            hr = IUriBuilder_CreateUriWithFlags(builder, 0, 0, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected test to be %p, but was %p instead.\n", uri, test);
            }
            if(test) IUri_Release(test);

            /* Doesn't return the same IUri, if the flag combination is different then the one that created
             * the base IUri.
             */
            test = NULL;
            hr = IUriBuilder_CreateUriWithFlags(builder, Uri_CREATE_ALLOW_RELATIVE, 0, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr))
                ok(test != uri, "Error: Wasn't expecting 'test' to be 'uri'\n");

            if(test) IUri_Release(test);

            /* Still returns the same IUri, even though the base one wasn't created with CREATE_CANONICALIZE
             * explicitly set (because it's a default flags).
             */
            test = NULL;
            hr = IUriBuilder_CreateUriWithFlags(builder, Uri_CREATE_CANONICALIZE, 0, 0, 0, &test);
            ok(hr == S_OK, "Error: IUriBuilder_CreateUriWithFlags returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                cur_count = get_refcnt(uri);
                ok(cur_count == orig_count+1, "Error: Expected uri ref count to be %d, but was %d instead.\n",
                    orig_count+1, cur_count);
                ok(test == uri, "Error: Expected 'test' to be %p, but was %p instead.\n", uri, test);
            }
            if(test) IUri_Release(test);
        }
        if(uri) IUri_Release(uri);
    }
    if(builder) IUriBuilder_Release(builder);
}

static void test_IUriBuilder_RemoveProperties(void) {
    IUriBuilder *builder = NULL;
    HRESULT hr;
    DWORD i;

    hr = pCreateIUriBuilder(NULL, 0, 0, &builder);
    ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        /* Properties that can't be removed. */
        const DWORD invalid = Uri_HAS_ABSOLUTE_URI|Uri_HAS_DISPLAY_URI|Uri_HAS_RAW_URI|Uri_HAS_HOST_TYPE|
                              Uri_HAS_SCHEME|Uri_HAS_ZONE;

        for(i = Uri_PROPERTY_STRING_START; i <= Uri_PROPERTY_DWORD_LAST; ++i) {
            hr = IUriBuilder_RemoveProperties(builder, i << 1);
            if((i << 1) & invalid) {
                ok(hr == E_INVALIDARG,
                    "Error: IUriBuilder_RemoveProperties returned 0x%08x, expected 0x%08x with prop=%d.\n",
                    hr, E_INVALIDARG, i);
            } else {
                ok(hr == S_OK,
                    "Error: IUriBuilder_RemoveProperties returned 0x%08x, expected 0x%08x with prop=%d.\n",
                    hr, S_OK, i);
            }
        }

        /* Also doesn't accept anything that's outside the range of the
         * Uri_HAS flags.
         */
        hr = IUriBuilder_RemoveProperties(builder, (Uri_PROPERTY_DWORD_LAST+1) << 1);
        ok(hr == E_INVALIDARG, "Error: IUriBuilder_RemoveProperties returned 0x%08x, expected 0x%08x.\n",
            hr, E_INVALIDARG);
    }
    if(builder) IUriBuilder_Release(builder);

    for(i = 0; i < sizeof(uri_builder_remove_tests)/sizeof(uri_builder_remove_tests[0]); ++i) {
        uri_builder_remove_test test = uri_builder_remove_tests[i];
        IUri *uri = NULL;
        LPWSTR uriW;

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.create_flags, 0, &uri);
        if(SUCCEEDED(hr)) {
            builder = NULL;

            hr = pCreateIUriBuilder(uri, 0, 0, &builder);
            if(test.create_builder_todo) {
                todo_wine {
                    ok(hr == test.create_builder_expected,
                        "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x on test %d.\n",
                        hr, test.create_builder_expected, i);
                }
            } else {
                ok(hr == test.create_builder_expected,
                    "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x on test %d.\n",
                    hr, test.create_builder_expected, i);
            }
            if(SUCCEEDED(hr)) {
                hr = IUriBuilder_RemoveProperties(builder, test.remove_properties);
                if(test.remove_todo) {
                    todo_wine {
                        ok(hr == test.remove_expected,
                            "Error: IUriBuilder_RemoveProperties returned 0x%08x, expected 0x%08x on test %d.\n",
                            hr, test.remove_expected, i);
                    }
                } else {
                    ok(hr == test.remove_expected,
                        "Error: IUriBuilder returned 0x%08x, expected 0x%08x on test %d.\n",
                        hr, test.remove_expected, i);
                }
                if(SUCCEEDED(hr)) {
                    IUri *result = NULL;

                    hr = IUriBuilder_CreateUri(builder, test.expected_flags, 0, 0, &result);
                    if(test.expected_todo) {
                        todo_wine {
                            ok(hr == test.expected_hres,
                                "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x on test %d.\n",
                                hr, test.expected_hres, i);
                        }
                    } else {
                        ok(hr == test.expected_hres,
                            "Error: IUriBuilder_CreateUri returned 0x%08x, expected 0x%08x on test %d.\n",
                            hr, test.expected_hres, i);
                    }
                    if(SUCCEEDED(hr)) {
                        BSTR received = NULL;

                        hr = IUri_GetAbsoluteUri(result, &received);
                        ok(hr == S_OK, "Error: Expected S_OK, but got 0x%08x instead.\n", hr);
                        ok(!strcmp_aw(test.expected_uri, received),
                            "Error: Expected %s but got %s instead on test %d.\n",
                            test.expected_uri, wine_dbgstr_w(received), i);
                        SysFreeString(received);
                    }
                    if(result) IUri_Release(result);
                }
            }
            if(builder) IUriBuilder_Release(builder);
        }
        if(uri) IUri_Release(uri);
        heap_free(uriW);
    }
}

static void test_IUriBuilder_Misc(void) {
    HRESULT hr;
    IUri *uri;

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    if(SUCCEEDED(hr)) {
        IUriBuilder *builder;

        hr = pCreateIUriBuilder(uri, 0, 0, &builder);
        ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
        if(SUCCEEDED(hr)) {
            BOOL has = -1;
            DWORD port = -1;

            hr = IUriBuilder_GetPort(builder, &has, &port);
            ok(hr == S_OK, "Error: IUriBuilder_GetPort returned 0x%08x, expected 0x%08x.\n", hr, S_OK);
            if(SUCCEEDED(hr)) {
                /* 'has' will be set to FALSE, even though uri had a port. */
                ok(has == FALSE, "Error: Expected 'has' to be FALSE, was %d instead.\n", has);
                /* Still sets 'port' to 80. */
                ok(port == 80, "Error: Expected the port to be 80, but, was %d instead.\n", port);
            }
        }
        if(builder) IUriBuilder_Release(builder);
    }
    if(uri) IUri_Release(uri);
}

static void test_IUriBuilderFactory(void) {
    HRESULT hr;
    IUri *uri;
    IUriBuilderFactory *factory;
    IUriBuilder *builder;

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        factory = NULL;
        hr = IUri_QueryInterface(uri, &IID_IUriBuilderFactory, (void**)&factory);
        ok(hr == S_OK, "Error: Expected S_OK, but got 0x%08x.\n", hr);
        ok(factory != NULL, "Error: Expected 'factory' to not be NULL.\n");

        if(SUCCEEDED(hr)) {
            builder = (void*) 0xdeadbeef;
            hr = IUriBuilderFactory_CreateIUriBuilder(factory, 10, 0, &builder);
            ok(hr == E_INVALIDARG, "Error: CreateInitializedIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_INVALIDARG);
            ok(!builder, "Error: Expected 'builder' to be NULL, but was %p.\n", builder);

            builder = (void*) 0xdeadbeef;
            hr = IUriBuilderFactory_CreateIUriBuilder(factory, 0, 10, &builder);
            ok(hr == E_INVALIDARG, "Error: CreateInitializedIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_INVALIDARG);
            ok(!builder, "Error: Expected 'builder' to be NULL, but was %p.\n", builder);

            hr = IUriBuilderFactory_CreateIUriBuilder(factory, 0, 0, NULL);
            ok(hr == E_POINTER, "Error: CreateInitializedIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_POINTER);

            builder = NULL;
            hr = IUriBuilderFactory_CreateIUriBuilder(factory, 0, 0, &builder);
            ok(hr == S_OK, "Error: CreateInitializedIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr)) {
                IUri *tmp = (void*) 0xdeadbeef;
                LPCWSTR result;
                DWORD result_len;

                hr = IUriBuilder_GetIUri(builder, &tmp);
                ok(hr == S_OK, "Error: GetIUri returned 0x%08x, expected 0x%08x.\n",
                    hr, S_OK);
                ok(!tmp, "Error: Expected 'tmp' to be NULL, but was %p instead.\n", tmp);

                hr = IUriBuilder_GetHost(builder, &result_len, &result);
                ok(hr == S_FALSE, "Error: GetHost returned 0x%08x, expected 0x%08x.\n",
                    hr, S_FALSE);
            }
            if(builder) IUriBuilder_Release(builder);

            builder = (void*) 0xdeadbeef;
            hr = IUriBuilderFactory_CreateInitializedIUriBuilder(factory, 10, 0, &builder);
            ok(hr == E_INVALIDARG, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_INVALIDARG);
            ok(!builder, "Error: Expected 'builder' to be NULL, but was %p.\n", builder);

            builder = (void*) 0xdeadbeef;
            hr = IUriBuilderFactory_CreateInitializedIUriBuilder(factory, 0, 10, &builder);
            ok(hr == E_INVALIDARG, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_INVALIDARG);
            ok(!builder, "Error: Expected 'builder' to be NULL, but was %p.\n", builder);

            hr = IUriBuilderFactory_CreateInitializedIUriBuilder(factory, 0, 0, NULL);
            ok(hr == E_POINTER, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, E_POINTER);

            builder = NULL;
            hr = IUriBuilderFactory_CreateInitializedIUriBuilder(factory, 0, 0, &builder);
            ok(hr == S_OK, "Error: CreateIUriBuilder returned 0x%08x, expected 0x%08x.\n",
                hr, S_OK);
            if(SUCCEEDED(hr)) {
                IUri *tmp = NULL;

                hr = IUriBuilder_GetIUri(builder, &tmp);
                ok(hr == S_OK, "Error: GetIUri return 0x%08x, expected 0x%08x.\n",
                    hr, S_OK);
                ok(tmp == uri, "Error: Expected tmp to be %p, but was %p.\n", uri, tmp);
                if(uri) IUri_Release(uri);
            }
            if(builder) IUriBuilder_Release(builder);
        }
        if(factory) IUriBuilderFactory_Release(factory);
    }
    if(uri) IUri_Release(uri);
}

static void test_CoInternetCombineIUri(void) {
    HRESULT hr;
    IUri *base, *relative, *result;
    DWORD i;

    base = NULL;
    hr = pCreateUri(http_urlW, 0, 0, &base);
    ok(SUCCEEDED(hr), "Error: Expected CreateUri to succeed, got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        result = (void*) 0xdeadbeef;
        hr = pCoInternetCombineIUri(base, NULL, 0, &result, 0);
        ok(hr == E_INVALIDARG, "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
        ok(!result, "Error: Expected 'result' to be NULL, was %p.\n", result);
    }

    relative = NULL;
    hr = pCreateUri(http_urlW, 0, 0, &relative);
    ok(SUCCEEDED(hr), "Error: Expected CreateUri to succeed, got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        result = (void*) 0xdeadbeef;
        hr = pCoInternetCombineIUri(NULL, relative, 0, &result, 0);
        ok(hr == E_INVALIDARG, "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);
        ok(!result, "Error: Expected 'result' to be NULL, was %p.\n", result);
    }

    hr = pCoInternetCombineIUri(base, relative, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x.\n", hr, E_INVALIDARG);

    if(base) IUri_Release(base);
    if(relative) IUri_Release(relative);

    for(i = 0; i < sizeof(uri_combine_tests)/sizeof(uri_combine_tests[0]); ++i) {
        LPWSTR baseW = a2w(uri_combine_tests[i].base_uri);

        hr = pCreateUri(baseW, uri_combine_tests[i].base_create_flags, 0, &base);
        ok(SUCCEEDED(hr), "Error: Expected CreateUri to succeed, got 0x%08x on uri_combine_tests[%d].\n", hr, i);
        if(SUCCEEDED(hr)) {
            LPWSTR relativeW = a2w(uri_combine_tests[i].relative_uri);

            hr = pCreateUri(relativeW, uri_combine_tests[i].relative_create_flags, 0, &relative);
            ok(SUCCEEDED(hr), "Error: Expected CreateUri to succeed, got 0x%08x on uri_combine_tests[%d].\n", hr, i);
            if(SUCCEEDED(hr)) {
                result = NULL;

                hr = pCoInternetCombineIUri(base, relative, uri_combine_tests[i].combine_flags, &result, 0);
                if(uri_combine_tests[i].todo) {
                    todo_wine {
                        ok(hr == uri_combine_tests[i].expected,
                            "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].\n",
                            hr, uri_combine_tests[i].expected, i);
                    }
                } else {
                    ok(hr == uri_combine_tests[i].expected,
                        "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].\n",
                        hr, uri_combine_tests[i]. expected, i);
                }
                if(SUCCEEDED(hr)) {
                    DWORD j;

                    for(j = 0; j < sizeof(uri_combine_tests[i].str_props)/sizeof(uri_combine_tests[i].str_props[0]); ++j) {
                        uri_combine_str_property prop = uri_combine_tests[i].str_props[j];
                        BSTR received;

                        hr = IUri_GetPropertyBSTR(result, j, &received, 0);
                        if(prop.todo) {
                            todo_wine {
                                ok(hr == prop.expected,
                                    "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].str_props[%d].\n",
                                    hr, prop.expected, i, j);
                            }
                            todo_wine {
                                ok(!strcmp_aw(prop.value, received) ||
                                   broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                                    "Error: Expected %s but got %s instead on uri_combine_tests[%d].str_props[%d].\n",
                                    prop.value, wine_dbgstr_w(received), i, j);
                            }
                        } else {
                            ok(hr == prop.expected,
                                "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].str_props[%d].\n",
                                hr, prop.expected, i, j);
                            ok(!strcmp_aw(prop.value, received) ||
                               broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                                "Error: Expected %s but got %s instead on uri_combine_tests[%d].str_props[%d].\n",
                                prop.value, wine_dbgstr_w(received), i, j);
                        }
                        SysFreeString(received);
                    }

                    for(j = 0; j < sizeof(uri_combine_tests[i].dword_props)/sizeof(uri_combine_tests[i].dword_props[0]); ++j) {
                        uri_dword_property prop = uri_combine_tests[i].dword_props[j];
                        DWORD received;

                        hr = IUri_GetPropertyDWORD(result, j+Uri_PROPERTY_DWORD_START, &received, 0);
                        if(prop.todo) {
                            todo_wine {
                                ok(hr == prop.expected,
                                    "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].dword_props[%d].\n",
                                    hr, prop.expected, i, j);
                            }
                            todo_wine {
                                ok(prop.value == received, "Error: Expected %d, but got %d instead on uri_combine_tests[%d].dword_props[%d].\n",
                                    prop.value, received, i, j);
                            }
                        } else {
                            ok(hr == prop.expected,
                                "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].dword_props[%d].\n",
                                hr, prop.expected, i, j);
                            ok(prop.value == received, "Error: Expected %d, but got %d instead on uri_combine_tests[%d].dword_props[%d].\n",
                                prop.value, received, i, j);
                        }
                    }
                }
                if(result) IUri_Release(result);
            }
            if(relative) IUri_Release(relative);
            heap_free(relativeW);
        }
        if(base) IUri_Release(base);
        heap_free(baseW);
    }
}

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface,
                                                          REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    return 1;
}

static HRESULT WINAPI InternetProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD *pcchResult, DWORD dwReserved)
{
    CHECK_EXPECT(ParseUrl);
    ok(!lstrcmpW(pwzUrl, parse_urlW), "Error: Expected %s, but got %s instead.\n",
        wine_dbgstr_w(parse_urlW), wine_dbgstr_w(pwzUrl));
    ok(ParseAction == parse_action, "Error: Expected %d, but got %d.\n", parse_action, ParseAction);
    ok(dwParseFlags == parse_flags, "Error: Expected 0x%08x, but got 0x%08x.\n", parse_flags, dwParseFlags);
    ok(cchResult == 200, "Error: Got %d.\n", cchResult);

    memcpy(pwzResult, parse_resultW, sizeof(parse_resultW));
    *pcchResult = lstrlenW(parse_resultW);

    return S_OK;
}

static HRESULT WINAPI InternetProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
        LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    CHECK_EXPECT(CombineUrl);
    ok(!lstrcmpW(pwzBaseUrl, combine_baseW), "Error: Expected %s, but got %s instead.\n",
        wine_dbgstr_w(combine_baseW), wine_dbgstr_w(pwzBaseUrl));
    ok(!lstrcmpW(pwzRelativeUrl, combine_relativeW), "Error: Expected %s, but got %s instead.\n",
        wine_dbgstr_w(combine_relativeW), wine_dbgstr_w(pwzRelativeUrl));
    ok(dwCombineFlags == (URL_DONT_SIMPLIFY|URL_FILE_USE_PATHURL|URL_DONT_UNESCAPE_EXTRA_INFO),
        "Error: Expected 0, but got 0x%08x.\n", dwCombineFlags);
    ok(cchResult == INTERNET_MAX_URL_LENGTH+1, "Error: Got %d.\n", cchResult);

    memcpy(pwzResult, combine_resultW, sizeof(combine_resultW));
    *pcchResult = lstrlenW(combine_resultW);

    return S_OK;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_QueryInfo(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer,
        DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl InternetProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    InternetProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    InternetProtocolInfo_QueryInfo
};

static IInternetProtocolInfo protocol_info = { &InternetProtocolInfoVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        *ppv = &protocol_info;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory protocol_cf = { &ClassFactoryVtbl };

static void register_protocols(void)
{
    IInternetSession *session;
    HRESULT hres;

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterNameSpace(session, &protocol_cf, &IID_NULL,
            winetestW, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    IInternetSession_Release(session);
}

static void unregister_protocols(void) {
    IInternetSession *session;
    HRESULT hr;

    hr = pCoInternetGetSession(0, &session, 0);
    ok(hr == S_OK, "CoInternetGetSession failed: 0x%08x\n", hr);
    if(FAILED(hr))
        return;

    hr = IInternetSession_UnregisterNameSpace(session, &protocol_cf, winetestW);
    ok(hr == S_OK, "UnregisterNameSpace failed: 0x%08x\n", hr);

    IInternetSession_Release(session);
}

static void test_CoInternetCombineIUri_Pluggable(void) {
    HRESULT hr;
    IUri *base = NULL;

    hr = pCreateUri(combine_baseW, 0, 0, &base);
    ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        IUri *relative = NULL;

        hr = pCreateUri(combine_relativeW, Uri_CREATE_ALLOW_RELATIVE, 0, &relative);
        ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) {
            IUri *result = NULL;

            SET_EXPECT(CombineUrl);

            hr = pCoInternetCombineIUri(base, relative, URL_DONT_SIMPLIFY|URL_FILE_USE_PATHURL|URL_DONT_UNESCAPE_EXTRA_INFO,
                                        &result, 0);
            ok(hr == S_OK, "Error: CoInternetCombineIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

            CHECK_CALLED(CombineUrl);

            if(SUCCEEDED(hr)) {
                BSTR received = NULL;
                hr = IUri_GetAbsoluteUri(result, &received);
                ok(hr == S_OK, "Error: Expected S_OK, but got 0x%08x instead.\n", hr);
                if(SUCCEEDED(hr)) {
                    ok(!lstrcmpW(combine_resultW, received), "Error: Expected %s, but got %s.\n",
                        wine_dbgstr_w(combine_resultW), wine_dbgstr_w(received));
                }
                SysFreeString(received);
            }
            if(result) IUri_Release(result);
        }
        if(relative) IUri_Release(relative);
    }
    if(base) IUri_Release(base);
}

static void test_CoInternetCombineUrlEx(void) {
    HRESULT hr;
    IUri *base, *result;
    DWORD i;

    base = NULL;
    hr = pCreateUri(http_urlW, 0, 0, &base);
    ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        result = (void*) 0xdeadbeef;
        hr = pCoInternetCombineUrlEx(base, NULL, 0, &result, 0);
        ok(hr == E_UNEXPECTED, "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x.\n",
            hr, E_UNEXPECTED);
        ok(!result, "Error: Expected 'result' to be NULL was %p instead.\n", result);
    }

    result = (void*) 0xdeadbeef;
    hr = pCoInternetCombineUrlEx(NULL, http_urlW, 0, &result, 0);
    ok(hr == E_INVALIDARG, "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x.\n",
        hr, E_INVALIDARG);
    ok(!result, "Error: Expected 'result' to be NULL, but was %p instead.\n", result);

    result = (void*) 0xdeadbeef;
    hr = pCoInternetCombineUrlEx(NULL, NULL, 0, &result, 0);
    ok(hr == E_UNEXPECTED, "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x.\n",
        hr, E_UNEXPECTED);
    ok(!result, "Error: Expected 'result' to be NULL, but was %p instead.\n", result);

    hr = pCoInternetCombineUrlEx(base, http_urlW, 0, NULL, 0);
    ok(hr == E_POINTER, "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x.\n",
        hr, E_POINTER);
    if(base) IUri_Release(base);

    for(i = 0; i < sizeof(uri_combine_tests)/sizeof(uri_combine_tests[0]); ++i) {
        LPWSTR baseW = a2w(uri_combine_tests[i].base_uri);

        hr = pCreateUri(baseW, uri_combine_tests[i].base_create_flags, 0, &base);
        ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x on uri_combine_tests[%d].\n", hr, i);
        if(SUCCEEDED(hr)) {
            LPWSTR relativeW = a2w(uri_combine_tests[i].relative_uri);

            hr = pCoInternetCombineUrlEx(base, relativeW, uri_combine_tests[i].combine_flags,
                                         &result, 0);
            if(uri_combine_tests[i].todo) {
                todo_wine {
                    ok(hr == uri_combine_tests[i].expected,
                        "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].\n",
                        hr, uri_combine_tests[i].expected, i);
                }
            } else {
                ok(hr == uri_combine_tests[i].expected,
                    "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].\n",
                    hr, uri_combine_tests[i]. expected, i);
            }
            if(SUCCEEDED(hr)) {
                DWORD j;

                for(j = 0; j < sizeof(uri_combine_tests[i].str_props)/sizeof(uri_combine_tests[i].str_props[0]); ++j) {
                    uri_combine_str_property prop = uri_combine_tests[i].str_props[j];
                    BSTR received;
                    LPCSTR value = (prop.value_ex) ? prop.value_ex : prop.value;

                    hr = IUri_GetPropertyBSTR(result, j, &received, 0);
                    if(prop.todo) {
                        todo_wine {
                            ok(hr == prop.expected,
                                "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].str_props[%d].\n",
                                hr, prop.expected, i, j);
                        }
                        todo_wine {
                            ok(!strcmp_aw(value, received) ||
                               broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                                "Error: Expected %s but got %s instead on uri_combine_tests[%d].str_props[%d].\n",
                                value, wine_dbgstr_w(received), i, j);
                        }
                    } else {
                        ok(hr == prop.expected,
                            "Error: IUri_GetPropertyBSTR returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].str_props[%d].\n",
                            hr, prop.expected, i, j);
                        ok(!strcmp_aw(value, received) ||
                           broken(prop.broken_value && !strcmp_aw(prop.broken_value, received)),
                            "Error: Expected %s but got %s instead on uri_combine_tests[%d].str_props[%d].\n",
                            value, wine_dbgstr_w(received), i, j);
                    }
                    SysFreeString(received);
                }

                for(j = 0; j < sizeof(uri_combine_tests[i].dword_props)/sizeof(uri_combine_tests[i].dword_props[0]); ++j) {
                    uri_dword_property prop = uri_combine_tests[i].dword_props[j];
                    DWORD received;

                    hr = IUri_GetPropertyDWORD(result, j+Uri_PROPERTY_DWORD_START, &received, 0);
                    if(prop.todo) {
                        todo_wine {
                            ok(hr == prop.expected,
                                "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].dword_props[%d].\n",
                                hr, prop.expected, i, j);
                        }
                        todo_wine {
                            ok(prop.value == received, "Error: Expected %d, but got %d instead on uri_combine_tests[%d].dword_props[%d].\n",
                                prop.value, received, i, j);
                        }
                    } else {
                        ok(hr == prop.expected,
                            "Error: IUri_GetPropertyDWORD returned 0x%08x, expected 0x%08x on uri_combine_tests[%d].dword_props[%d].\n",
                            hr, prop.expected, i, j);
                        ok(prop.value == received, "Error: Expected %d, but got %d instead on uri_combine_tests[%d].dword_props[%d].\n",
                            prop.value, received, i, j);
                    }
                }
            }
            if(result) IUri_Release(result);
            heap_free(relativeW);
        }
        if(base) IUri_Release(base);
        heap_free(baseW);
    }
}

static void test_CoInternetCombineUrlEx_Pluggable(void) {
    HRESULT hr;
    IUri *base = NULL;

    hr = pCreateUri(combine_baseW, 0, 0, &base);
    ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        IUri *result = NULL;

        SET_EXPECT(CombineUrl);

        hr = pCoInternetCombineUrlEx(base, combine_relativeW, URL_DONT_SIMPLIFY|URL_FILE_USE_PATHURL|URL_DONT_UNESCAPE_EXTRA_INFO,
                                     &result, 0);
        ok(hr == S_OK, "Error: CoInternetCombineUrlEx returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

        CHECK_CALLED(CombineUrl);

        if(SUCCEEDED(hr)) {
            BSTR received = NULL;
            hr = IUri_GetAbsoluteUri(result, &received);
            ok(hr == S_OK, "Error: Expected S_OK, but got 0x%08x instead.\n", hr);
            if(SUCCEEDED(hr)) {
                ok(!lstrcmpW(combine_resultW, received), "Error: Expected %s, but got %s.\n",
                    wine_dbgstr_w(combine_resultW), wine_dbgstr_w(received));
            }
            SysFreeString(received);
        }
        if(result) IUri_Release(result);
    }
    if(base) IUri_Release(base);
}

static void test_CoInternetParseIUri_InvalidArgs(void) {
    HRESULT hr;
    IUri *uri = NULL;
    WCHAR tmp[3];
    DWORD result = -1;

    hr = pCoInternetParseIUri(NULL, PARSE_CANONICALIZE, 0, tmp, 3, &result, 0);
    ok(hr == E_INVALIDARG, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
        hr, E_INVALIDARG);
    ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

    hr = pCreateUri(http_urlW, 0, 0, &uri);
    ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        DWORD expected_len;

        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_CANONICALIZE, 0, NULL, 0, &result, 0);
        ok(hr == E_INVALIDARG, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_INVALIDARG);
        ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

        hr = pCoInternetParseIUri(uri, PARSE_CANONICALIZE, 0, tmp, 3, NULL, 0);
        ok(hr == E_POINTER, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_POINTER);

        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_SECURITY_URL, 0, tmp, 3, &result, 0);
        ok(hr == E_FAIL, "Error: CoInternetParseIUri returned 0x%08x expected 0x%08x.\n",
            hr, E_FAIL);
        ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_MIME, 0, tmp, 3, &result, 0);
        ok(hr == E_FAIL, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_FAIL);
        ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_SERVER, 0, tmp, 3, &result, 0);
        ok(hr == E_FAIL, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_FAIL);
        ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_SECURITY_DOMAIN, 0, tmp, 3, &result, 0);
        ok(hr == E_FAIL, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, E_FAIL);
        ok(!result, "Error: Expected 'result' to be 0, but was %d.\n", result);

        expected_len = lstrlenW(http_urlW);
        result = -1;
        hr = pCoInternetParseIUri(uri, PARSE_CANONICALIZE, 0, tmp, 3, &result, 0);
        ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER,
            "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n",
            hr, STRSAFE_E_INSUFFICIENT_BUFFER);
        ok(result == expected_len, "Error: Expected 'result' to be %d, but was %d instead.\n",
            expected_len, result);
    }
    if(uri) IUri_Release(uri);
}

static void test_CoInternetParseIUri(void) {
    DWORD i;

    for(i = 0; i < sizeof(uri_parse_tests)/sizeof(uri_parse_tests[0]); ++i) {
        HRESULT hr;
        IUri *uri;
        LPWSTR uriW;
        uri_parse_test test = uri_parse_tests[i];

        uriW = a2w(test.uri);
        hr = pCreateUri(uriW, test.uri_flags, 0, &uri);
        ok(SUCCEEDED(hr), "Error: CreateUri returned 0x%08x on uri_parse_tests[%d].\n", hr, i);
        if(SUCCEEDED(hr)) {
            WCHAR result[INTERNET_MAX_URL_LENGTH+1];
            DWORD result_len = -1;

            hr = pCoInternetParseIUri(uri, test.action, test.flags, result, INTERNET_MAX_URL_LENGTH+1, &result_len, 0);
            if(test.todo) {
                todo_wine {
                    ok(hr == test.expected,
                        "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x on uri_parse_tests[%d].\n",
                        hr, test.expected, i);
                }
            } else {
                ok(hr == test.expected,
                    "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x on uri_parse_tests[%d].\n",
                    hr, test.expected, i);
            }
            if(SUCCEEDED(hr)) {
                DWORD len = lstrlenA(test.property);
                ok(!strcmp_aw(test.property, result),
                    "Error: Expected %s but got %s instead on uri_parse_tests[%d].\n",
                    test.property, wine_dbgstr_w(result), i);
                ok(len == result_len,
                    "Error: Expected %d, but got %d instead on uri_parse_tests[%d].\n",
                    len, result_len, i);
            } else {
                ok(!result_len,
                    "Error: Expected 'result_len' to be 0, but was %d on uri_parse_tests[%d].\n",
                    result_len, i);
            }
        }
        if(uri) IUri_Release(uri);
        heap_free(uriW);
    }
}

static void test_CoInternetParseIUri_Pluggable(void) {
    HRESULT hr;
    IUri *uri = NULL;

    hr = pCreateUri(parse_urlW, 0, 0, &uri);
    ok(SUCCEEDED(hr), "Error: Expected CreateUri to succeed, but got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) {
        WCHAR result[200];
        DWORD result_len;

        SET_EXPECT(ParseUrl);

        parse_action = PARSE_CANONICALIZE;
        parse_flags = URL_UNESCAPE|URL_ESCAPE_UNSAFE;

        hr = pCoInternetParseIUri(uri, parse_action, parse_flags, result, 200, &result_len, 0);
        ok(hr == S_OK, "Error: CoInternetParseIUri returned 0x%08x, expected 0x%08x.\n", hr, S_OK);

        CHECK_CALLED(ParseUrl);

        if(SUCCEEDED(hr)) {
            ok(result_len == lstrlenW(parse_resultW), "Error: Expected %d, but got %d.\n",
                lstrlenW(parse_resultW), result_len);
            ok(!lstrcmpW(result, parse_resultW), "Error: Expected %s, but got %s.\n",
                wine_dbgstr_w(parse_resultW), wine_dbgstr_w(result));
        }
    }
    if(uri) IUri_Release(uri);
}

START_TEST(uri) {
    HMODULE hurlmon;

    hurlmon = GetModuleHandle("urlmon.dll");
    pCoInternetGetSession = (void*) GetProcAddress(hurlmon, "CoInternetGetSession");
    pCreateUri = (void*) GetProcAddress(hurlmon, "CreateUri");
    pCreateUriWithFragment = (void*) GetProcAddress(hurlmon, "CreateUriWithFragment");
    pCreateIUriBuilder = (void*) GetProcAddress(hurlmon, "CreateIUriBuilder");
    pCoInternetCombineIUri = (void*) GetProcAddress(hurlmon, "CoInternetCombineIUri");
    pCoInternetCombineUrlEx = (void*) GetProcAddress(hurlmon, "CoInternetCombineUrlEx");
    pCoInternetParseIUri = (void*) GetProcAddress(hurlmon, "CoInternetParseIUri");

    if(!pCreateUri) {
        win_skip("CreateUri is not present, skipping tests.\n");
        return;
    }

    trace("test CreateUri invalid flags...\n");
    test_CreateUri_InvalidFlags();

    trace("test CreateUri invalid args...\n");
    test_CreateUri_InvalidArgs();

    trace("test CreateUri invalid URIs...\n");
    test_CreateUri_InvalidUri();

    trace("test IUri_GetPropertyBSTR...\n");
    test_IUri_GetPropertyBSTR();

    trace("test IUri_GetPropertyDWORD...\n");
    test_IUri_GetPropertyDWORD();

    trace("test IUri_GetStrProperties...\n");
    test_IUri_GetStrProperties();

    trace("test IUri_GetDwordProperties...\n");
    test_IUri_GetDwordProperties();

    trace("test IUri_GetPropertyLength...\n");
    test_IUri_GetPropertyLength();

    trace("test IUri_GetProperties...\n");
    test_IUri_GetProperties();

    trace("test IUri_HasProperty...\n");
    test_IUri_HasProperty();

    trace("test IUri_IsEqual...\n");
    test_IUri_IsEqual();

    trace("test CreateUriWithFragment invalid args...\n");
    test_CreateUriWithFragment_InvalidArgs();

    trace("test CreateUriWithFragment invalid flags...\n");
    test_CreateUriWithFragment_InvalidFlags();

    trace("test CreateUriWithFragment...\n");
    test_CreateUriWithFragment();

    trace("test CreateIUriBuilder...\n");
    test_CreateIUriBuilder();

    trace("test IUriBuilder_CreateInvalidArgs...\n");
    test_IUriBuilder_CreateInvalidArgs();

    trace("test IUriBuilder...\n");
    test_IUriBuilder();

    trace("test IUriBuilder_GetInvalidArgs...\n");
    test_IUriBuilder_GetInvalidArgs();

    trace("test IUriBuilder_HasBeenModified...\n");
    test_IUriBuilder_HasBeenModified();

    trace("test IUriBuilder_IUriProperty...\n");
    test_IUriBuilder_IUriProperty();

    trace("test IUriBuilder_RemoveProperties...\n");
    test_IUriBuilder_RemoveProperties();

    trace("test IUriBuilder miscellaneous...\n");
    test_IUriBuilder_Misc();

    trace("test IUriBuilderFactory...\n");
    test_IUriBuilderFactory();

    trace("test CoInternetCombineIUri...\n");
    test_CoInternetCombineIUri();

    trace("test CoInternetCombineUrlEx...\n");
    test_CoInternetCombineUrlEx();

    trace("test CoInternetParseIUri Invalid Args...\n");
    test_CoInternetParseIUri_InvalidArgs();

    trace("test CoInternetParseIUri...\n");
    test_CoInternetParseIUri();

    register_protocols();

    trace("test CoInternetCombineIUri pluggable...\n");
    test_CoInternetCombineIUri_Pluggable();

    trace("test CoInternetCombineUrlEx Pluggable...\n");
    test_CoInternetCombineUrlEx_Pluggable();

    trace("test CoInternetParseIUri pluggable...\n");
    test_CoInternetParseIUri_Pluggable();

    unregister_protocols();
}
