#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "header1.h"

// Set to 1 to place all pieces at target positions for testing, 0 for normal gameplay
#define TEST_TARGET_POSITIONS 0

// Global variables declarations
SDL_Surface *completePuzzleImage = NULL;
Mix_Chunk *successSound = NULL;
Mix_Chunk *failureSound = NULL;
Mix_Chunk *clickSound = NULL;
Mix_Music *backgroundMusic = NULL;

static SDL_Surface *barImages[NUM_BAR_LEVELS] = {NULL};
static int barInitialized = 0;

// Puzzle configurations
Puzzle puzzles[NUM_PUZZLES] = {
    // Puzzle 1: 2x4 grid, 8 pieces
    {
        "assets/puzzle1_full.png",
        {
            "assets/puzzle1_piece0.png", "assets/puzzle1_piece1.png", "assets/puzzle1_piece2.png",
            "assets/puzzle1_piece3.png", "assets/puzzle1_piece4.png", "assets/puzzle1_piece5.png",
            "assets/puzzle1_piece6.png", "assets/puzzle1_piece7.png", NULL, NULL, NULL, NULL
        },
        {{0}}, // pieces initialized in load_puzzle
        8,    // num_pieces
        2,    // grid_rows
        4     // grid_cols
    },
    // Puzzle 2: 3x3 grid, 9 pieces
    {
        "assets/puzzle2_full.png",
        {
            "assets/puzzle2_piece0.png", "assets/puzzle2_piece1.png", "assets/puzzle2_piece2.png",
            "assets/puzzle2_piece3.png", "assets/puzzle2_piece4.png", "assets/puzzle2_piece5.png",
            "assets/puzzle2_piece6.png", "assets/puzzle2_piece7.png", "assets/puzzle2_piece8.png",
            NULL, NULL, NULL
        },
        {{0}},
        9,
        3,
        3
    },
    // Puzzle 3: 4x3 grid, 12 pieces
    {
        "assets/puzzle3_full.png",
        {
            "assets/puzzle3_piece0.png", "assets/puzzle3_piece1.png", "assets/puzzle3_piece2.png",
            "assets/puzzle3_piece3.png", "assets/puzzle3_piece4.png", "assets/puzzle3_piece5.png",
            "assets/puzzle3_piece6.png", "assets/puzzle3_piece7.png", "assets/puzzle3_piece8.png",
            "assets/puzzle3_piece9.png", "assets/puzzle3_piece10.png", "assets/puzzle3_piece11.png"
        },
        {{0}},
        12,
        4,    // grid_rows
        3     // grid_cols
    }
};

int puzzle_order[NUM_PUZZLES];
int current_puzzle_index = -1;
int hasUsedContinue = 0;

SDL_Surface* chargerImage(const char *chemin) {
    SDL_Surface *image = IMG_Load(chemin);
    if (!image) {
        fprintf(stderr, "Erreur de chargement de l'image %s: %s\n", chemin, SDL_GetError());
        return NULL;
    }
    SDL_Surface *optimized = SDL_DisplayFormatAlpha(image);
    SDL_FreeSurface(image);
    if (!optimized) {
        fprintf(stderr, "Erreur d'optimisation de l'image %s: %s\n", chemin, SDL_GetError());
        return NULL;
    }
    return optimized;
}

void init_puzzle_order(void) {
    // Set puzzles in sequential order
    for (int i = 0; i < NUM_PUZZLES; i++) {
        puzzle_order[i] = i;
    }
    current_puzzle_index = 0;
}

