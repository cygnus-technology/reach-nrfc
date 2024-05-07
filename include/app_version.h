/*
 * app_version.h
 *
 *  Version of application
 */

#ifndef _APP_VERSION_H_
#define _APP_VERSION_H_

#define APP_MAJOR_VERSION 0
#define APP_MINOR_VERSION 1
#define APP_PATCH_VERSION 0

// appends a note string to the version
// #define DEV_BUILD

extern void print_versions();
extern const char *get_app_version();

#endif  // ndef _APP_VERSION_H_