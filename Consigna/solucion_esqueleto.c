// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

char* strDuplicate(char* src) {
    int longitud = 0;

    while (src[longitud] != '\0') {
        longitud++;
    }

    // +1 para el '\0', que no se cuenta en longitud pero hay que copiarlo
    char* copia = malloc(longitud + 1);

    if (copia == NULL) {
        return NULL;
    }

    int i = 0;

    while (i < longitud) {
        copia[i] = src[i];
        i++;
    }

    copia[longitud] = '\0';

    return copia;
}

int strCompare(char* s1, char* s2) {
    int i = 0;

    // Avanza mientras coincidan. Si uno es prefijo del otro, frena en su '\0',
    // que es menor que cualquier caracter: asi el prefijo queda como menor.
    while (s1[i] == s2[i] && s1[i] != '\0') {
        i++;
    }

    if (s1[i] == s2[i]) {
        return 0;
    }

    // Se compara como unsigned char (igual que strcmp) para que los caracteres
    // no ASCII (> 127) no queden negativos y se ordenen antes que 'a'.
    if ((unsigned char)s1[i] < (unsigned char)s2[i]) {
        return 1;
    } else {
        return -1;
    }
}

char* strConcatenate(char* src1, char* src2) {
    int longitud1 = 0;
    while (src1[longitud1] != '\0') {
        longitud1++;
    }

    int longitud2 = 0;
    while (src2[longitud2] != '\0') {
        longitud2++;
    }

    // Un solo malloc con el largo total de ambos, mas 1 para el '\0' final
    char* resultado = malloc(longitud1 + longitud2 + 1);

    if (resultado == NULL) {
        return NULL;
    }

    int i = 0;
    while (i < longitud1) {
        resultado[i] = src1[i];
        i++;
    }

    // i sigue desde donde termino src1: src2 se copia a continuacion
    int j = 0;
    while (j < longitud2) {
        resultado[i] = src2[j];
        i++;
        j++;
    }

    resultado[i] = '\0';

    // Los originales se liberan recien despues de copiarlos.
    // Precondicion: vienen de malloc y son punteros distintos.
    free(src1);
    free(src2);

    return resultado;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

void gameBoardDelete(GameBoard* board);   // gameBoardNew la usa para limpiar si falla un malloc

GameBoard* gameBoardNew() {
    GameBoard* board = malloc(sizeof(GameBoard));
    if (board == NULL) return NULL;

    // Todos los punteros arrancan en NULL: asi gameBoardDelete puede liberar
    // un tablero a medio construir si falla algun malloc de las filas.
    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    board->score = 0;
    board->tick_timer = 0;

    // Cada fila nueva se inserta en la cabeza (O(1), sin recorrer la lista).
    // Como todas arrancan vacias, el orden de creacion no importa.
    for (int i = 0; i < GRID_ROWS; i++) {
        RowNode* row = malloc(sizeof(RowNode));
        if (row == NULL) {
            gameBoardDelete(board);   // libera las filas ya enlazadas y el board
            return NULL;
        }
        for (int c = 0; c < GRID_COLS; c++) row->cells[c] = 0;
        row->next = board->first_row;
        board->first_row = row;
    }
    return board;
}

void gameBoardDelete(GameBoard* board) {
    if (board == NULL) return;

    // En las tres listas se guarda el siguiente ANTES del free, porque
    // despues de liberar el nodo no se puede leer su campo next.
    RowNode* row = board->first_row;
    while (row != NULL) {
        RowNode* next = row->next;
        free(row);
        row = next;
    }

    PieceQueueNode* piece = board->queue_head;
    while (piece != NULL) {
        PieceQueueNode* next = piece->next;
        free(piece);
        piece = next;
    }

    ParticleNode* particle = board->first_particle;
    while (particle != NULL) {
        ParticleNode* next = particle->next;
        free(particle);
        particle = next;
    }

    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    // TODO: Crear un ParticleNode con memoria dinámica.
    // TODO: Inicializar x, y, velocidades aleatorias y un life_timer.
    // TODO: Agregarlo al principio de la lista enlazada simple de partículas.
    printf("Función gameBoardAddParticle no implementada.\n");
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    // TODO: Recorrer la lista de RowNode.
    // TODO: Si un RowNode tiene todas sus celdas > 0, desenlazarlo y hacer free().
    // TODO: Crear un nuevo RowNode vacío (lleno de ceros).
    // TODO: Insertar el nuevo RowNode en la CABEZA del tablero (board->first_row).
    // TODO: Por cada celda destruida, invocar a gameBoardAddParticle().
    printf("Función gameBoardCheckAndClearLines no implementada.\n");
    return 0;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board == NULL) return;
    PieceQueueNode* node = malloc(sizeof(PieceQueueNode));
    if (node == NULL) return;   // sin memoria: la pieza se descarta
    node->type = type;
    node->next = NULL;          // el nuevo nodo es el ultimo de la cola

    // Gracias a queue_tail se encola en O(1) sin recorrer la lista.
    // Si la cola estaba vacia, el nodo es a la vez cabeza y cola.
    if (board->queue_tail == NULL) {
        board->queue_head = node;
    } else {
        board->queue_tail->next = node;
    }
    board->queue_tail = node;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board == NULL || board->queue_head == NULL) return 0;

    PieceQueueNode* node = board->queue_head;
    *out_type = node->type;
    board->queue_head = node->next;
    // Si se saco el ultimo nodo, queue_tail apuntaria a memoria liberada:
    // hay que dejarlo en NULL junto con queue_head.
    if (board->queue_head == NULL) board->queue_tail = NULL;
    free(node);
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========