void load_puzzle(Puzzle *puzzle, SDL_Surface **completeImage) {
    if (!puzzle || !completeImage) {
        fprintf(stderr, "Invalid puzzle or completeImage pointer\n");
        return;
    }
    if (current_puzzle_index >= NUM_PUZZLES) {
        current_puzzle_index = 0;
    }
    int puzzle_idx = puzzle_order[current_puzzle_index];
    *puzzle = puzzles[puzzle_idx];

    if (*completeImage) {
        SDL_FreeSurface(*completeImage);
        *completeImage = NULL;
    }
    *completeImage = chargerImage(puzzle->full_image_path);
    if (!*completeImage) {
        fprintf(stderr, "Failed to load complete image: %s\n", puzzle->full_image_path);
        return;
    }

    // Define target positions based on puzzle type
    int target_x[MAX_PIECES];
    int target_y[MAX_PIECES];
    if (puzzle->num_pieces == 8) { // Puzzle 1: 2x4
        // Grid: [0][1][2][3]
        //       [7][6][5][4]
        // Target positions based on user's correct placement
        int x[] = {360, 572, 796, 1015, 1010, 792, 592, 358};
        int y[] = {180, 177, 178, 173, 398, 424, 460, 446};
        for (int i = 0; i < 8; i++) {
            target_x[i] = x[i];
            target_y[i] = y[i];
        }
    } else if (puzzle->num_pieces == 9) { // Puzzle 2: 3x3
        // Grid: [0][3][6]
        //       [1][4][7]
        //       [2][5][8]
        int x[] = {0, 0, 0, 180, 180, 180, 360, 360, 360};
        int y[] = {0, 180, 360, 0, 180, 360, 0, 180, 360};
        for (int i = 0; i < 9; i++) {
            target_x[i] = PUZZLE_X + x[i];
            target_y[i] = PUZZLE_Y + y[i];
        }
    } else { // Puzzle 3: 4x3
        // Grid: [0][1][2]
        //       [3][4][5]
        //       [6][7][8]
        //       [9][10][11]
        // Target positions based on user's correct placement
        int x[] = {360, 470, 625, 357, 442, 624, 351, 451, 659, 348, 476, 649};
        int y[] = {180, 178, 178, 294, 287, 319, 467, 435, 435, 621, 599, 597};
        for (int i = 0; i < 12; i++) {
            target_x[i] = x[i];
            target_y[i] = y[i];
        }
    }

    for (int i = 0; i < puzzle->num_pieces; i++) {
        if (puzzle->pieces[i].image) {
            SDL_FreeSurface(puzzle->pieces[i].image);
            puzzle->pieces[i].image = NULL;
        }

        puzzle->pieces[i].target.x = target_x[i];
        puzzle->pieces[i].target.y = target_y[i];
        puzzle->pieces[i].target.w = PIECE_WIDTH;
        puzzle->pieces[i].target.h = PIECE_HEIGHT;

        puzzle->pieces[i].image = chargerImage(puzzle->piece_paths[i]);
        if (!puzzle->pieces[i].image) {
            fprintf(stderr, "Failed to load piece %d: %s\n", i, puzzle->piece_paths[i]);
            puzzle->pieces[i].image = SDL_CreateRGBSurface(SDL_SWSURFACE, PIECE_WIDTH, PIECE_HEIGHT, 32, 0, 0, 0, 0);
            if (puzzle->pieces[i].image) {
                SDL_FillRect(puzzle->pieces[i].image, NULL, SDL_MapRGB(puzzle->pieces[i].image->format, 255, 0, 0));
            }
            continue;
        }
        printf("Piece %d loaded: w=%d, h=%d, target=(%d,%d)\n",
               i, puzzle->pieces[i].image->w, puzzle->pieces[i].image->h,
               puzzle->pieces[i].target.x, puzzle->pieces[i].target.y);

        if (TEST_TARGET_POSITIONS || i == 0) {
            puzzle->pieces[i].position.x = puzzle->pieces[i].target.x;
            puzzle->pieces[i].position.y = puzzle->pieces[i].target.y;
            puzzle->pieces[i].isPlaced = 1;
        } else {
            puzzle->pieces[i].position.x = 100 + (rand() % (SCREEN_WIDTH - PIECE_WIDTH - 100));
            puzzle->pieces[i].position.y = 650 + (rand() % (SCREEN_HEIGHT - PIECE_HEIGHT - 650));
            puzzle->pieces[i].isPlaced = 0;
        }
        puzzle->pieces[i].position.w = PIECE_WIDTH;
        puzzle->pieces[i].position.h = PIECE_HEIGHT;
    }
    current_puzzle_index++;
}

void restart_puzzle(Puzzle *puzzle) {
    if (!puzzle) {
        fprintf(stderr, "Invalid puzzle pointer\n");
        return;
    }

    // Randomize positions for all pieces except piece0
    for (int i = 1; i < puzzle->num_pieces; i++) {
        puzzle->pieces[i].position.x = 100 + (rand() % (SCREEN_WIDTH - PIECE_WIDTH - 100));
        puzzle->pieces[i].position.y = 650 + (rand() % (SCREEN_HEIGHT - PIECE_HEIGHT - 650));
        puzzle->pieces[i].isPlaced = 0;
    }
}

void afficherImage(SDL_Surface *screen, SDL_Surface *image, SDL_Rect position) {
    if (screen && image) {
        SDL_BlitSurface(image, NULL, screen, &position);
    } else {
        fprintf(stderr, "Cannot blit: screen=%p, image=%p\n", screen, image);
    }
}

