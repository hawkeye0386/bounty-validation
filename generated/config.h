#pragma once

#define HAVE_FSEEKO 0
#define HAVE_FSEEK64 1
#define fseek64 _fseeki64
#define ftell64 _ftelli64
using off64_t = __int64;