void testStrings() {

    // TESTS DE strDuplicate

    char* copia = strDuplicate("");
    assert(copia != NULL);
    assert(copia[0] == '\0');
    free(copia);

    copia = strDuplicate("A");
    assert(copia != NULL);
    assert(copia[0] == 'A');
    assert(copia[1] == '\0');
    free(copia);

    char* copiaLarga = strDuplicate("Tecnología");
    assert(copiaLarga != NULL);
    assert(strCompare(copiaLarga, "Tecnología") == 0);
    free(copiaLarga);


    // TESTS DE strCompare

    assert(strCompare("Tigre", "Tigre") == 0);
    assert(strCompare("Gala", "Gato") == 1);
    assert(strCompare("Tigre", "Leon") == -1);
    assert(strCompare("casa", "casas") == 1);
    assert(strCompare("casas", "casa") == -1);
    assert(strCompare("", "") == 0);
    assert(strCompare("", "Fede") == 1);
    assert(strCompare("FedeBanana", "") == -1);
    assert(strCompare("A", "B") == 1);
    assert(strCompare("B", "A") == -1);


    // TEST 1 DE strConcatenate

    char* texto1 = strDuplicate("Ejemplo1");
    char* texto2 = strDuplicate("Ejemplo2");

    assert(texto1 != NULL);
    assert(texto2 != NULL);

    char* resultado = strConcatenate(texto1, texto2);

    assert(resultado != NULL);
    assert(strCompare(resultado, "Ejemplo1Ejemplo2") == 0);

    free(resultado);


    // TEST 2: STRING VACIO

    char* vacio = strDuplicate("");
    char* texto3 = strDuplicate("ABC");

    assert(vacio != NULL);
    assert(texto3 != NULL);

    char* resultadoVacio = strConcatenate(vacio, texto3);

    assert(resultadoVacio != NULL);
    assert(strCompare(resultadoVacio, "ABC") == 0);

    free(resultadoVacio);


    // TEST 3: DOS STRINGS DE 5 CARACTERES

    char* palabra1 = strDuplicate("Perro");
    char* palabra2 = strDuplicate("Feliz");

    assert(palabra1 != NULL);
    assert(palabra2 != NULL);

    char* resultadoFinal = strConcatenate(palabra1, palabra2);

    assert(resultadoFinal != NULL);
    assert(strCompare(resultadoFinal, "PerroFeliz") == 0);

    free(resultadoFinal);


    printf("[OK] strings: strDuplicate, strCompare y strConcatenate\n");
}

void testCola() {
    GameBoard* board = gameBoardNew();
    if (board == NULL) {
        printf("[FAIL] cola: gameBoardNew devolvio NULL\n");
        return;
    }
    int ok = 1;
    int type;

    // 5 push y 5 pop: deben salir en el mismo orden en que entraron (FIFO)
    for (int i = 0; i < 5; i++) gameBoardEnqueuePiece(board, i);
    for (int i = 0; i < 5; i++) {
        if (!gameBoardDequeuePiece(board, &type) || type != i) ok = 0;
    }

    // 6to pop: cola vacia, devuelve 0, no toca out_type y deja head/tail en NULL
    type = -1;
    if (gameBoardDequeuePiece(board, &type) != 0 || type != -1) ok = 0;
    if (board->queue_head != NULL || board->queue_tail != NULL) ok = 0;

    // La cola sigue usable despues de vaciarse
    gameBoardEnqueuePiece(board, 6);
    if (!gameBoardDequeuePiece(board, &type) || type != 6) ok = 0;
    if (board->queue_head != NULL || board->queue_tail != NULL) ok = 0;

    printf("[%s] cola: 5 push, 6 pop y push posterior\n", ok ? "OK" : "FAIL");

    // Se dejan piezas encoladas para que gameBoardDelete tambien libere la cola
    gameBoardEnqueuePiece(board, 1);
    gameBoardEnqueuePiece(board, 2);
    gameBoardDelete(board);
}

void correrTests() {
    // TODO: Implementar los casos minimos del enunciado. Por ejemplo:
    //  - strDuplicate: "", "x", un string con distintos caracteres.
    //  - strCompare: "" vs "", "a" vs "a", y una desigualdad en ambos sentidos (s1/s2 y s2/s1).
    //  - strConcatenate: "" + "abc" y "hola " + "mundo". Los argumentos deben crearse con
    //    strDuplicate, porque strConcatenate los libera (no pasar literales).
    //  - Cola: 5 push y 6 pop; el 6to debe devolver 0 y dejar queue_head y queue_tail en NULL.
    //    Verificar tambien que un push posterior funcione.
    //  - Tablero: llenar la fila de indice 19 (la de abajo), llamar a
    //    gameBoardCheckAndClearLines y comprobar que first_row es la fila nueva vacia y que
    //    la lista sigue teniendo 20 filas.
    // Liberar toda la memoria pedida en los tests (strings y tableros).
    testStrings();
    testCola();
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
