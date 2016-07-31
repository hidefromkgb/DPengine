#ifndef HDR_OBJC
#define HDR_OBJC

/// [IMPORTANT:] use CoreFoundation classes wherever possible!
/// There is no ARC in pure C, so the bridging will be direct!

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <stdarg.h>



#if __LP64__ || TARGET_OS_EMBEDDED || TARGET_OS_IPHONE || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
    typedef long NSInteger;
    typedef unsigned long NSUInteger;
#else
    typedef int NSInteger;
    typedef unsigned int NSUInteger;
#endif



static struct {
    Class uuid;
    char *name;
    long icnt;
} *SubclassedObjCClasses = 0;
static id *LoadedObjCClasses = 0;
static SEL *LoadedObjCSelectors = 0;



/// OSX versions
#define OSX_10_05_PLUS (kCFCoreFoundationVersionNumber >=  476.00)
#define OSX_10_06_PLUS (kCFCoreFoundationVersionNumber >=  550.00)
#define OSX_10_07_PLUS (kCFCoreFoundationVersionNumber >=  635.00)
#define OSX_10_08_PLUS (kCFCoreFoundationVersionNumber >=  744.00)
#define OSX_10_09_PLUS (kCFCoreFoundationVersionNumber >=  855.11)
#define OSX_10_10_PLUS (kCFCoreFoundationVersionNumber >= 1151.16)

/// NSString creation macro
#define UTF8(s) CFStringCreateWithBytes(0, (s)? (uint8_t*)(s) : (uint8_t*)"", strlen((s)? (char*)(s) : (char*)""), kCFStringEncodingUTF8, false)

/// class instance variable management macros
#define GET_IVAR(inst, name, data) object_getInstanceVariable(inst, name, (void**)(data))
#define SET_IVAR(inst, name, data) object_setInstanceVariable(inst, name, (void*)(data))

/// getter macros for double floats and structures of double floats
#define GetT1DV(r, i, ...) { void *GetT1DV = objc_msgSend_fpret; r = ((typeof(r) (*)(id, ...))GetT1DV)(i, __VA_ARGS__); }
#define GetT2DV(r, i, ...) { void *GetT2DV = objc_msgSend;       r = ((typeof(r) (*)(id, ...))GetT2DV)(i, __VA_ARGS__); }
#define GetT4DV(r, i, ...) { void *GetT4DV = objc_msgSend_stret; r = ((typeof(r) (*)(id, ...))GetT4DV)(i, __VA_ARGS__); }

/// helper function for a tricky task of passing floats to objc_msgSend()
/// (as it is a vararg, all passed floats are promoted to doubles, which
/// is not the right behaviour in the case of IA-32 architecture)
#ifdef __i386__
    #define ___F uint32_t
    #define __PassT1FV(f) *(___F*)&(f)
#else
    #define ___F double
    #define __PassT1FV
#endif
__attribute__((unused)) /// signals that the function might be left unused
static inline ___F PassT1FV(float what) {
    return __PassT1FV(what);
}
#undef ___F
#undef __PassT1FV



__attribute__((unused))
static char *CopyUTF8(CFStringRef cfsr) {
    CFIndex slen, size;
    uint8_t *retn = 0;

    if (CFStringGetBytes(cfsr, CFRangeMake(0, slen = CFStringGetLength(cfsr)),
                         kCFStringEncodingUTF8, '?', false, 0, 0, &size) > 0)
        CFStringGetBytes(cfsr, CFRangeMake(0, slen), kCFStringEncodingUTF8,
                         '?', false, retn = calloc(1, 1 + size), size, 0);
    return (char*)retn;
}



