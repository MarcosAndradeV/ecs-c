#define ECS_IMPLEMENTATION
#include "../ecs.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <string.h>


#define BOARD_WIDTH 40
#define BOARD_HEIGHT 10
#define MAX_SNAKE_LENGTH (BOARD_WIDTH * BOARD_HEIGHT)


Component(Position, struct { int x, y; });
Component(Velocity, struct { int dx, dy; });
Component(SnakeHead, struct { int length; });
Component(SnakeBody, struct { int segment_index; });
Component(Food, struct { int value; });
Component(Renderable, struct { char symbol; });


typedef struct {
    bool running;
    int score;
    struct termios old_termios;
} GameState;

static GameState game_state = {0};

void setup_terminal() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &game_state.old_termios);
    new_termios = game_state.old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &game_state.old_termios);
}

bool kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

System(input) {
    if (!kbhit()) return;

    char input = getchar();

    QueryByComponents(snake, COMP_SnakeHead | COMP_Velocity) {
        Velocity* vel = get_Velocity(snake);

        switch(input) {
            case 'w':
            case 'W':
                if (vel->dy != 1) { vel->dx = 0; vel->dy = -1; }
                break;
            case 's':
            case 'S':
                if (vel->dy != -1) { vel->dx = 0; vel->dy = 1; }
                break;
            case 'a':
            case 'A':
                if (vel->dx != 1) { vel->dx = -1; vel->dy = 0; }
                break;
            case 'd':
            case 'D':
                if (vel->dx != -1) { vel->dx = 1; vel->dy = 0; }
                break;
            case 'q':
            case 'Q':
                game_state.running = false;
                break;
        }
    }
}

System(movement) {
    static Position snake_positions[MAX_SNAKE_LENGTH];
    static int position_count = 0;

    QueryByComponents(head, COMP_SnakeHead | COMP_Position | COMP_Velocity) {
        Position* head_pos = get_Position(head);
        Velocity* vel = get_Velocity(head);
        SnakeHead* snake_head = get_SnakeHead(head);

        if (position_count < MAX_SNAKE_LENGTH) {
            snake_positions[position_count] = *head_pos;
            position_count++;
        } else {
            for (int i = 0; i < MAX_SNAKE_LENGTH - 1; i++) {
                snake_positions[i] = snake_positions[i + 1];
            }
            snake_positions[MAX_SNAKE_LENGTH - 1] = *head_pos;
        }

        head_pos->x += vel->dx;
        head_pos->y += vel->dy;

        if (head_pos->x < 0) head_pos->x = BOARD_WIDTH - 1;
        if (head_pos->x >= BOARD_WIDTH) head_pos->x = 0;
        if (head_pos->y < 0) head_pos->y = BOARD_HEIGHT - 1;
        if (head_pos->y >= BOARD_HEIGHT) head_pos->y = 0;

        QueryByComponents(body, COMP_SnakeBody | COMP_Position) {
            SnakeBody* snake_body = get_SnakeBody(body);
            if (snake_body->segment_index < snake_head->length &&
                snake_body->segment_index < position_count - 1) {
                Position* body_pos = get_Position(body);
                int pos_idx = position_count - 2 - snake_body->segment_index;
                if (pos_idx >= 0) {
                    *body_pos = snake_positions[pos_idx];
                }
            }
        }
    }
}

System(collision) {
    QueryByComponents(head, COMP_SnakeHead | COMP_Position) {
        Position* head_pos = get_Position(head);
        SnakeHead* snake_head = get_SnakeHead(head);

        QueryByComponents(body, COMP_SnakeBody | COMP_Position) {
            Position* body_pos = get_Position(body);
            if (head_pos->x == body_pos->x && head_pos->y == body_pos->y) {
                game_state.running = false;
                return;
            }
        }

        QueryByComponents(food, COMP_Food | COMP_Position) {
            Position* food_pos = get_Position(food);
            if (head_pos->x == food_pos->x && head_pos->y == food_pos->y) {
                snake_head->length++;
                game_state.score += 10;

                ECSEntity* new_segment = ecs_spawn_entity();
                add_Position(new_segment, (Position){head_pos->x, head_pos->y});
                add_SnakeBody(new_segment, (SnakeBody){snake_head->length - 1});
                add_Renderable(new_segment, (Renderable){'o'});

                food_pos->x = rand() % BOARD_WIDTH;
                food_pos->y = rand() % BOARD_HEIGHT;
            }
        }

    }
}

System(render) {
    printf("\033[2J\033[H");

    char board[BOARD_HEIGHT][BOARD_WIDTH];
    memset(board, ' ', sizeof(board));

    QueryByComponents(entity, COMP_Position | COMP_Renderable) {
        Position* pos = get_Position(entity);
        Renderable* render = get_Renderable(entity);
        if (pos->x >= 0 && pos->x < BOARD_WIDTH && pos->y >= 0 && pos->y < BOARD_HEIGHT) {
            board[pos->y][pos->x] = render->symbol;
        }
    }

    printf("┌");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("─");
    printf("┐\n");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            printf("%c", board[y][x]);
        }
        printf("│\n");
    }

    printf("└");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("─");
    printf("┘\n");

    printf("Score: %d\n", game_state.score);
    printf("Controls: WASD to move, Q to quit\n");

    fflush(stdout);
}

void spawn_snake() {
    ECSEntity* head = ecs_spawn_entity();
    add_Position(head, (Position){BOARD_WIDTH/2, BOARD_HEIGHT/2});
    add_Velocity(head, (Velocity){1, 0});
    add_SnakeHead(head, (SnakeHead){3});
    add_Renderable(head, (Renderable){'@'});

    for (int i = 0; i < 3; i++) {
        ECSEntity* body = ecs_spawn_entity();
        add_Position(body, (Position){BOARD_WIDTH/2 - 1 - i, BOARD_HEIGHT/2});
        add_SnakeBody(body, (SnakeBody){i});
        add_Renderable(body, (Renderable){'o'});
    }
}

void spawn_food() {
    ECSEntity* food = ecs_spawn_entity();
    add_Position(food, (Position){rand() % BOARD_WIDTH, rand() % BOARD_HEIGHT});
    add_Food(food, (Food){1});
    add_Renderable(food, (Renderable){'*'});
}

int main() {
    srand(time(NULL));

    register_Position();
    register_Velocity();
    register_SnakeHead();
    register_SnakeBody();
    register_Food();
    register_Renderable();

    setup_terminal();
    game_state.running = true;

    spawn_snake();
    spawn_food();

    struct timespec last_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    while (game_state.running) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed = (current_time.tv_sec - last_time.tv_sec) +
                        (current_time.tv_nsec - last_time.tv_nsec) / 1e9;

        if (elapsed >= 0.15) {
            input_system();
            movement_system();
            collision_system();
            render_system();
            last_time = current_time;
        }

        usleep(1000);
    }

    restore_terminal();
    printf("\nGame Over! Final Score: %d\n", game_state.score);
    ecs_deinit();

    return 0;
}
