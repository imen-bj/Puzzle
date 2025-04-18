#ifndef HEADER_H
#define HEADER_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_mixer.h>

// Constants
#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 900
#define PUZZLE_WIDTH 540
#define PUZZLE_HEIGHT 540
#define PIECE_WIDTH 180
#define PIECE_HEIGHT 180
#define NUM_PIECES 9
#define NUM_PUZZLES 1
#define PUZZLE_X (SCREEN_WIDTH - PUZZLE_WIDTH) / 2  // 450
#define PUZZLE_Y (SCREEN_HEIGHT - PUZZLE_HEIGHT) / 2  // 180 (centered vertically)
#define WELCOME_WIDTH 1000
#define WELCOME_HEIGHT 750

typedef struct {
    SDL_Surface *image;
    SDL_Rect position;
    SDL_Rect target;
    int isPlaced;
} Piece;

typedef struct {
    char *full_image_path;
    char *piece_paths[NUM_PIECES];
    Piece pieces[NUM_PIECES];
} Puzzle;

// External declarations
extern SDL_Surface *completePuzzleImage;
extern Mix_Chunk *successSound;
extern Mix_Chunk *failureSound;
extern Mix_Chunk *clickSound;
extern Mix_Music *backgroundMusic;
extern Puzzle puzzles[NUM_PUZZLES];
extern int puzzle_order[NUM_PUZZLES];
extern int current_puzzle_index;

// Function declarations
SDL_Surface* chargerImage(const char *chemin);
void init_puzzle_order(void);
void load_puzzle(Puzzle *puzzle, SDL_Surface **completeImage);
void afficherImage(SDL_Surface *screen, SDL_Surface *image, SDL_Rect position);
void showResultPopup(SDL_Surface *parentScreen, int isSuccess, int elapsedTime);
void draw_piece_highlight(SDL_Surface *screen, SDL_Rect *position);
int check_puzzle_completion(Puzzle *puzzle);
void snap_piece(Piece *piece);
int show_welcome_screen(SDL_Surface *screen);
int show_rules_screen(SDL_Surface *screen);

#endif
