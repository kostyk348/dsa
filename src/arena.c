#include "dsa_internal.h"
#include <stdlib.h>
#include <string.h>

DSA_Arena *dsa_arena_create(const char *type_name, uint32_t type_id, int audit_cap) {
    DSA_Arena *arena = (DSA_Arena *)malloc(sizeof(DSA_Arena));
    if (!arena) return NULL;
    memset(arena, 0, sizeof(*arena));

    arena->block_capacity = DSA_ARENA_BLOCK_SIZE - sizeof(DSA_ArenaBlock);
    arena->type_name = type_name;
    arena->type_id = type_id;

    if (audit_cap > 0) {
        arena->audit = (DSA_AuditEntry *)malloc(sizeof(DSA_AuditEntry) * audit_cap);
        if (arena->audit) arena->audit_cap = audit_cap;
    }

    /* Allocate first block */
    DSA_ArenaBlock *block = (DSA_ArenaBlock *)malloc(
        sizeof(DSA_ArenaBlock) + DSA_ARENA_BLOCK_SIZE
    );
    if (!block) { free(arena->audit); free(arena); return NULL; }

    uintptr_t raw = (uintptr_t)(block + 1);
    uintptr_t aligned = dsa_align_up(raw, DSA_CACHE_LINE);
    block->data_start = aligned;
    block->capacity = DSA_ARENA_BLOCK_SIZE - (aligned - raw);
    block->used = 0;
    block->next = NULL;
    block->hash = 0;
    arena->head = block;
    arena->tail = block;
    arena->total_allocated = 0;
    arena->total_freed = 0;
    arena->audit_hash = 0;
    return arena;
}

void dsa_arena_destroy(DSA_Arena *arena) {
    if (!arena) return;
    DSA_ArenaBlock *b = arena->head;
    while (b) {
        DSA_ArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    free(arena->audit);
    free(arena);
}

void *dsa_arena_alloc(DSA_Arena *arena, size_t size, size_t alignment, uint32_t line) {
    if (!arena || size == 0) return NULL;
    if (alignment < sizeof(void *)) alignment = sizeof(void *);

    /* Ensure alignment is power of 2 */
    if (!dsa_is_power2(alignment)) alignment = DSA_CACHE_LINE;

    size_t aligned_size = dsa_align_up(size, sizeof(void *));
    DSA_ArenaBlock *block = arena->tail;

    /* Check if current block has room */
    uintptr_t next_data = block->data_start + block->used;
    uintptr_t aligned_next = dsa_align_up(next_data, alignment);
    size_t needed = aligned_next - next_data + aligned_size;

    if (needed > block->capacity - block->used) {
        /* Allocate new block */
        size_t block_size = aligned_size + DSA_ARENA_BLOCK_SIZE;
        if (block_size < DSA_ARENA_BLOCK_SIZE) block_size = DSA_ARENA_BLOCK_SIZE;
        block = (DSA_ArenaBlock *)malloc(sizeof(DSA_ArenaBlock) + block_size);
        if (!block) return NULL;

        uintptr_t raw = (uintptr_t)(block + 1);
        uintptr_t aligned = dsa_align_up(raw, DSA_CACHE_LINE);
        block->data_start = aligned;
        block->capacity = block_size - (aligned - raw);
        block->used = 0;
        block->next = NULL;
        block->hash = 0;
        arena->tail->next = block;
        arena->tail = block;
        next_data = block->data_start;
        aligned_next = next_data;
    }

    /* Bump-allocate */
    void *ptr = (void *)aligned_next;
    size_t actual_size = aligned_size + (aligned_next - next_data);
    block->used += actual_size;
    arena->total_allocated += aligned_size;

    /* Audit */
    uint64_t obj_hash = dsa_ptr_hash(ptr);
    block->hash = dsa_hash_combine(block->hash, obj_hash);
    arena->audit_hash = dsa_hash_combine(arena->audit_hash, obj_hash);

    if (arena->audit && arena->audit_count < arena->audit_cap) {
        DSA_AuditEntry *e = &arena->audit[arena->audit_count++];
        e->prev_hash = arena->audit_hash;
        e->obj_hash = obj_hash;
        e->size = aligned_size;
        e->type_id = arena->type_id;
        e->line = line;
    }

    return ptr;
}

void dsa_arena_reset(DSA_Arena *arena) {
    if (!arena) return;
    /* Bulk-free: reset all blocks to used=0 */
    DSA_ArenaBlock *b = arena->head;
    while (b) {
        arena->total_freed += b->used;
        b->used = 0;
        b->next = NULL;  /* detach overflow blocks */
        b = b->next;
    }
    /* Keep only the head block */
    b = arena->head->next;
    while (b) {
        DSA_ArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    arena->head->next = NULL;
    arena->tail = arena->head;
    arena->epoch++;
    /* Reset audit trail */
    arena->audit_count = 0;
    arena->audit_hash = 0;
}

uint64_t dsa_arena_hash(DSA_Arena *arena) {
    if (!arena) return 0;
    uint64_t h = 0;
    DSA_ArenaBlock *b = arena->head;
    while (b) {
        h = dsa_hash_combine(h, b->hash);
        b = b->next;
    }
    return h;
}