int calculate_score(int elapsedTime) {
    int baseScore;
    // Level-based scoring
    switch(currentLevel) {
        case 1:
            baseScore = 1000;
            break;
        case 2:
            baseScore = 1500; // Higher base score for level 2
            break;
        case 3:
            baseScore = 2000; // Highest base score for level 3
            break;
        default:
            baseScore = 1000;
    }
    
    // Score decreases faster in higher levels
    int decreaseRate;
    switch(currentLevel) {
        case 1:
            decreaseRate = 150;
            break;
        case 2:
            decreaseRate = 200; // Faster decrease in level 2
            break;
        case 3:
            decreaseRate = 250; // Fastest decrease in level 3
            break;
        default:
            decreaseRate = 150;
    }
    
    int intervals = elapsedTime / 15;
    int score = baseScore - (intervals * decreaseRate);
    return score > 100 ? score : 100;  // Minimum score is always 100
}

int get_allowed_time_for_level(int level) {
    switch(level) {
        case 1: return 90; // 90 seconds for level 1
        case 2: return 75; // 75 seconds for level 2
        case 3: return 60; // 60 seconds for level 3
        default: return 90;
    }
}

void showResultPopup(SDL_Surface *parentScreen, int isSuccess, int elapsedTime) {
    fprintf(stderr, "[DEBUG] Entered showResultPopup: currentLevel=%d, isSuccess=%d, elapsedTime=%d\n", currentLevel, isSuccess, elapsedTime);
    if (!parentScreen) {
        fprintf(stderr, "Invalid parent screen\n");
        return;
    }

    char window_pos[32];
    snprintf(window_pos, sizeof(window_pos), "SDL_VIDEO_WINDOW_POS=%d,%d", 320, 150);
    SDL_putenv(window_pos);

    SDL_Surface *popup = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
    if (!popup) {
        fprintf(stderr, "Couldn't create popup window: %s\n", SDL_GetError());
        return;
    }

    // Create background
    SDL_Surface *darkBg = SDL_CreateRGBSurface(SDL_HWSURFACE, 800, 600, 32, 0, 0, 0, 0);
    if (isSuccess) {
        SDL_FillRect(darkBg, NULL, SDL_MapRGB(darkBg->format, 245, 245, 220));
    } else {
        SDL_FillRect(darkBg, NULL, SDL_MapRGB(darkBg->format, 0, 0, 0));
        // Add stars
        for(int i = 0; i < 150; i++) {
            int x = rand() % 800;
            int y = rand() % 600;
            int size = (rand() % 2) + 1;
            SDL_Rect star = {x, y, size, size};
            Uint32 starColor = (rand() % 4 == 0) ? 
                SDL_MapRGB(darkBg->format, 255, 255, 255) : 
                SDL_MapRGB(darkBg->format, 200, 200, 200);
            SDL_FillRect(darkBg, &star, starColor);
        }
    }

    // Play sound
    if (isSuccess && successSound) {
        Mix_PlayChannel(-1, successSound, 0);
    } else if (!isSuccess && failureSound) {
        Mix_PlayChannel(-1, failureSound, 0);
    }

    // Setup text
    TTF_Font* popupFont = TTF_OpenFont("assets/arial.ttf", 48);
    SDL_Surface *text1 = NULL;
    SDL_Surface *text2 = NULL;
    SDL_Surface *continueText = NULL;
    SDL_Surface *congratsText = NULL;
    
    int bonus = 0;
    if (isSuccess) {
        int allowedTime = get_allowed_time_for_level(currentLevel);
        if (elapsedTime < allowedTime / 2) {
            bonus = 400;
            totalScore += 400;
        }
    }

    if (popupFont) {
        SDL_Color textColor = {139, 0, 0};
        SDL_Color buttonColor = {120, 0, 0};
        SDL_Color congratsColor = {255, 215, 0}; // Gold color
        
        lastRoundScore = isSuccess ? calculate_score(elapsedTime) : 100;
        totalScore += lastRoundScore;

        if (isSuccess) {
            congratsText = TTF_RenderText_Blended(popupFont, "Congratulations!", congratsColor);
        }
        if (currentLevel < 3) {
            if (!isSuccess) {
                text1 = TTF_RenderText_Blended(popupFont, "Game Over", textColor);
            }
            char scoreText[150];
            if (isSuccess && bonus > 0) {
                snprintf(scoreText, sizeof(scoreText), "Round Score: %d | Bonus: +%d | Total: %d", lastRoundScore, bonus, totalScore);
            } else {
                snprintf(scoreText, sizeof(scoreText), "Round Score: %d | Total: %d", lastRoundScore, totalScore);
            }
            text2 = TTF_RenderText_Blended(popupFont, scoreText, textColor);
            continueText = TTF_RenderText_Blended(popupFont, "Continue?", buttonColor);
        } else {
            char scoreText[150];
            if (isSuccess && bonus > 0) {
                snprintf(scoreText, sizeof(scoreText), "Final Score: %d (+%d bonus)", totalScore, bonus);
            } else {
                snprintf(scoreText, sizeof(scoreText), "Final Score: %d", totalScore);
            }
            if (isSuccess) {
                text2 = TTF_RenderText_Blended(popupFont, scoreText, buttonColor);
            } else {
                text1 = TTF_RenderText_Blended(popupFont, "Game Over", textColor);
                text2 = TTF_RenderText_Blended(popupFont, scoreText, textColor);
            }
        }
    }

    // Set duration
    Uint32 startTime = SDL_GetTicks();
    Uint32 duration = (currentLevel == 3) ? 5000 : 0;

    // Button setup
    SDL_Rect yesButtonRect = {250, 450, 100, 50};
    SDL_Rect noButtonRect = {450, 450, 100, 50};
    int mouseX, mouseY;
    int continueChoice = 0;

    // Main loop
    while (duration == 0 || SDL_GetTicks() - startTime < duration) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (currentLevel < 3 && event.type == SDL_MOUSEBUTTONDOWN) {
                mouseX = event.button.x;
                mouseY = event.button.y;
                fprintf(stderr, "[DEBUG] Mouse click at x=%d, y=%d\n", mouseX, mouseY);
                if (mouseX >= yesButtonRect.x && mouseX <= yesButtonRect.x + yesButtonRect.w &&
                    mouseY >= yesButtonRect.y && mouseY <= yesButtonRect.y + yesButtonRect.h) {
                    fprintf(stderr, "[DEBUG] YES button clicked\n");
                    continueChoice = 1;
                    break;
                }
                if (mouseX >= noButtonRect.x && mouseX <= noButtonRect.x + noButtonRect.w &&
                    mouseY >= noButtonRect.y && mouseY <= noButtonRect.y + noButtonRect.h) {
                    fprintf(stderr, "[DEBUG] NO button clicked\n");
                    continueChoice = 2;
                    break;
                }
            }
        }

        if (continueChoice > 0) {
            fprintf(stderr, "[DEBUG] Breaking popup loop, continueChoice=%d\n", continueChoice);
            break;
        }

        // Draw popup
        SDL_BlitSurface(darkBg, NULL, popup, NULL);
        Uint32 elapsed = SDL_GetTicks() - startTime;

        if (isSuccess && congratsText) {
            double congratsScale = 0.7 + 0.1 * sin(elapsed * 0.003);
            double congratsAngle = 8 * sin(elapsed * 0.002);
            SDL_Surface* scaledCongrats = rotozoomSurfaceXY(congratsText, congratsAngle, congratsScale, congratsScale, 1);
            if (scaledCongrats) {
                SDL_Rect congratsPos = { (800 - scaledCongrats->w) / 2, 60, scaledCongrats->w, scaledCongrats->h };
                SDL_BlitSurface(scaledCongrats, NULL, popup, &congratsPos);
                SDL_FreeSurface(scaledCongrats);
            }
        }

        if (currentLevel < 3) {
            // Draw text with animation
            if (text1) {
                double scale = 1.2 + 0.2 * sin(elapsed * 0.003);
                double angle = 15 * sin(elapsed * 0.002);
                SDL_Surface* scaledText = rotozoomSurfaceXY(text1, angle, scale, scale, 1);
                if (scaledText) {
                    SDL_Rect pos = {(800 - scaledText->w) / 2, 120, scaledText->w, scaledText->h};
                    SDL_BlitSurface(scaledText, NULL, popup, &pos);
                    SDL_FreeSurface(scaledText);
                }
            }

            if (text2) {
                SDL_Rect pos = {(800 - text2->w) / 2, 250, text2->w, text2->h};
                SDL_BlitSurface(text2, NULL, popup, &pos);
            }

            if (continueText) {
                double scale = 0.8 + 0.1 * sin(elapsed * 0.003);
                SDL_Surface* scaledContinue = rotozoomSurfaceXY(continueText, 0, scale, scale, 1);
                if (scaledContinue) {
                    SDL_Rect pos = {(800 - scaledContinue->w) / 2, 300, scaledContinue->w, scaledContinue->h};
                    SDL_BlitSurface(scaledContinue, NULL, popup, &pos);
                    SDL_FreeSurface(scaledContinue);
                }
            }

            // Draw buttons
            SDL_Color buttonColor = {120, 0, 0};
            SDL_Surface* yesText = TTF_RenderText_Blended(popupFont, "Yes", buttonColor);
            SDL_Surface* noText = TTF_RenderText_Blended(popupFont, "No", buttonColor);
            
            if (yesText && noText) {
                yesButtonRect.x = (800 / 2) - yesText->w - 50;
                yesButtonRect.y = 400;
                noButtonRect.x = (800 / 2) + 50;
                noButtonRect.y = 400;
                
                SDL_BlitSurface(yesText, NULL, popup, &yesButtonRect);
                SDL_BlitSurface(noText, NULL, popup, &noButtonRect);
                
                SDL_FreeSurface(yesText);
                SDL_FreeSurface(noText);
            }
        } else {
            // Draw puzzle 3 text
            if (text1) {
                double scale = 1.2 + 0.2 * sin(elapsed * 0.003);
                double angle = 15 * sin(elapsed * 0.002);
                SDL_Surface* scaledText = rotozoomSurfaceXY(text1, angle, scale, scale, 1);
                if (scaledText) {
                    SDL_Rect pos = {(800 - scaledText->w) / 2, 120, scaledText->w, scaledText->h};
                    SDL_BlitSurface(scaledText, NULL, popup, &pos);
                    SDL_FreeSurface(scaledText);
                }
            }

            if (text2) {
                SDL_Rect pos = {(800 - text2->w) / 2, 300, text2->w, text2->h};
                SDL_BlitSurface(text2, NULL, popup, &pos);
            }
        }

        SDL_Flip(popup);
        SDL_Delay(16);
    }

    // Cleanup
    if (text1) SDL_FreeSurface(text1);
    if (text2) SDL_FreeSurface(text2);
    if (darkBg) SDL_FreeSurface(darkBg);
    if (continueText) SDL_FreeSurface(continueText);
    if (popupFont) TTF_CloseFont(popupFont);
    if (congratsText) SDL_FreeSurface(congratsText);

    // Handle result
    if (currentLevel < 3) {
        if (continueChoice == 2) {  // No button
            fprintf(stderr, "[DEBUG] Exiting game after NO button, currentLevel=%d\n", currentLevel);
            exit(0);
        } else if (continueChoice == 1) {  // Yes button
            currentLevel++;
            fprintf(stderr, "[DEBUG] YES button: incremented currentLevel to %d, returning to continue game\n", currentLevel);
            return;  // Return to continue the game
        }
        // If success and no explicit choice was made (timeout/close), still continue
        if (isSuccess) {
            currentLevel++;
            fprintf(stderr, "[DEBUG] Success with no explicit choice, incremented currentLevel to %d\n", currentLevel);
        }
    } else {
        fprintf(stderr, "[DEBUG] Puzzle 3: waiting 5 seconds then exiting game\n");
        SDL_Delay(4000);  // Show final score for 5 seconds in puzzle 3
        exit(0);
    }
}

