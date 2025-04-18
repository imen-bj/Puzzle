#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "header1.h"

SDL_Surface *completePuzzleImage = NULL;
Mix_Chunk *successSound = NULL;
Mix_Chunk *failureSound = NULL;
Mix_Chunk *clickSound = NULL;
Mix_Music *backgroundMusic = NULL;

Puzzle puzzles[NUM_PUZZLES] = {
    {
        "assets/puzzle2_full.png",
        {
            "assets/puzzle2_piece0.png", "assets/puzzle2_piece1.png", "assets/puzzle2_piece2.png",
            "assets/puzzle2_piece3.png", "assets/puzzle2_piece4.png", "assets/puzzle2_piece5.png",
            "assets/puzzle2_piece6.png", "assets/puzzle2_piece7.png", "assets/puzzle2_piece8.png"
        }
    }
};

int puzzle_order[NUM_PUZZLES];
int current_puzzle_index = -1;

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
    
    // Define piece order: column-major (left to right, top to bottom per column)
    // Use 178-pixel intervals to overlap pieces slightly and eliminate gaps (180 * 0.99)
    int target_x[NUM_PIECES] = {0, 0, 0, 178, 178, 178, 356, 356, 356};
    int target_y[NUM_PIECES] = {0, 178, 356, 0, 178, 356, 0, 178, 356};
    
    for (int i = 0; i < NUM_PIECES; i++) {
        if (puzzle->pieces[i].image) {
            SDL_FreeSurface(puzzle->pieces[i].image);
            puzzle->pieces[i].image = NULL;
        }
        
        puzzle->pieces[i].target.x = PUZZLE_X + target_x[i];
        puzzle->pieces[i].target.y = PUZZLE_Y + target_y[i];
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
        
        if (i == 0) {
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

void afficherImage(SDL_Surface *screen, SDL_Surface *image, SDL_Rect position) {
    if (screen && image) {
        SDL_BlitSurface(image, NULL, screen, &position);
    } else {
        fprintf(stderr, "Cannot blit: screen=%p, image=%p\n", screen, image);
    }
}

void showResultPopup(SDL_Surface *parentScreen, int isSuccess, int elapsedTime) {
    if (!parentScreen) {
        fprintf(stderr, "Invalid parent screen\n");
        return;
    }
    // Center the popup window (800x600) in the screen (1440x900)
    char window_pos[32];
    snprintf(window_pos, sizeof(window_pos), "SDL_VIDEO_WINDOW_POS=%d,%d", 320, 150);
    SDL_putenv(window_pos);
    
    SDL_Surface *popup = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
    if (!popup) {
        fprintf(stderr, "Couldn't create popup window: %s\n", SDL_GetError());
        return;
    }
    SDL_Surface* bgImage = chargerImage(isSuccess ? "assets/popup.jpg" : "assets/failure.jpg");
    if (bgImage) {
        SDL_BlitSurface(bgImage, NULL, popup, NULL);
        SDL_FreeSurface(bgImage);
    } else {
        SDL_FillRect(popup, NULL, SDL_MapRGB(popup->format, 255, 255, 255));
    }
    TTF_Font* popupFont = TTF_OpenFont("assets/arial.ttf", 36);
    if (popupFont) {
        SDL_Color textColor = isSuccess ? (SDL_Color){139, 28, 98} : (SDL_Color){0, 0, 0};
        char message[100];
        if (isSuccess) {
            int score = elapsedTime > 0 ? (1000 - elapsedTime * 10 > 100 ? 1000 - elapsedTime * 10 : 100) : 100;
            snprintf(message, sizeof(message), "Completed in %d seconds! Score: %d", elapsedTime, score);
        } else {
            snprintf(message, sizeof(message), "Incorrect Placement! Try Again!");
        }
        SDL_Surface* text = TTF_RenderText_Blended(popupFont, message, textColor);
        if (text) {
            SDL_Rect textPos = {
                (800 - text->w) / 2,
                (600 - text->h) / 2,
                text->w,
                text->h
            };
            SDL_BlitSurface(text, NULL, popup, &textPos);
            SDL_Flip(popup);
            SDL_FreeSurface(text);
        }
        TTF_CloseFont(popupFont);
    }
    if (isSuccess && successSound) {
        Mix_PlayChannel(-1, successSound, 0);
    } else if (!isSuccess && failureSound) {
        Mix_PlayChannel(-1, failureSound, 0);
    }
    SDL_Delay(isSuccess ? 3000 : 2000);
    SDL_Flip(popup);
}

void draw_piece_highlight(SDL_Surface *screen, SDL_Rect *position) {
    if (screen && position) {
        SDL_Rect outline = {position->x - 2, position->y - 2, position->w + 4, position->h + 4};
        SDL_FillRect(screen, &outline, SDL_MapRGB(screen->format, 0, 255, 0));
    }
}

int check_puzzle_completion(Puzzle *puzzle) {
    if (!puzzle) return 0;
    
    int ref_x = puzzle->pieces[0].position.x;
    int ref_y = puzzle->pieces[0].position.y;
    
    // Column-major order: left column (0,1,2), middle (3,4,5), right (6,7,8)
    // Adjusted to 178-pixel intervals to match target positions
    int expected_x[NUM_PIECES] = {0, 0, 0, 178, 178, 178, 356, 356, 356};
    int expected_y[NUM_PIECES] = {0, 178, 356, 0, 178, 356, 0, 178, 356};
    int tolerance = 50;
    
    int isComplete = 1;
    for (int i = 0; i < NUM_PIECES; i++) {
        int expected_pos_x = ref_x + expected_x[i];
        int expected_pos_y = ref_y + expected_y[i];
        int xDiff = abs(puzzle->pieces[i].position.x - expected_pos_x);
        int yDiff = abs(puzzle->pieces[i].position.y - expected_pos_y);
        fprintf(stderr, "Piece %d: pos=(%d,%d), expected=(%d,%d), xDiff=%d, yDiff=%d\n",
                i, puzzle->pieces[i].position.x, puzzle->pieces[i].position.y,
                expected_pos_x, expected_pos_y, xDiff, yDiff);
        if (xDiff > tolerance || yDiff > tolerance) {
            isComplete = 0;
        }
    }
    
    if (isComplete) {
        for (int i = 0; i < NUM_PIECES; i++) {
            puzzle->pieces[i].isPlaced = 1;
        }
        return 1;
    }
    return 0;
}

void snap_piece(Piece *piece) {
    if (!piece) return;
    int tolerance = 30;
    int xDiff = abs(piece->position.x - piece->target.x);
    int yDiff = abs(piece->position.y - piece->target.y);
    if (xDiff < tolerance && yDiff < tolerance) {
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
        if (!scaled_bg) {
            fprintf(stderr, "Failed to scale welcome background: %s\n", SDL_GetError());
        }
    } else {
        fprintf(stderr, "Failed to load welcome background: assets/background.png\n");
    }
    if (!scaled_bg) {
        scaled_bg = SDL_CreateRGBSurface(SDL_SWSURFACE, WELCOME_WIDTH, WELCOME_HEIGHT, 32, 0, 0, 0, 0);
        SDL_FillRect(scaled_bg, NULL, SDL_MapRGB(screen->format, 255, 255, 255));
    }
    
    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        SDL_FreeSurface(scaled_bg);
        return 0;
    }
    
    // Next button
    SDL_Surface *nextButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *nextButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_FillRect(nextButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(nextButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));
    
    // Back button
    SDL_Surface *backButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *backButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_FillRect(backButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(backButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));
    
    SDL_Color textColor = {255, 255, 255};
    SDL_Surface *nextButtonText = TTF_RenderText_Blended(font, "Next", textColor);
    SDL_Surface *backButtonText = TTF_RenderText_Blended(font, "Back", textColor);
    SDL_Rect nextTextPos = {10, 10, nextButtonText->w, nextButtonText->h};
    SDL_Rect backTextPos = {10, 10, backButtonText->w, backButtonText->h};
    SDL_BlitSurface(nextButtonText, NULL, nextButtonNormal, &nextTextPos);
    SDL_BlitSurface(nextButtonText, NULL, nextButtonHover, &nextTextPos);
    SDL_BlitSurface(backButtonText, NULL, backButtonNormal, &backTextPos);
    SDL_BlitSurface(backButtonText, NULL, backButtonHover, &backTextPos);
    
    SDL_Color msgColor = {255, 255, 255};
    const char *message = "You're about to solve a fun puzzle! Fit the pieces together to complete the image.";
    SDL_Surface *text = TTF_RenderText_Blended(font, message, msgColor);
    
    int continuer = 1;
    SDL_Event event;
    int mouseX = 0, mouseY = 0;
    int nextButtonHovered = 0, backButtonHovered = 0;
    
    while (continuer) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    SDL_FreeSurface(scaled_bg);
                    SDL_FreeSurface(nextButtonNormal);
                    SDL_FreeSurface(nextButtonHover);
                    SDL_FreeSurface(backButtonNormal);
                    SDL_FreeSurface(backButtonHover);
                    SDL_FreeSurface(nextButtonText);
                    SDL_FreeSurface(backButtonText);
                    SDL_FreeSurface(text);
                    TTF_CloseFont(font);
                    return 0;
                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    nextButtonHovered = (mouseX >= 525 && mouseX <= 675 && mouseY >= 600 && mouseY <= 650);
                    backButtonHovered = (mouseX >= 325 && mouseX <= 475 && mouseY >= 600 && mouseY <= 650);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (mouseX >= 525 && mouseX <= 675 && mouseY >= 600 && mouseY <= 650) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            continuer = 0;
                        } else if (mouseX >= 325 && mouseX <= 475 && mouseY >= 600 && mouseY <= 650) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            SDL_FreeSurface(scaled_bg);
                            SDL_FreeSurface(nextButtonNormal);
                            SDL_FreeSurface(nextButtonHover);
                            SDL_FreeSurface(backButtonNormal);
                            SDL_FreeSurface(backButtonHover);
                            SDL_FreeSurface(nextButtonText);
                            SDL_FreeSurface(backButtonText);
                            SDL_FreeSurface(text);
                            TTF_CloseFont(font);
                            return 0; // Exit on Back button
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
        SDL_Rect nextButtonPos = {525, 600, 150, 50}; // Moved to the right
        SDL_Rect backButtonPos = {325, 600, 150, 50};
        SDL_BlitSurface(nextButtonHovered ? nextButtonHover : nextButtonNormal, NULL, screen, &nextButtonPos);
        SDL_BlitSurface(backButtonHovered ? backButtonHover : backButtonNormal, NULL, screen, &backButtonPos);
        SDL_Flip(screen);
    }
    
    SDL_FreeSurface(scaled_bg);
    SDL_FreeSurface(nextButtonNormal);
    SDL_FreeSurface(nextButtonHover);
    SDL_FreeSurface(backButtonNormal);
    SDL_FreeSurface(backButtonHover);
    SDL_FreeSurface(nextButtonText);
    SDL_FreeSurface(backButtonText);
    SDL_FreeSurface(text);
    TTF_CloseFont(font);
    return 1;
}

