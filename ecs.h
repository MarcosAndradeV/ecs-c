#ifndef ECS_H_
#define ECS_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#ifndef ECS_DA_INIT_CAP
#define ECS_DA_INIT_CAP 256
#endif
#ifndef ECS_REALLOC
#define ECS_REALLOC realloc
#endif
#ifndef ECS_ASSERT
#define ECS_ASSERT assert
#endif

#define ecs_da_reserve(da, expected_capacity)                                              \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = ECS_DA_INIT_CAP;                                          \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = ECS_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            ECS_ASSERT((da)->items != NULL && "Buy more RAM lol");                         \
        }                                                                                  \
    } while (0)

#define ecs_da_append(da, item)                \
    do {                                       \
        ecs_da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item);   \
    } while (0)


#define ecs_da_last(da) (da)->items[(ECS_ASSERT((da)->count > 0), (da)->count-1)]
#define ecs_da_remove_unordered(da, i)               \
    do {                                             \
        size_t j = (i);                              \
        ECS_ASSERT(j < (da)->count);                 \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while(0)


#define ecs_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#define Component(name, ...) \
    static size_t COMP_##name; \
    typedef __VA_ARGS__ name; \
    typedef struct {size_t capacity, count; name * items;} ECS_DA_##name;\
    static ECS_DA_##name name##_components; \
    void cleanup_##name() { \
        free(name##_components.items); \
    }\
    void register_##name() { \
        if(COMP_##name == 0) COMP_##name = 1 << component_type_iota(); \
        ecs_da_append(&components_cleanups, cleanup_##name);\
    }\
    name* get_##name(Entity* e) { return &name##_components.items[e->id]; } \
    void add_##name(Entity* e, name value) { \
        if(COMP_##name == 0) { printf("[ERROR] Forgot to register `%s` componet first\n", #name); abort(); }\
        e->mask |= COMP_##name; \
        ecs_da_append(&(name##_components), value); \
    }

#define System(name, ...) void name##_system(__VA_ARGS__)
#define QueryByComponents(e, ...) \
    ecs_da_foreach(Entity, e, &entities) if(has_components(e, __VA_ARGS__))
#define QueryById(e, _id) \
    ecs_da_foreach(Entity, e, &entities) if(e->id == _id)

// ----------------------
// ECS "registry"
// ----------------------
typedef unsigned long int ECSEntityMask;
typedef size_t ECSEntityId;

typedef struct {
    ECSEntityMask mask; // bitmask dos componentes
    ECSEntityId id;
} Entity;

typedef struct {
    Entity *items;
    size_t capacity, count;
} Entities;

typedef struct {
    ECSEntityId *items;
    size_t capacity, count;
} EntityIds;

typedef void (*ComponentsCleanupCallback)();
typedef struct {
    size_t capacity, count;
    ComponentsCleanupCallback * items;
} ComponentsCleanups;

static Entities entities;
static EntityIds dead_entities;
static ComponentsCleanups components_cleanups;

// ----------------------
// Helpers
// ----------------------

Entity* spawn_entity();
void despawn_entity(Entity *e);
void despawn_entity_with_id(ECSEntityId id);
Entity* get_entity_with_id(ECSEntityId id);
bool has_components(Entity* e, ECSEntityMask mask) ;
size_t component_type_iota();

// #define  ECS_IMPLEMENTATION
#ifdef ECS_IMPLEMENTATION

Entity* spawn_entity() {
    ECSEntityId id;
    if(dead_entities.count > 0) {
       id = ecs_da_last(&dead_entities);
       dead_entities.count--;
    } else {
        id = entities.count;
        Entity e = {
            .id = id,
        };
        ecs_da_append(&entities, e);
    }
    return &entities.items[id];
}

void despawn_entity(Entity *e) {
    e->mask = 0;
    ecs_da_append(&dead_entities, e->id);
}

void despawn_entity_with_id(ECSEntityId id) {
    entities.items[id].id = 0;
    ecs_da_append(&dead_entities, id);
}

Entity* get_entity_with_id(ECSEntityId id) {
    return &entities.items[id];
}

bool has_components(Entity* e, ECSEntityMask mask) {
    return (e->mask & mask) == mask;
}

size_t component_type_iota() {
    static size_t id = 0;
    return id++;
}

void ecs_deinit() {
    free(entities.items);
    free(dead_entities.items);
    ecs_da_foreach(ComponentsCleanupCallback, it, &components_cleanups) {
        (*it)();
    }
    free(components_cleanups.items);
}

#endif // ECS_IMPLEMENTATION

#endif // ECS_H_