void draw_piece_highlight(SDL_Surface *screen, SDL_Rect *position) {
    if (screen && position) {
        SDL_Rect outline = {position->x - 2, position->y - 2, position->w + 4, position->h + 4};
        SDL_FillRect(screen, &outline, SDL_MapRGB(screen->format, 139, 0, 0)); // Dark red highlight
    }
}

int check_puzzle_completion(Puzzle *puzzle) {
    if (!puzzle) return 0;

    int tolerance = 60; // Tolerance for piece placement
    int isComplete = 1;
    for (int i = 0; i < puzzle->num_pieces; i++) {
        int expected_pos_x = puzzle->pieces[i].target.x;
        int expected_pos_y = puzzle->pieces[i].target.y;
        int xDiff = abs(puzzle->pieces[i].position.x - expected_pos_x);
        int yDiff = abs(puzzle->pieces[i].position.y - expected_pos_y);
        // Calculate grid position for logging
        int grid_row = (expected_pos_y - PUZZLE_Y) / PIECE_HEIGHT;
        int grid_col = (expected_pos_x - PUZZLE_X) / PIECE_WIDTH;
        fprintf(stderr, "Piece %d (grid: row=%d, col=%d): pos=(%d,%d), expected=(%d,%d), xDiff=%d, yDiff=%d\n",
                i, grid_row, grid_col,
                puzzle->pieces[i].position.x, puzzle->pieces[i].position.y,
                expected_pos_x, expected_pos_y, xDiff, yDiff);
        if (xDiff > tolerance || yDiff > tolerance) {
            isComplete = 0;
            fprintf(stderr, "Piece %d is misplaced: xDiff=%d, yDiff=%d\n", i, xDiff, yDiff);
        }
    }

    return isComplete;
}

