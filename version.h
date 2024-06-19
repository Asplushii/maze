#ifndef VERSION_H
#define VERSION_H

#define MAJOR_VERSION 1
#define MINOR_VERSION 3
#define PATCH_VERSION 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION_STRING TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(PATCH_VERSION)

const char* get_build_timestamp();

#endif
