#include <stdio.h>
#include <stdlib.h>
#include "header1.h"

// Global variables for game progression
int currentLevel = 1;
int totalScore = 0;
int lastRoundScore = 0;

int main(int argc, char *argv[]) {
    const int screenWidth = 1440;
    const int screenHeight = 900;

    // Declare button surfaces at the top so they are in scope for cleanup
    SDL_Surface *restartButtonNormal = NULL;
    SDL_Surface *restartButtonHover = NULL;
    SDL_Surface *finishButtonNormal = NULL;
    SDL_Surface *finishButtonHover = NULL;

    // Initialize SDL, SDL_mixer, and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Erreur d'initialisation de SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize: %s\n", Mix_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "Erreur d'initialisation de SDL_ttf: %s\n", TTF_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Load font
    TTF_Font* font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Load audio resources
    successSound = Mix_LoadWAV("assets/explo.wav");
    if (!successSound) {
        fprintf(stderr, "Failed to load success sound: %s\n", Mix_GetError());
    }
    failureSound = Mix_LoadWAV("assets/fail.wav");
    if (!failureSound) {
        fprintf(stderr, "Failed to load failure sound: %s\n", Mix_GetError());
    }
    clickSound = Mix_LoadWAV("assets/clic.wav");
    if (!clickSound) {
        fprintf(stderr, "Failed to load click sound: %s\n", Mix_GetError());
    }
    backgroundMusic = Mix_LoadMUS("assets/background.wav");
    if (!backgroundMusic) {
        fprintf(stderr, "Failed to load background music: %s\n", Mix_GetError());
    }
    if (backgroundMusic) {
        Mix_PlayMusic(backgroundMusic, -1);
    }

    // Load countdown sounds
    Mix_Chunk* countdownSounds[11];
    const char* countdownFiles[11] = {
        "assets/ten.wav", "assets/nine.wav", "assets/eight.wav", "assets/seven.wav",
        "assets/six.wav", "assets/five.wav", "assets/four.wav", "assets/three.wav",
        "assets/two.wav", "assets/one.wav", "assets/zero.wav"
    };
    for (int i = 0; i < 11; i++) {
        countdownSounds[i] = Mix_LoadWAV(countdownFiles[i]);
        if (!countdownSounds[i]) {
            fprintf(stderr, "Failed to load countdown sound %s: %s\n", countdownFiles[i], Mix_GetError());
        }
    }

    // Create welcome screen
    SDL_Surface *welcomeScreen = SDL_SetVideoMode(WELCOME_WIDTH, WELCOME_HEIGHT, 32, SDL_HWSURFACE);
    if (!welcomeScreen) {
        fprintf(stderr, "Erreur de création de la fenêtre de bienvenue: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialize puzzle order before showing welcome screen
    init_puzzle_order();

    // Handle welcome and rules screens
    int showWelcome = 1;
    int showRules = 0;
    while (showWelcome) {
        int welcomeResult = show_welcome_screen(welcomeScreen);
        if (welcomeResult == 0) {
            TTF_CloseFont(font);
            Mix_CloseAudio();
            TTF_Quit();
            SDL_Quit();
            return EXIT_FAILURE;
        }
        showWelcome = 0;
        showRules = 1;

        while (showRules) {
            int rulesResult = show_rules_screen(welcomeScreen);
            if (rulesResult == 0) {
                TTF_CloseFont(font);
                Mix_CloseAudio();
                TTF_Quit();
                SDL_Quit();
                return EXIT_FAILURE;
            } else if (rulesResult == 2) {
                showWelcome = 1;
                showRules = 0;
            } else {
                showRules = 0;
            }
        }
    }

    // Free welcome screen and create main game screen
    SDL_FreeSurface(welcomeScreen);
    welcomeScreen = NULL;

    SDL_Surface *screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen) {
        fprintf(stderr, "Erreur de création de la fenêtre: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_WM_SetCaption("Puzzle Game", NULL);

    // Display preview screen
    SDL_Surface *bgImage = chargerImage("assets/background.png");
    SDL_Surface *scaled_bg = NULL;
    if (bgImage) {
        double scaleX = (double)screenWidth / bgImage->w;
        double scaleY = (double)screenHeight / bgImage->h;
        scaled_bg = rotozoomSurfaceXY(bgImage, 0.0, scaleX, scaleY, 0);
        SDL_FreeSurface(bgImage);
    }
    
    if (scaled_bg) {
        SDL_BlitSurface(scaled_bg, NULL, screen, NULL);
    } else {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 40, 0, 0)); // Dark red background
    }

    TTF_Font* puzzleFont = TTF_OpenFont("assets/arial.ttf", 48);
    if (puzzleFont) {
        SDL_Color textColor = {255, 248, 220}; // Cornsilk white for text
        SDL_Surface* puzzleText = TTF_RenderText_Blended(puzzleFont, "Good luck!", textColor);
        if (puzzleText) {
            SDL_Surface* previewImage = chargerImage(puzzles[puzzle_order[0]].full_image_path);
            if (previewImage) {
                SDL_Rect messagePos = {
                    (screenWidth - puzzleText->w) / 2,
                    50,
                    puzzleText->w,
                    puzzleText->h
                };
                SDL_BlitSurface(puzzleText, NULL, screen, &messagePos);
                SDL_Rect imagePos = {
                    PUZZLE_X,
                    PUZZLE_Y,
                    puzzles[puzzle_order[0]].grid_cols * PIECE_WIDTH,
                    puzzles[puzzle_order[0]].grid_rows * PIECE_HEIGHT
                };
                SDL_BlitSurface(previewImage, NULL, screen, &imagePos);
                SDL_Flip(screen);
                SDL_Delay(2000);
                SDL_FreeSurface(previewImage);
            }
            SDL_FreeSurface(puzzleText);
        }
        TTF_CloseFont(puzzleFont);
    }
    
    if (scaled_bg) {
        SDL_FreeSurface(scaled_bg);
    }

    // Main puzzle loop for all levels
    while (currentLevel <= 3) {
        // Initialize puzzle
        SDL_Surface *completePuzzleImage = NULL;
        Puzzle currentPuzzle = {0};
        load_puzzle(&currentPuzzle, &completePuzzleImage);
        if (!completePuzzleImage) {
            fprintf(stderr, "Failed to initialize puzzle\n");
            TTF_CloseFont(font);
            Mix_CloseAudio();
            TTF_Quit();
            SDL_Quit();
            return EXIT_FAILURE;
        }

        // Load button images instead of creating surfaces
        restartButtonNormal = chargerImage("assets/restart.png");
        restartButtonHover = chargerImage("assets/restartH.png");
        finishButtonNormal = chargerImage("assets/finish.png");
        finishButtonHover = chargerImage("assets/finishH.png");
        
        if (!restartButtonNormal || !restartButtonHover || !finishButtonNormal || !finishButtonHover) {
            fprintf(stderr, "Failed to load button images\n");
            TTF_CloseFont(font);
            Mix_CloseAudio();
            TTF_Quit();
            SDL_Quit();
            return EXIT_FAILURE;
        }

        // Load background image for main game
        bgImage = chargerImage("assets/background.png");
        scaled_bg = NULL;
        if (bgImage) {
            double scaleX = (double)screenWidth / bgImage->w;
            double scaleY = (double)screenHeight / bgImage->h;
            scaled_bg = rotozoomSurfaceXY(bgImage, 0.0, scaleX, scaleY, 0);
            SDL_FreeSurface(bgImage);
        }

        int pieceSelectionnee = -1;
        int mouseOffsetX = 0;
        int mouseOffsetY = 0;
        int continuer = 1;
        SDL_Event event;
        Uint32 startTime = SDL_GetTicks() / 1000;
        int mouseX = 0, mouseY = 0;
        int restartHovered = 0, finishHovered = 0;
        SDL_Rect barPos = {20, 100, 0, 0};
        int lastCountdownSecond = -1;

        while (continuer) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT:
                        continuer = 0;
                        break;

                    case SDL_MOUSEBUTTONDOWN:
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            mouseX = event.button.x;
                            mouseY = event.button.y;
                            fprintf(stderr, "Mouse down at x=%d, y=%d\n", mouseX, mouseY);
                            if (mouseX >= 50 && mouseX <= 50 + 180 && mouseY >= 800 && mouseY <= 800 + 80) {
                                fprintf(stderr, "Restart button clicked\n");
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                restart_puzzle(&currentPuzzle);
                                startTime = SDL_GetTicks() / 1000;
                                lastCountdownSecond = -1;
                                continue;
                            }
                            if (mouseX >= 1240 && mouseX <= 1240 + 180 && mouseY >= 800 && mouseY <= 800 + 80) {
                                fprintf(stderr, "Finish button clicked\n");
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                int elapsedTime = (SDL_GetTicks() / 1000) - startTime;
                                
                                if (check_puzzle_completion(&currentPuzzle)) {
                                    SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                    if (newWindow) {
                                        showResultPopup(newWindow, 1, elapsedTime);
                                        SDL_FreeSurface(newWindow);
                                        continuer = 0;
                                    }
                                } else {
                                    SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                    if (newWindow) {
                                        showResultPopup(newWindow, 0, 0);
                                        SDL_FreeSurface(newWindow);
                                        
                                        if (hasUsedContinue) {
                                            // Create new screen surface
                                            screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
                                            if (screen) {
                                                // Draw initial background
                                                if (scaled_bg) {
                                                    SDL_BlitSurface(scaled_bg, NULL, screen, NULL);
                                                } else {
                                                    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 40, 0, 0));
                                                }
                                                SDL_Flip(screen);
                                                
                                                // Load new puzzle pieces and reset game state
                                                load_puzzle(&currentPuzzle, &completePuzzleImage);
                                                startTime = SDL_GetTicks() / 1000;
                                                lastCountdownSecond = -1;
                                                pieceSelectionnee = -1;
                                            } else {
                                                fprintf(stderr, "Failed to recreate game screen\n");
                                                continuer = 0;
                                            }
                                        } else {
                                            continuer = 0;
                                        }
                                    }
                                }
                                continue;
                            }
                            for (int i = currentPuzzle.num_pieces - 1; i >= 0; i--) {
                                if (currentPuzzle.pieces[i].image) {
                                    SDL_Rect *pos = &currentPuzzle.pieces[i].position;
                                    if (mouseX >= pos->x &&
                                        mouseX <= pos->x + pos->w &&
                                        mouseY >= pos->y &&
                                        mouseY <= pos->y + pos->h) {
                                        pieceSelectionnee = i;
                                        mouseOffsetX = mouseX - pos->x;
                                        mouseOffsetY = mouseY - pos->y;
                                        fprintf(stderr, "Selected piece %d at (%d, %d)\n", i, mouseX, mouseY);
                                        if (clickSound) {
                                            Mix_PlayChannel(-1, clickSound, 0);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        break;

                    case SDL_MOUSEBUTTONUP:
                        if (event.button.button == SDL_BUTTON_LEFT && pieceSelectionnee != -1) {
                            fprintf(stderr, "Mouse up, piece %d dropped at (%d, %d)\n", pieceSelectionnee,
                                    currentPuzzle.pieces[pieceSelectionnee].position.x,
                                    currentPuzzle.pieces[pieceSelectionnee].position.y);
                            pieceSelectionnee = -1;
                        }
                        break;

                    case SDL_MOUSEMOTION:
                        mouseX = event.motion.x;
                        mouseY = event.motion.y;
                        fprintf(stderr, "Mouse motion: x=%d, y=%d\n", mouseX, mouseY);
                        restartHovered = (mouseX >= 50 && mouseX <= 50 + 180 && mouseY >= 800 && mouseY <= 800 + 80);
                        finishHovered = (mouseX >= 1240 && mouseX <= 1240 + 180 && mouseY >= 800 && mouseY <= 800 + 80);
                        if (pieceSelectionnee != -1) {
                            currentPuzzle.pieces[pieceSelectionnee].position.x = mouseX - mouseOffsetX;
                            currentPuzzle.pieces[pieceSelectionnee].position.y = mouseY - mouseOffsetY;
                        }
                        break;

                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                            fprintf(stderr, "Enter pressed, checking completion...\n");
                            int elapsedTime = (SDL_GetTicks() / 1000) - startTime;
                            
                            if (check_puzzle_completion(&currentPuzzle)) {
                                SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                if (newWindow) {
                                    showResultPopup(newWindow, 1, elapsedTime);
                                    SDL_FreeSurface(newWindow);
                                    
                                    // Restore game screen
                                    screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
                                    if (!screen) {
                                        fprintf(stderr, "Failed to restore game screen\n");
                                        continuer = 0;
                                    }
                                    continuer = 0;
                                }
                            } else {
                                SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                if (newWindow) {
                                    showResultPopup(newWindow, 0, 0);
                                    SDL_FreeSurface(newWindow);
                                    
                                    // Restore game screen if continuing
                                    if (hasUsedContinue) {
                                        screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
                                        if (screen) {
                                            // Draw initial background
                                            if (scaled_bg) {
                                                SDL_BlitSurface(scaled_bg, NULL, screen, NULL);
                                            } else {
                                                SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 40, 0, 0));
                                            }
                                            SDL_Flip(screen);
                                            
                                            // Load new puzzle pieces and reset game state
                                            load_puzzle(&currentPuzzle, &completePuzzleImage);
                                            startTime = SDL_GetTicks() / 1000;
                                            lastCountdownSecond = -1;
                                            pieceSelectionnee = -1;
                                        } else {
                                            fprintf(stderr, "Failed to restore game screen\n");
                                            continuer = 0;
                                        }
                                    } else {
                                        continuer = 0;
                                    }
                                }
                            }
                        }
                        break;
                }
            }

            if (!screen) {
                fprintf(stderr, "Screen surface lost\n");
                break;
            }

            // Draw background first
            if (scaled_bg) {
                SDL_BlitSurface(scaled_bg, NULL, screen, NULL);
            } else {
                SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 40, 0, 0)); // Dark red background
            }

            // Draw puzzle pieces
            for (int i = 0; i < currentPuzzle.num_pieces; i++) {
                if (i != pieceSelectionnee && currentPuzzle.pieces[i].image) {
                    afficherImage(screen, currentPuzzle.pieces[i].image, currentPuzzle.pieces[i].position);
                }
            }

            if (pieceSelectionnee != -1 && currentPuzzle.pieces[pieceSelectionnee].image) {
                afficherImage(screen, currentPuzzle.pieces[pieceSelectionnee].image, currentPuzzle.pieces[pieceSelectionnee].position);
            }

            // Handle countdown sounds
            int elapsedTime = (SDL_GetTicks() / 1000) - startTime;
            if (elapsedTime >= 80 && elapsedTime <= 90) {
                int countdown = 90 - elapsedTime;
                int currentSecond = elapsedTime - 80;
                if (currentSecond != lastCountdownSecond) {
                    int soundIndex = 10 - countdown;
                    if (countdownSounds[soundIndex]) {
                        Mix_PlayChannel(-1, countdownSounds[soundIndex], 0);
                        fprintf(stderr, "Playing countdown sound %d at elapsedTime=%d\n", countdown, elapsedTime);
                    }
                    lastCountdownSecond = currentSecond;
                }
            }

            // Check for time-out
            int score = calculate_score(elapsedTime);
            if (score <= 100) {
                if (screen) {
                    SDL_FreeSurface(screen);
                    screen = NULL;
                }
                SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                if (newWindow) {
                    showResultPopup(newWindow, 0, 0);
                    SDL_FreeSurface(newWindow);
                    continuer = 0;
                }
            }

            if (!screen) {
                continue;
            }

            // Display timer
            char timerText[20];
            snprintf(timerText, sizeof(timerText), "Time: %ds", elapsedTime);
            SDL_Color textColor = {255, 248, 220}; // Cornsilk white for better contrast
            SDL_Surface *timerSurface = TTF_RenderText_Blended(font, timerText, textColor);
            if (timerSurface) {
                SDL_Rect timerPos = {20, 20, timerSurface->w, timerSurface->h};
                SDL_BlitSurface(timerSurface, NULL, screen, &timerPos);
                SDL_FreeSurface(timerSurface);
            }

            // Display score
            char scoreText[20];
            snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
            SDL_Surface *scoreSurface = TTF_RenderText_Blended(font, scoreText, textColor);
            if (scoreSurface) {
                SDL_Rect scorePos = {20, 60, scoreSurface->w, scoreSurface->h};
                SDL_BlitSurface(scoreSurface, NULL, screen, &scorePos);
                SDL_FreeSurface(scoreSurface);
            }

            // Display timer bar and buttons
            displayTimerBar(screen, elapsedTime, &barPos);
            SDL_Rect restartButtonPos = {50, 800, 180, 80};  // Updated dimensions
            SDL_Rect finishButtonPos = {1240, 800, 180, 80}; // Updated dimensions
            SDL_BlitSurface(restartHovered ? restartButtonHover : restartButtonNormal, NULL, screen, &restartButtonPos);
            SDL_BlitSurface(finishHovered ? finishButtonHover : finishButtonNormal, NULL, screen, &finishButtonPos);

            SDL_Flip(screen);
            SDL_Delay(16);
        }

        // Cleanup puzzle resources for this round
        for (int i = 0; i < MAX_PIECES; i++) {
            if (currentPuzzle.pieces[i].image) {
                SDL_FreeSurface(currentPuzzle.pieces[i].image);
                currentPuzzle.pieces[i].image = NULL;
            }
        }
        if (completePuzzleImage) {
            SDL_FreeSurface(completePuzzleImage);
            completePuzzleImage = NULL;
        }

        // Restore main game window size for the next puzzle
        screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
        if (!screen) {
            fprintf(stderr, "Failed to restore main game window after popup.\n");
            break;
        }
        // If currentLevel was incremented in showResultPopup, the loop continues to next puzzle
        // If user chose to quit, showResultPopup will call exit(0)
    }

    // Cleanup
    if (screen) {
        SDL_FreeSurface(screen);
        screen = NULL;
    }
    if (restartButtonNormal) {
        SDL_FreeSurface(restartButtonNormal);
        restartButtonNormal = NULL;
    }
    if (restartButtonHover) {
        SDL_FreeSurface(restartButtonHover);
        restartButtonHover = NULL;
    }
    if (finishButtonNormal) {
        SDL_FreeSurface(finishButtonNormal);
        finishButtonNormal = NULL;
    }
    if (finishButtonHover) {
        SDL_FreeSurface(finishButtonHover);
        finishButtonHover = NULL;
    }
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }
    if (successSound) {
        Mix_FreeChunk(successSound);
        successSound = NULL;
    }
    if (failureSound) {
        Mix_FreeChunk(failureSound);
        failureSound = NULL;
    }
    if (clickSound) {
        Mix_FreeChunk(clickSound);
        clickSound = NULL;
    }
    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
        backgroundMusic = NULL;
    }
    for (int i = 0; i < 11; i++) {
        if (countdownSounds[i]) {
            Mix_FreeChunk(countdownSounds[i]);
            countdownSounds[i] = NULL;
        }
    }
    if (scaled_bg) {
        SDL_FreeSurface(scaled_bg);
        scaled_bg = NULL;
    }
    cleanup_bar_images(); // Clean up timer bar images
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
