#pragma once

#if defined(DEBUG_LOG)
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...)
#endif