void snap_piece(Piece *piece) {
    if (!piece) return;
    
    // Adjust snapping tolerance based on level
    int tolerance;
    switch(currentLevel) {
        case 1:
            tolerance = 60;  // Most forgiving
            break;
        case 2:
            tolerance = 50;  // Moderate precision required
            break;
        case 3:
            tolerance = 40;  // Most precise
            break;
        default:
            tolerance = 60;
    }

    // Check if piece is near its target position
    int xDiff = abs(piece->position.x - piece->target.x);
    int yDiff = abs(piece->position.y - piece->target.y);
    
    if (xDiff <= tolerance && yDiff <= tolerance) {
        piece->position.x = piece->target.x;
        piece->position.y = piece->target.y;
        piece->isPlaced = 1;
    }
}

int show_welcome_screen(SDL_Surface *screen) {
    SDL_Surface *bg = chargerImage("assets/background.png");
    SDL_Surface *scaled_bg = NULL;
    if (bg) {
        printf("Welcome background loaded: w=%d, h=%d\n", bg->w, bg->h);
        double scaleX = (double)WELCOME_WIDTH / bg->w;
        double scaleY = (double)WELCOME_HEIGHT / bg->h;
        scaled_bg = rotozoomSurfaceXY(bg, 0.0, scaleX, scaleY, 0);
        SDL_FreeSurface(bg);
    }
    if (!scaled_bg) {
        scaled_bg = SDL_CreateRGBSurface(SDL_SWSURFACE, WELCOME_WIDTH, WELCOME_HEIGHT, 32, 0, 0, 0, 0);
        SDL_FillRect(scaled_bg, NULL, SDL_MapRGB(screen->format, 40, 0, 0)); // Dark red background
    }

    // Load button images
    SDL_Surface *nextButtonNormal = chargerImage("assets/next.png");
    SDL_Surface *nextButtonHover = chargerImage("assets/nextH.png");
    SDL_Surface *backButtonNormal = chargerImage("assets/back.png");
    SDL_Surface *backButtonHover = chargerImage("assets/backH.png");

    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        SDL_FreeSurface(scaled_bg);
        return 0;
    }

    SDL_Color msgColor = {255, 255, 255}; // Pure white color
    const char *message = "Welcome to the Puzzle Game! Press Next to continue.";
    SDL_Surface *text = TTF_RenderText_Blended(font, message, msgColor);

    int continuer = 1;
    SDL_Event event;
    int mouseX = 0, mouseY = 0;
    int nextButtonHovered = 0, backButtonHovered = 0;

    // Define button positions and dimensions
    SDL_Rect nextButtonPos = {525, 600, 180, 80};  // Updated dimensions
    SDL_Rect backButtonPos = {325, 600, 180, 80};  // Updated dimensions

    while (continuer) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    SDL_FreeSurface(scaled_bg);
                    SDL_FreeSurface(nextButtonNormal);
                    SDL_FreeSurface(nextButtonHover);
                    SDL_FreeSurface(backButtonNormal);
                    SDL_FreeSurface(backButtonHover);
                    SDL_FreeSurface(text);
                    TTF_CloseFont(font);
                    return 0;
                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    nextButtonHovered = (mouseX >= nextButtonPos.x && 
                                      mouseX <= nextButtonPos.x + nextButtonPos.w && 
                                      mouseY >= nextButtonPos.y && 
                                      mouseY <= nextButtonPos.y + nextButtonPos.h);
                    backButtonHovered = (mouseX >= backButtonPos.x && 
                                       mouseX <= backButtonPos.x + backButtonPos.w && 
                                       mouseY >= backButtonPos.y && 
                                       mouseY <= backButtonPos.y + backButtonPos.h);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (mouseX >= nextButtonPos.x && mouseX <= nextButtonPos.x + nextButtonPos.w &&
                            mouseY >= nextButtonPos.y && mouseY <= nextButtonPos.y + nextButtonPos.h) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            continuer = 0;
                        } else if (mouseX >= backButtonPos.x && mouseX <= backButtonPos.x + backButtonPos.w &&
                                 mouseY >= backButtonPos.y && mouseY <= backButtonPos.y + backButtonPos.h) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            SDL_FreeSurface(scaled_bg);
                            SDL_FreeSurface(nextButtonNormal);
                            SDL_FreeSurface(nextButtonHover);
                            SDL_FreeSurface(backButtonNormal);
                            SDL_FreeSurface(backButtonHover);
                            SDL_FreeSurface(text);
                            TTF_CloseFont(font);
                            return 0;
                        }
                    }
                    break;
            }
        }

        SDL_BlitSurface(scaled_bg, NULL, screen, NULL);

        if (text) {
            SDL_Rect textPos = {(WELCOME_WIDTH - text->w) / 2, 250, text->w, text->h};
            SDL_BlitSurface(text, NULL, screen, &textPos);
        }

        SDL_BlitSurface(nextButtonHovered ? nextButtonHover : nextButtonNormal, NULL, screen, &nextButtonPos);
        SDL_BlitSurface(backButtonHovered ? backButtonHover : backButtonNormal, NULL, screen, &backButtonPos);
        SDL_Flip(screen);
    }

    SDL_FreeSurface(scaled_bg);
    SDL_FreeSurface(nextButtonNormal);
    SDL_FreeSurface(nextButtonHover);
    SDL_FreeSurface(backButtonNormal);
    SDL_FreeSurface(backButtonHover);
    SDL_FreeSurface(text);
    TTF_CloseFont(font);
    return 1;
}

