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
#define MAX_PIECES 12 // Maximum number of pieces (for Puzzle 3)
#define NUM_PUZZLES 3 // Now supporting three puzzles
#define PIECE_WIDTH 180
#define PIECE_HEIGHT 180
#define PUZZLE_X (SCREEN_WIDTH - 720) / 2 // Centered for max width (4 columns = 720)
#define PUZZLE_Y (SCREEN_HEIGHT - 540) / 2 // Centered for max height (3 rows = 540)
#define WELCOME_WIDTH 1000
#define WELCOME_HEIGHT 750
#define PREVIEW_X 50
#define PREVIEW_Y 50
#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 200
#define TOTAL_TIME 60
#define NUM_BAR_LEVELS 6

typedef struct {
    SDL_Surface *image;
    SDL_Rect position;
    SDL_Rect target;
    int isPlaced;
} Piece;

typedef struct {
    char *full_image_path;
    char *piece_paths[MAX_PIECES];
    Piece pieces[MAX_PIECES];
    int num_pieces; // Number of pieces in this puzzle
    int grid_rows; // Number of rows in the grid
    int grid_cols; // Number of columns in the grid
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
extern int hasUsedContinue; // Track if continue has been used

// Game progression tracking
extern int currentLevel;
extern int totalScore;
extern int lastRoundScore;

// Function declarations
SDL_Surface* chargerImage(const char *chemin);
void init_puzzle_order(void);
void load_puzzle(Puzzle *puzzle, SDL_Surface **completeImage);
void restart_puzzle(Puzzle *puzzle); // New function for restarting current puzzle
void afficherImage(SDL_Surface *screen, SDL_Surface *image, SDL_Rect position);
void showResultPopup(SDL_Surface *parentScreen, int isSuccess, int elapsedTime);
void draw_piece_highlight(SDL_Surface *screen, SDL_Rect *position);
int check_puzzle_completion(Puzzle *puzzle);
void snap_piece(Piece *piece);
int show_welcome_screen(SDL_Surface *screen);
int show_rules_screen(SDL_Surface *screen);
int calculate_score(int elapsedTime);
void displayTimerBar(SDL_Surface *screen, int elapsedTime, SDL_Rect *barPos);
void cleanup_bar_images(void);

#endif
