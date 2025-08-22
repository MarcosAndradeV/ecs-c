#define ECS_IMPLEMENTATION
#include "ecs.h"

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "raylib.h"

Component(Player, struct {})
Component(Rect, Rectangle)
Component(Color, Color)
Component(Velocity, struct {
    float vx, vy;
})

System(draw_rects) {
    QueryByComponents(e, COMP_Color | COMP_Rect) {
        Color *c = get_Color(e);
        Rectangle *r = get_Rect(e);
        DrawRectangleRec(*r, *c);
    }
}

System(move_rects) {
    QueryByComponents(e, COMP_Velocity | COMP_Rect) {
        Velocity *v = get_Velocity(e);
        Rectangle *r = get_Rect(e);
        r->x += v->vx;
        r->y += v->vy;
    }
}

System(keyborad_events) {
    QueryByComponents(e, COMP_Player | COMP_Velocity) {
        Velocity *v = get_Velocity(e);
        if(IsKeyPressed(KEY_W)) {v->vy = -1.0f; return;}
        if(IsKeyPressed(KEY_S)) {v->vy =  1.0f; return;}
        if(IsKeyPressed(KEY_D)) {v->vx =  1.0f; return;}
        if(IsKeyPressed(KEY_A)) {v->vx = -1.0f; return;}
    }
}

int main(void) {
    register_Player();
    register_Rect();
    register_Color();
    register_Velocity();

    InitWindow(800, 600, "test");
    SetTargetFPS(60);

    Entity *square = spawn_entity();
    add_Rect(square, (Rectangle){100.0f, 100.0f, 20.0f, 20.0f});
    add_Velocity(square, (Velocity){0});
    add_Color(square, RED);
    add_Player(square, (Player){});

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(DARKGRAY);
            keyborad_events_system();
            move_rects_system();
            draw_rects_system();
        EndDrawing();
    }
    CloseWindow();
    ecs_deinit();
}
