#ifndef HPDF_CONFIG_H
#define HPDF_CONFIG_H

/* LibHaru configuration (manual, without CMake) */

/* zlib is available */
#define LIBHPDF_HAVE_ZLIB 1

/* PNG support (disabled unless you build libpng yourself) */
#undef LIBHPDF_HAVE_LIBPNG

/* Debug mode disabled */
#undef LIBHPDF_DEBUG
#undef LIBHPDF_DEBUG_TRACE

#endif /* HPDF_CONFIG_H */