int show_rules_screen(SDL_Surface *screen) {
    SDL_Surface *bg = chargerImage("assets/background.png");
    SDL_Surface *scaled_bg = NULL;
    if (bg) {
        printf("Rules background loaded: w=%d, h=%d\n", bg->w, bg->h);
        double scaleX = (double)WELCOME_WIDTH / bg->w;
        double scaleY = (double)WELCOME_HEIGHT / bg->h;
        scaled_bg = rotozoomSurfaceXY(bg, 0.0, scaleX, scaleY, 0);
        SDL_FreeSurface(bg);
        if (!scaled_bg) {
            fprintf(stderr, "Failed to scale rules background: %s\n", SDL_GetError());
        }
    } else {
        fprintf(stderr, "Failed to load rules background: assets/background.png\n");
    }
    if (!scaled_bg) {
        scaled_bg = SDL_CreateRGBSurface(SDL_SWSURFACE, WELCOME_WIDTH, WELCOME_HEIGHT, 32, 0, 0, 0, 0);
        SDL_FillRect(scaled_bg, NULL, SDL_MapRGB(screen->format, 255, 255, 255));
    }
    
    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        SDL_FreeSurface(scaled_bg);
        return 0;
    }
    
    // Play button
    SDL_Surface *playButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *playButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_FillRect(playButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(playButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));
    
    // Back button
    SDL_Surface *backButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *backButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_FillRect(backButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(backButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));
    
    SDL_Color textColor = {255, 255, 255};
    SDL_Surface *playButtonText = TTF_RenderText_Blended(font, "Play", textColor);
    SDL_Surface *backButtonText = TTF_RenderText_Blended(font, "Back", textColor);
    SDL_Rect playTextPos = {10, 10, playButtonText->w, playButtonText->h};
    SDL_Rect backTextPos = {10, 10, backButtonText->w, backButtonText->h};
    SDL_BlitSurface(playButtonText, NULL, playButtonNormal, &playTextPos);
    SDL_BlitSurface(playButtonText, NULL, playButtonHover, &playTextPos);
    SDL_BlitSurface(backButtonText, NULL, backButtonNormal, &backTextPos);
    SDL_BlitSurface(backButtonText, NULL, backButtonHover, &backTextPos);
    
    SDL_Color msgColor = {255, 255, 255};
    const char *message = "The puzzle is timed! Solve it as fast as possible to earn a higher score.";
    SDL_Surface *text = TTF_RenderText_Blended(font, message, msgColor);
    
    int continuer = 1;
    SDL_Event event;
    int mouseX = 0, mouseY = 0;
    int playButtonHovered = 0, backButtonHovered = 0;
    
    while (continuer) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    SDL_FreeSurface(scaled_bg);
                    SDL_FreeSurface(playButtonNormal);
                    SDL_FreeSurface(playButtonHover);
                    SDL_FreeSurface(backButtonNormal);
                    SDL_FreeSurface(backButtonHover);
                    SDL_FreeSurface(playButtonText);
                    SDL_FreeSurface(backButtonText);
                    SDL_FreeSurface(text);
                    TTF_CloseFont(font);
                    return 0;
                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    playButtonHovered = (mouseX >= 525 && mouseX <= 675 && mouseY >= 600 && mouseY <= 650);
                    backButtonHovered = (mouseX >= 325 && mouseX <= 475 && mouseY >= 600 && mouseY <= 650);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (mouseX >= 525 && mouseX <= 675 && mouseY >= 600 && mouseY <= 650) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            continuer = 0;
                        } else if (mouseX >= 325 && mouseX <= 475 && mouseY >= 600 && mouseY <= 650) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            SDL_FreeSurface(scaled_bg);
                            SDL_FreeSurface(playButtonNormal);
                            SDL_FreeSurface(playButtonHover);
                            SDL_FreeSurface(backButtonNormal);
                            SDL_FreeSurface(backButtonHover);
                            SDL_FreeSurface(playButtonText);
                            SDL_FreeSurface(backButtonText);
                            SDL_FreeSurface(text);
                            TTF_CloseFont(font);
                            return 2; // Go back to welcome screen
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
        SDL_Rect playButtonPos = {525, 600, 150, 50}; // Moved to the right
        SDL_Rect backButtonPos = {325, 600, 150, 50};
        SDL_BlitSurface(playButtonHovered ? playButtonHover : playButtonNormal, NULL, screen, &playButtonPos);
        SDL_BlitSurface(backButtonHovered ? backButtonHover : backButtonNormal, NULL, screen, &backButtonPos);
        SDL_Flip(screen);
    }
    
    SDL_FreeSurface(scaled_bg);
    SDL_FreeSurface(playButtonNormal);
    SDL_FreeSurface(playButtonHover);
    SDL_FreeSurface(backButtonNormal);
    SDL_FreeSurface(backButtonHover);
    SDL_FreeSurface(playButtonText);
    SDL_FreeSurface(backButtonText);
    SDL_FreeSurface(text);
    TTF_CloseFont(font);
    return 1;
}
