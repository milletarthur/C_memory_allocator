#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdlib.h>

#ifdef DEBUG
#include <stdio.h>
#define debug(...) ((void)fprintf(stderr, __VA_ARGS__))
#else
#define debug(...) ((void)0)
#endif

/* function to retrieve info about the globally allocated memory zone */
void *get_memory_adr();
size_t get_memory_size();

/* function to try to allocate as much as memory as possible */
void *alloc_max(size_t estimate);

/* function to know how many bytes can really be used in the
 * user zone (obtained with mem_alloc)
 *
 * Only used to implement realloc in malloc_stub.c
 */
size_t mem_get_size(void *zone);

// static inline struct allocator_header* get_header();

// static inline size_t get_system_memory_size();

size_t aligne_taille(size_t taille, int alignement);

void fusionner_zl();

typedef struct zones_libres *(mem_fit_function_t)(struct zones_libres *, size_t);

mem_fit_function_t mem_fit_first;
mem_fit_function_t mem_fit_worst;
mem_fit_function_t mem_fit_best;

struct allocator_header {
	size_t memory_size;
	mem_fit_function_t *fit;
	struct zones_libres *liste_zone_libre;
	int taille_max_zone_libre;
};

struct zones_libres {
	size_t size;
	struct zones_libres *next;
};

#endif