int show_rules_screen(SDL_Surface *screen) {
    // Load background
    SDL_Surface *bg = chargerImage("assets/background.png");
    SDL_Surface *scaled_bg = NULL;
    if (bg) {
        double scaleX = (double)WELCOME_WIDTH / bg->w;
        double scaleY = (double)WELCOME_HEIGHT / bg->h;
        scaled_bg = rotozoomSurfaceXY(bg, 0.0, scaleX, scaleY, 0);
        SDL_FreeSurface(bg);
    }
    if (!scaled_bg) {
        scaled_bg = SDL_CreateRGBSurface(SDL_SWSURFACE, WELCOME_WIDTH, WELCOME_HEIGHT, 32, 0, 0, 0, 0);
        SDL_FillRect(scaled_bg, NULL, SDL_MapRGB(screen->format, 40, 0, 0)); // Dark red background
    }

    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        SDL_FreeSurface(scaled_bg);
        return 0;
    }

    // Load button images
    SDL_Surface *playButtonNormal = chargerImage("assets/play.png");
    SDL_Surface *playButtonHover = chargerImage("assets/playH.png");
    SDL_Surface *backButtonNormal = chargerImage("assets/back.png");
    SDL_Surface *backButtonHover = chargerImage("assets/backH.png");

    // Render rules text
    SDL_Color msgColor = {255, 215, 0}; // Gold for main rule
    SDL_Color whiteColor = {255, 255, 255};
    SDL_Color accentColor = {220, 50, 50}; // Lighter red for accent
    const char *lines[] = {
        "There are 3 levels: Easy, Medium, and Hard!",
        "After each level, you can choose to continue or quit.",
        "Each level is timed: the timer runs faster as you progress!",
        "Score decreases more quickly in higher levels.",
        "Complete puzzles fast for the best score!",
        "+400 bonus if you finish a level in less than half the time (45s)!"
    };
    SDL_Surface *texts[6];
    texts[0] = TTF_RenderText_Blended(font, lines[0], msgColor);
    texts[1] = TTF_RenderText_Blended(font, lines[1], whiteColor);
    texts[2] = TTF_RenderText_Blended(font, lines[2], accentColor);
    texts[3] = TTF_RenderText_Blended(font, lines[3], whiteColor);
    texts[4] = TTF_RenderText_Blended(font, lines[4], msgColor);
    texts[5] = TTF_RenderText_Blended(font, lines[5], accentColor);

    // Calculate bounding box for all lines
    int maxWidth = 0, totalHeight = 0;
    for (int i = 0; i < 6; i++) {
        if (texts[i]) {
            if (texts[i]->w > maxWidth) maxWidth = texts[i]->w;
            totalHeight += texts[i]->h + 18;
        }
    }
    totalHeight -= 18; // Remove last extra spacing
    int padding = 32;
    int rectW = maxWidth + 2 * padding;
    int rectH = totalHeight + 2 * padding;
    int rectX = (WELCOME_WIDTH - rectW) / 2;
    int rectY = 170 - padding;

    // Create a semi-transparent black rectangle
    SDL_Surface *rectSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, rectW, rectH, 32, 0, 0, 0, 0);
    SDL_FillRect(rectSurf, NULL, SDL_MapRGBA(rectSurf->format, 0, 0, 0, 180)); // 180 alpha for transparency
    SDL_SetAlpha(rectSurf, SDL_SRCALPHA, 180);

    int continuer = 1;
    SDL_Event event;
    int mouseX = 0, mouseY = 0;
    int playButtonHovered = 0, backButtonHovered = 0;

    // Define button positions and dimensions
    SDL_Rect playButtonPos = {525, 600, 180, 80};  // Updated dimensions
    SDL_Rect backButtonPos = {325, 600, 180, 80};  // Updated dimensions

    while (continuer) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    SDL_FreeSurface(scaled_bg);
                    SDL_FreeSurface(playButtonNormal);
                    SDL_FreeSurface(playButtonHover);
                    SDL_FreeSurface(backButtonNormal);
                    SDL_FreeSurface(backButtonHover);
                    for (int i = 0; i < 6; i++) {
                        if (texts[i]) SDL_FreeSurface(texts[i]);
                    }
                    TTF_CloseFont(font);
                    SDL_FreeSurface(rectSurf);
                    return 0;
                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    playButtonHovered = (mouseX >= playButtonPos.x && 
                                      mouseX <= playButtonPos.x + playButtonPos.w && 
                                      mouseY >= playButtonPos.y && 
                                      mouseY <= playButtonPos.y + playButtonPos.h);
                    backButtonHovered = (mouseX >= backButtonPos.x && 
                                       mouseX <= backButtonPos.x + backButtonPos.w && 
                                       mouseY >= backButtonPos.y && 
                                       mouseY <= backButtonPos.y + backButtonPos.h);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (mouseX >= playButtonPos.x && mouseX <= playButtonPos.x + playButtonPos.w &&
                            mouseY >= playButtonPos.y && mouseY <= playButtonPos.y + playButtonPos.h) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            continuer = 0;
                        } else if (mouseX >= backButtonPos.x && mouseX <= backButtonPos.x + backButtonPos.w &&
                                 mouseY >= backButtonPos.y && mouseY <= backButtonPos.y + backButtonPos.h) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            SDL_FreeSurface(scaled_bg);
                            SDL_FreeSurface(playButtonNormal);
                            SDL_FreeSurface(playButtonHover);
                            SDL_FreeSurface(backButtonNormal);
                            SDL_FreeSurface(backButtonHover);
                            for (int i = 0; i < 6; i++) {
                                if (texts[i]) SDL_FreeSurface(texts[i]);
                            }
                            TTF_CloseFont(font);
                            SDL_FreeSurface(rectSurf);
                            return 2;
                        }
                    }
                    break;
            }
        }

        // Draw screen elements
        SDL_BlitSurface(scaled_bg, NULL, screen, NULL);
        // Draw the transparent rectangle first
        SDL_Rect rectPos = {rectX, rectY, rectW, rectH};
        SDL_BlitSurface(rectSurf, NULL, screen, &rectPos);
        // Draw rules lines, spaced vertically
        int y = 180;
        for (int i = 0; i < 6; i++) {
            if (texts[i]) {
                SDL_Rect pos = {(WELCOME_WIDTH - texts[i]->w) / 2, y, texts[i]->w, texts[i]->h};
                SDL_BlitSurface(texts[i], NULL, screen, &pos);
                y += texts[i]->h + 18;
            }
        }
        
        SDL_BlitSurface(playButtonHovered ? playButtonHover : playButtonNormal, NULL, screen, &playButtonPos);
        SDL_BlitSurface(backButtonHovered ? backButtonHover : backButtonNormal, NULL, screen, &backButtonPos);
        
        SDL_Flip(screen);
    }

    // Cleanup
    SDL_FreeSurface(scaled_bg);
    SDL_FreeSurface(playButtonNormal);
    SDL_FreeSurface(playButtonHover);
    SDL_FreeSurface(backButtonNormal);
    SDL_FreeSurface(backButtonHover);
    for (int i = 0; i < 6; i++) {
        if (texts[i]) SDL_FreeSurface(texts[i]);
    }
    TTF_CloseFont(font);
    SDL_FreeSurface(rectSurf);
    return 1;
}

