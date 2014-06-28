#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "../core/engine.h"



#ifndef O_BINARY
#define O_BINARY 0
#endif

/// Default animation folder
#define DEF_FLDR "../anim"
/// Default config file
#define DEF_CONF "pony.ini"
/// Default comment character
#define DEF_CMNT '\''
/// Default token separator
#define DEF_TSEP ','

/// [not recognized / comment / invalid token]
#define AVT_NONE 0x00000000
/// 'Name'
#define AVT_NAME 0xC6961EB1
/// 'Effect'
#define AVT_EFCT 0xE708AEFF
/// 'Behavior'
#define AVT_BHVR 0xB6AB6234
/// 'behaviorgroup'
#define AVT_BGRP 0xA40004B2
/// 'Categories'
#define AVT_CTGS 0xC831B868
/// 'Speak'
#define AVT_PHRS 0x90E5E31D



#define FRM_WAIT 40



/// elementary animation unit
typedef struct _UNIT {
    char *path;           /// path to the original animation file
    uint32_t uuid, *time; /// unique unit identifier and frame delays array
    uint32_t fram, fcnt;  /// current frame and frame count
    uint32_t xdim, ydim;  /// dimensions (actual, non-modified)
    uint32_t xpos, ypos;  /// position of the unit`s lower-left corner
    uint32_t xptr, yptr;  /// mouse position for dragging
    uint64_t tstp;        /// timestamp of the previous frame
    struct _UNIT *prev;   /// previous unit in the list
    struct _UNIT *next;   /// next unit in the list
} UNIT;

/// unit library
typedef struct _ULIB {
    char *path;           /// the folder from which the library was built
    UNIT **uarr;          /// array of animation units
    long ucnt;            /// length of the array
    uint32_t flgs;        /// library flags (ULF_ prefix)
    struct _ULIB *prev;   /// previous library in the list
    struct _ULIB *next;   /// next library in the list
} ULIB;



void MakeEmptyLib(ULIB **head, char *base, char *path);
void FillLib(ULIB *ulib, char *pcnf, LOAD load);
long UnitListFromLib(ULIB *ulib, long uses, long xdim, long ydim);
void FreeEverything(ULIB **ulib);

void UpdateFrame(T2UV *data, uint64_t *time, uint32_t flgs,
                 int32_t xptr, int32_t yptr, int32_t isel);
