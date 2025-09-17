#ifndef _AIDS_H
#define _AIDS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* macros for memory arenas */
#define MEMORY_ARENA_DEFAULT_SIZE 4096
#define momory_arena_push(arena, type)	_memory_arena_push(arena, sizeof(type))

/* macros for dynamic arrays */
#define dynamic_array_create(type, allocator)	_dynamic_array_create(sizeof(type), 64, allocator)
#define dynamic_array_destroy(array)		dynamic_array_header(array)->allocator->free(dynamic_array_header(array)->capacity, dynamic_array_header(array), dynamic_array_header(array)->allocator->context)
#define dynamic_array_header(array)		((dynamic_array_header_t*)(array) - 1)
#define dynamic_array_append(array, v)		( \
	(array) = _dynamic_array_ensure_capacity(array, 1, sizeof(v)), \
	(array)[dynamic_array_header(array)->length] = (v), \
	&(array)[dynamic_array_header(array)->length++])
#define array_length(array)		(array_header(array)->length)
#define array_capacity(array)		(array_header(array)->capacity)
#define array_remove(array, index)	do {\
	array_header_t* header = array_header(array); \
	if (index == header->length - 1) { \
		header->length--; \
	} else if (header->length > 1){ \
		void* ptr = &array[index]; \
		void* last = &array[header->length - 1]; \
\
		header->length--; \
		memcpy(ptr, last, sizeof(*array)); \
	} \
} while (0)\

typedef struct {
	void* (*alloc)(size_t size, void* context);
	void* (*free)(size_t size, void* ptr, void* context);
	void* context;
} allocator_t;
typedef struct {
	size_t length;
	void* ptr;
} memory_arena_t;
typedef struct {
	size_t capacity;
	size_t length;
	size_t padding; // unused, padding for 32 bit alignment
	allocator_t* allocator;
} dynamic_array_header_t;

/* functions for allocators */
/* internal use only */
void* _alloc(size_t size, void* context);
void* _free(size_t size, void* ptr, void* context);

/* functions for memory arenas */
memory_arena_t* memory_arena_allocator_create(void);
void memory_arena_allocator_destroy(memory_arena_t* arena);
void memory_arena_clear(memory_arena_t* arena);
/* internal use only */
void* _memory_arena_push(memory_arena_t* arena, size_t size);

/* functions for dynamic arrays */

/* variables */
allocator_t allocator = {
	.alloc = _alloc,
	.free = _free,
	.context = NULL
};

/* implementations for allocators */
void*
_alloc(size_t size, void* context)
{
	void* ptr;

	(void)context;
	ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "malloc()");
		exit(EXIT_FAILURE);
	} else {
		return ptr;
	}
}

void*
_free(size_t size, void* ptr, void* context)
{
	(void)ptr;
	(void)context;
	free(ptr);
}

/* meomory arena function implementations */
memory_arena_t*
memory_arena_allocator_create(void)
{
	void* ptr = calloc(1, MEMORY_ARENA_DEFAULT_SIZE + sizeof(memory_arena_t));
	memory_arena_t* arena = (memory_arena_t*)ptr;

	arena->length = (uint64_t)ptr + sizeof(memory_arena_t);
	return arena;
}

void
memory_arena_allocator_destroy(memory_arena_t* arena)
{
	free(arena);
}

void
memory_arena_clear(memory_arena_t* arena)
{
	arena->length = 0;
}

void*
_memeory_arena_push(memory_arena_t* arena, size_t size)
{
	void* ptr = arena->ptr + arena->length;
	arena->length += size;
	return ptr;
}

/* dynamic array function implementations */
void*
_dynamic_array_create(size_t item_size, size_t capacity, allocator_t* a)
{
	void* ptr = NULL;
	size_t size = item_size * capacity + sizeof(dynamic_array_header_t);
	dynamic_array_header_t *h = a->alloc(size, a->context);

	if (h) {
		h->capacity = capacity;
		h->length = 0;
		h->allocator = a;
		ptr = h + 1;
	}
	return ptr;
}

void*
_dynamic_array_ensure_capacity(void* array, size_t length, size_t item_size)
{
	dynamic_array_header_t* header = dynamic_array_header(array);
	size_t new_length = header->length + length;

	if (header->capacity < new_length) {
		size_t new_capacity = header->capacity * 2;

		while (new_capacity < new_length)
			new_capacity *= 2;
		size_t new_size = sizeof(dynamic_array_header_t) + new_capacity * item_size;
		dynamic_array_header_t* new_header = header->allocator->alloc(new_size,
				header->allocator->context);
		if (new_header) {
			size_t old_size = sizeof(*header) + header->length * item_size;

			memcpy(new_header, header, old_size);
			if (header->allocator->free)
				header->allocator->free(old_size, header,
						header->allocator->context);
			new_header->capacity = new_capacity;
			header = new_header + 1;
		} else {
			header = NULL;
		}
	} else {
		header++;
	}
	return header;
}

#endif /* _AIDS_H */
