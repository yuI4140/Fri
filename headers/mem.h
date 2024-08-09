#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#define ALIGNMENT 8

#define REN_BACKEND_C 0
#define REN_BACKEND_LINUX 1
#define REN_BACKEND_WIN32 2

#ifndef REN_BACKEND
#define REN_BACKEND REN_BACKEND_C
#endif

#define REGION_DEFAULT_CAPACITY (8 * 1024)

typedef struct Region {
    struct Region *next;
    size_t count;
    size_t capacity;
    uintptr_t data[];
} Region;

typedef struct Ren {
    Region *begin, *end;
} Ren;

static void r_assert(int condition, const char *message);
static void r_p_assert(void *ptr, const char *message);
Ren *create_Ren(size_t initial_capacity);
void destroy_Ren(Ren *r);
void *Ren_alloc(Ren *r, size_t size_bytes);
void *Ren_realloc(Ren *r, void *oldptr, size_t oldsz, size_t newsz);
void Ren_reset(Ren *r);
char *Ren_strdup(Ren *r, const char *cstr);
void *Ren_memdup(Ren *r, void *data, size_t size);
void Ren_free(Ren *r);

#ifdef MEM_IMP

static void r_assert(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "Error: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void r_p_assert(void *ptr, const char *message) {
    if (!ptr) {
        fprintf(stderr, "Error: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

#if REN_BACKEND == REN_BACKEND_C

Region *new_Region(size_t capacity) {
    size_t size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *r = (Region *)malloc(size_bytes);
    r_p_assert(r, "Memory allocation failed for Region");
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void free_Region(Region *r) {
    free(r);
}

#elif REN_BACKEND == REN_BACKEND_LINUX
#include <unistd.h>
#include <sys/mman.h>

Region *new_Region(size_t capacity) {
    size_t size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *r = mmap(NULL, size_bytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    r_assert(r != MAP_FAILED, "mmap() failed");
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void free_Region(Region *r) {
    size_t size_bytes = sizeof(Region) + sizeof(uintptr_t) * r->capacity;
    int ret = munmap(r, size_bytes);
    r_assert(ret == 0, "munmap() failed");
}

#elif REN_BACKEND == REN_BACKEND_WIN32

#if !defined(_WIN32)
#error "CurRent platform is not Windows"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define INV_HANDLE(x) (((x) == NULL) || ((x) == INVALID_HANDLE_VALUE))

Region *new_Region(size_t capacity) {
    SIZE_T size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *r = VirtualAllocEx(
        GetCurRentProcess(), /* Allocate in curRent process address space */
        NULL,                /* Unknown position */
        size_bytes,          /* Bytes to allocate */
        MEM_COMMIT | MEM_RESERVE, /* Reserve and commit allocated page */
        PAGE_READWRITE       /* Permissions (Read/Write) */
    );
    if (INV_HANDLE(r))
        r_assert(0 && "VirtualAllocEx() failed.");

    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void free_Region(Region *r) {
    if (INV_HANDLE(r))
        return;

    BOOL free_result = VirtualFreeEx(
        GetCurRentProcess(), /* Deallocate from curRent process address space */
        (LPVOID)r,           /* Address to deallocate */
        0,                   /* Bytes to deallocate (Unknown, deallocate entire page) */
        MEM_RELEASE          /* Release the page (And implicitly decommit it) */
    );

    if (FALSE == free_result)
        r_assert(0 && "VirtualFreeEx() failed.");
}

#else
#error "Unknown Ren backend"
#endif

Ren *create_Ren(size_t initial_capacity) {
    Ren *r = (Ren *)malloc(sizeof(Ren));
    r_p_assert(r, "Memory allocation failed for Ren");

    r->begin = r->end = new_Region(initial_capacity > 0 ? initial_capacity : REGION_DEFAULT_CAPACITY);

    return r;
}

void destroy_Ren(Ren *r) {
    if (r) {
        Ren_free(r);
        free(r);
    }
}

void *Ren_alloc(Ren *r, size_t size_bytes) {
    size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (r->end == NULL) {
        r_assert(r->begin == NULL, "Ren end is NULL but begin is not");
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size)
            capacity = size;
        r->end = new_Region(capacity);
        r->begin = r->end;
    }

    while (r->end->count + size > r->end->capacity && r->end->next != NULL) {
        r->end = r->end->next;
    }

    if (r->end->count + size > r->end->capacity) {
        r_assert(r->end->next == NULL, "Region has more capacity than expected");
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size)
            capacity = size;
        r->end->next = new_Region(capacity);
        r->end = r->end->next;
    }

    void *result = &r->end->data[r->end->count];
    r->end->count += size;
    return result;
}

void *Ren_realloc(Ren *r, void *oldptr, size_t oldsz, size_t newsz) {
    if (newsz <= oldsz)
        return oldptr;
    void *newptr = Ren_alloc(r, newsz);
    memcpy(newptr, oldptr, oldsz);
    return newptr;
}

char *Ren_strdup(Ren *r, const char *cstr) {
    size_t n = strlen(cstr);
    char *dup = (char *)Ren_alloc(r, n + 1);
    memcpy(dup, cstr, n);
    dup[n] = '\0';
    return dup;
}

void *Ren_memdup(Ren *r, void *data, size_t size) {
    return memcpy(Ren_alloc(r, size), data, size);
}

void Ren_reset(Ren *r) {
    for (Region *reg = r->begin; reg != NULL; reg = reg->next) {
        reg->count = 0;
    }

    r->end = r->begin;
}

void Ren_free(Ren *r) {
    Region *reg = r->begin;
    while (reg) {
        Region *reg0 = reg;
        reg = reg->next;
        free_Region(reg0);
    }
    r->begin = NULL;
    r->end = NULL;
}
#endif // MEM_IMP