void displayTimerBar(SDL_Surface *screen, int elapsedTime, SDL_Rect *barPos) {
    if (!screen || !barPos) return;

    // Initialize bar images if not already done
    if (!barInitialized) {
        char path[32];
        for (int i = 0; i < NUM_BAR_LEVELS; i++) {
            snprintf(path, sizeof(path), "assets/bar_%d.png", i);
            barImages[i] = chargerImage(path);
            if (!barImages[i]) {
                fprintf(stderr, "Failed to load bar image %s\n", path);
                barImages[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, 200, 20, 32, 0, 0, 0, 0);
                if (barImages[i]) {
                    // Create a gradient from dark red to black based on level
                    int red = 139 - (i * 20); // Start at dark red (139,0,0) and fade to black
                    SDL_FillRect(barImages[i], NULL, SDL_MapRGB(screen->format, red, 0, 0));
                }
            }
        }
        barInitialized = 1;
    }

    // Calculate which bar image to display (update every 15 seconds)
    int level = elapsedTime / 15;
    if (level >= NUM_BAR_LEVELS) {
        level = NUM_BAR_LEVELS - 1;
    }

    // Add a black border around the timer bar
    SDL_Rect border = {barPos->x - 1, barPos->y - 1, 202, 22};
    SDL_FillRect(screen, &border, SDL_MapRGB(screen->format, 0, 0, 0));

    // Display the appropriate bar image
    if (barImages[level]) {
        SDL_BlitSurface(barImages[level], NULL, screen, barPos);
    }
}

void cleanup_bar_images(void) {
    for (int i = 0; i < NUM_BAR_LEVELS; i++) {
        if (barImages[i]) {
            SDL_FreeSurface(barImages[i]);
            barImages[i] = NULL;
        }
    }
    barInitialized = 0;
}