/// When the official documentation states that there is an NSDictionary to
/// be created with some NS<Whatever> values used as keys, remember that in
/// the CoreFoundation framework their equivalents are named kCT<Whatever>.
/// If there are none in CF, just import them via "extern id NS<Whatever>".
#define MakeDict(k, ...) __MakeDict(k, __VA_ARGS__, nil)
__attribute__((unused))
static CFDictionaryRef __MakeDict(CFStringRef key1, ...) {
    CFDictionaryRef retn = 0;
    CFStringRef iter;
    va_list list;
    long size;

    CFStringRef *keys;
    id *vals;

    size = 0;
    iter = key1;
    va_start(list, key1);
    while (iter) {
        va_arg(list, void*);
        iter = va_arg(list, typeof(iter));
        size++;
    }
    va_end(list);
    if (size) {
        keys = malloc(size * sizeof(*keys));
        vals = malloc(size * sizeof(*vals));
        size = 0;
        iter = key1;
        va_start(list, key1);
        while (iter) {
            keys[size] = iter;
            vals[size++] = va_arg(list, typeof(*vals));
            iter = va_arg(list, typeof(iter));
        }
        va_end(list);
        retn = CFDictionaryCreate(0, (const void**)keys, (const void**)vals,
                                  size, &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
        free(vals);
        free(keys);
    }
    return retn;
}



#define PutToArr(...) __PutToArr(nil, ##__VA_ARGS__, nil)
__attribute__((unused))
static id *__PutToArr(id head, ...) {
    va_list list;
    long size;
    id *retn;

    retn = 0;
    va_start(list, head);
    for (size = 0; va_arg(list, typeof(*retn)); size++);
    va_end(list);
    if (size) {
        retn = malloc((size + 1) * sizeof(*retn));
        va_start(list, head);
        for (size = 0; (retn[size] = va_arg(list, typeof(*retn))); size++);
        va_end(list);
    }
    return retn;
}

__attribute__((unused))
static id NewClass(id base, char *name, id *flds, id *mths) {
    Class retn;
    long iter;

    for (retn = 0, iter = 0;
         SubclassedObjCClasses && SubclassedObjCClasses[iter].name; iter++)
        if (!strcmp(name, SubclassedObjCClasses[iter].name)) {
            retn = SubclassedObjCClasses[iter].uuid;
            SubclassedObjCClasses[iter].icnt++;
            break;
        }
    if (!retn) {
        retn = objc_allocateClassPair((Class)base, name, 0);
        SubclassedObjCClasses = realloc(SubclassedObjCClasses, (iter + 2)
                                      * sizeof(*SubclassedObjCClasses));
        SubclassedObjCClasses[iter] =
            (typeof(*SubclassedObjCClasses)){retn, strdup(name), 1};
        SubclassedObjCClasses[iter + 1].name = 0;

        iter = -1;
        /// adding fields
        while (flds && flds[++iter])
            class_addIvar(retn, (char*)flds[iter],
                          sizeof(id), (sizeof(id) >= 8)? 3 : 2, 0);
        iter = -2;
        /// overloading methods
        while (mths && mths[iter += 2])
            class_addMethod(retn, (SEL)mths[iter], (IMP)mths[iter + 1], 0);

        objc_registerClassPair(retn);
    }
    return (id)retn;
}



__attribute__((unused))
static void DelClass(Class uuid) {
    long iter, size = 0;

    for (; SubclassedObjCClasses && SubclassedObjCClasses[size].name; size++);
    for (iter = 0; iter < size; iter++)
        if (uuid == SubclassedObjCClasses[iter].uuid) {
            SubclassedObjCClasses[iter].icnt--;
            if (!SubclassedObjCClasses[iter].icnt) {
                objc_disposeClassPair(SubclassedObjCClasses[iter].uuid);
                free(SubclassedObjCClasses[iter].name);
                if (iter < --size) {
                    SubclassedObjCClasses[iter] = SubclassedObjCClasses[size];
                    iter = size;
                }
                SubclassedObjCClasses[iter].name = 0;
            }
            break;
        }
    if (SubclassedObjCClasses && !SubclassedObjCClasses[0].name) {
        free(SubclassedObjCClasses);
        SubclassedObjCClasses = 0;
    }
}



/// no __attribute__((unused)) here: this function has to be called ASAP
static void LoadObjC(char *clas[], char *sele[]) {
    long iter;

    if (!LoadedObjCClasses) {
        for (iter = 0; clas[iter]; iter++);
        LoadedObjCClasses = malloc(iter * sizeof(*LoadedObjCClasses));
        for (iter = 0; sele[iter]; iter++);
        LoadedObjCSelectors = malloc(iter * sizeof(*LoadedObjCSelectors));
        for (iter = 0; clas[iter]; iter++)
            LoadedObjCClasses[iter] = (id)objc_getClass(clas[iter]);
        for (iter = 0; sele[iter]; iter++)
            LoadedObjCSelectors[iter] = sel_registerName(sele[iter]);
    }
}

#endif
