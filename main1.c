#include <stdio.h>
#include <stdlib.h>
#include "header1.h"

int main(int argc, char *argv[]) {
    const int screenWidth = 1440;
    const int screenHeight = 900;
    
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
        SDL_Quit();
        return EXIT_FAILURE;
    }

    TTF_Font* font = TTF_OpenFont("assets/arial.ttf", 36);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

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

    SDL_Surface *welcomeScreen = SDL_SetVideoMode(WELCOME_WIDTH, WELCOME_HEIGHT, 32, SDL_HWSURFACE);
    if (!welcomeScreen) {
        fprintf(stderr, "Erreur de création de la fenêtre de bienvenue: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
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
                showWelcome = 1; // Go back to welcome screen
                showRules = 0;
            } else {
                showRules = 0; // Proceed to game
            }
        }
    }

    SDL_Surface *screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE);
    if (!screen) {
        fprintf(stderr, "Erreur de création de la fenêtre: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 230, 230, 230)); // Very light grey
    TTF_Font* puzzleFont = TTF_OpenFont("assets/arial.ttf", 48);
    if (puzzleFont) {
        SDL_Color textColor = {255, 255, 255};
        SDL_Surface* puzzleText = TTF_RenderText_Blended(puzzleFont, "Good luck!", textColor);
        if (puzzleText) {
            SDL_Surface* previewImage = chargerImage(puzzles[0].full_image_path);
            if (previewImage) {
                SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 230, 230, 230)); // Very light grey
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
                    PUZZLE_WIDTH,
                    PUZZLE_HEIGHT
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

    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 230, 230, 230)); // Very light grey
    SDL_Flip(screen);

    SDL_Surface* completePuzzleImage = NULL;
    Puzzle currentPuzzle = {0};
    init_puzzle_order();
    load_puzzle(&currentPuzzle, &completePuzzleImage);
    if (!completePuzzleImage) {
        fprintf(stderr, "Failed to initialize puzzle\n");
        TTF_CloseFont(font);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Initialize buttons
    SDL_Surface *restartButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *restartButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *finishButtonNormal = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_Surface *finishButtonHover = SDL_CreateRGBSurface(SDL_SWSURFACE, 150, 50, 32, 0, 0, 0, 0);
    SDL_FillRect(restartButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(restartButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));
    SDL_FillRect(finishButtonNormal, NULL, SDL_MapRGB(screen->format, 100, 100, 100));
    SDL_FillRect(finishButtonHover, NULL, SDL_MapRGB(screen->format, 150, 150, 150));

    SDL_Color buttonTextColor = {255, 255, 255};
    SDL_Surface *restartText = TTF_RenderText_Blended(font, "Restart", buttonTextColor);
    SDL_Surface *finishText = TTF_RenderText_Blended(font, "Finish", buttonTextColor);
    SDL_Rect restartTextPos = {10, 10, restartText->w, restartText->h};
    SDL_Rect finishTextPos = {10, 10, finishText->w, finishText->h};
    SDL_BlitSurface(restartText, NULL, restartButtonNormal, &restartTextPos);
    SDL_BlitSurface(restartText, NULL, restartButtonHover, &restartTextPos);
    SDL_BlitSurface(finishText, NULL, finishButtonNormal, &finishTextPos);
    SDL_BlitSurface(finishText, NULL, finishButtonHover, &finishTextPos);

    int pieceSelectionnee = -1;
    int mouseOffsetX = 0;
    int mouseOffsetY = 0;
    int continuer = 1;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();
    int mouseX = 0, mouseY = 0;
    int restartHovered = 0, finishHovered = 0;

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
                        // Check for Restart button click
                        if (mouseX >= 50 && mouseX <= 200 && mouseY >= 800 && mouseY <= 850) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            load_puzzle(&currentPuzzle, &completePuzzleImage);
                            startTime = SDL_GetTicks();
                            if (!completePuzzleImage) {
                                fprintf(stderr, "Failed to load puzzle\n");
                                continuer = 0;
                            }
                            continue;
                        }
                        // Check for Finish button click
                        if (mouseX >= 1240 && mouseX <= 1390 && mouseY >= 800 && mouseY <= 850) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            if (check_puzzle_completion(&currentPuzzle)) {
                                int elapsedTime = (SDL_GetTicks() - startTime) / 1000;
                                SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                if (newWindow) {
                                    showResultPopup(newWindow, 1, elapsedTime);
                                    screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE);
                                    if (screen) {
                                        load_puzzle(&currentPuzzle, &completePuzzleImage);
                                        startTime = SDL_GetTicks();
                                        if (!completePuzzleImage) {
                                            fprintf(stderr, "Failed to load puzzle\n");
                                            continuer = 0;
                                        }
                                    } else {
                                        fprintf(stderr, "Failed to restore main window: %s\n", SDL_GetError());
                                        continuer = 0;
                                    }
                                }
                            } else {
                                SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                                if (newWindow) {
                                    showResultPopup(newWindow, 0, 0);
                                    screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE);
                                    if (!screen) {
                                        fprintf(stderr, "Failed to restore main window: %s\n", SDL_GetError());
                                        continuer = 0;
                                    }
                                }
                            }
                            continue;
                        }
                        // Check for piece selection
                        for (int i = 1; i < NUM_PIECES; i++) {
                            if (!currentPuzzle.pieces[i].isPlaced && currentPuzzle.pieces[i].image) {
                                SDL_Rect *pos = &currentPuzzle.pieces[i].position;
                                if (event.button.x >= pos->x &&
                                    event.button.x <= pos->x + pos->w &&
                                    event.button.y >= pos->y &&
                                    event.button.y <= pos->y + pos->h) {
                                    pieceSelectionnee = i;
                                    mouseOffsetX = event.button.x - pos->x;
                                    mouseOffsetY = event.button.y - pos->y;
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
                    if (pieceSelectionnee != -1) {
                        pieceSelectionnee = -1;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = event.motion.y;
                    restartHovered = (mouseX >= 50 && mouseX <= 200 && mouseY >= 800 && mouseY <= 850);
                    finishHovered = (mouseX >= 1240 && mouseX <= 1390 && mouseY >= 800 && mouseY <= 850);
                    if (pieceSelectionnee != -1) {
                        currentPuzzle.pieces[pieceSelectionnee].position.x = event.motion.x - mouseOffsetX;
                        currentPuzzle.pieces[pieceSelectionnee].position.y = event.motion.y - mouseOffsetY;
                    }
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                        fprintf(stderr, "Enter pressed, checking completion...\n");
                        if (check_puzzle_completion(&currentPuzzle)) {
                            int elapsedTime = (SDL_GetTicks() - startTime) / 1000;
                            SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                            if (newWindow) {
                                showResultPopup(newWindow, 1, elapsedTime);
                                screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE);
                                if (screen) {
                                    load_puzzle(&currentPuzzle, &completePuzzleImage);
                                    startTime = SDL_GetTicks();
                                    if (!completePuzzleImage) {
                                        fprintf(stderr, "Failed to load puzzle\n");
                                        continuer = 0;
                                    }
                                } else {
                                    fprintf(stderr, "Failed to restore main window: %s\n", SDL_GetError());
                                    continuer = 0;
                                }
                            }
                        } else {
                            SDL_Surface *newWindow = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE);
                            if (newWindow) {
                                showResultPopup(newWindow, 0, 0);
                                screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, SDL_HWSURFACE);
                                if (!screen) {
                                    fprintf(stderr, "Failed to restore main window: %s\n", SDL_GetError());
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
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 230, 230, 230)); // Very light grey
        
        // Render timer
        int elapsedTime = (SDL_GetTicks() - startTime) / 1000;
        char timerText[20];
        snprintf(timerText, sizeof(timerText), "Time: %ds", elapsedTime);
        SDL_Color timerColor = {255, 255, 255};
        SDL_Surface *timerSurface = TTF_RenderText_Blended(font, timerText, timerColor);
        if (timerSurface) {
            SDL_Rect timerPos = {20, 20, timerSurface->w, timerSurface->h};
            SDL_BlitSurface(timerSurface, NULL, screen, &timerPos);
            SDL_FreeSurface(timerSurface);
        }

        // Render pieces
        for (int i = 0; i < NUM_PIECES; i++) {
            if (currentPuzzle.pieces[i].image) {
                afficherImage(screen, currentPuzzle.pieces[i].image, currentPuzzle.pieces[i].position);
            } else {
                fprintf(stderr, "Piece %d has no image\n", i);
            }
        }

        // Render buttons
        SDL_Rect restartButtonPos = {50, 800, 150, 50};
        SDL_Rect finishButtonPos = {1240, 800, 150, 50};
        SDL_BlitSurface(restartHovered ? restartButtonHover : restartButtonNormal, NULL, screen, &restartButtonPos);
        SDL_BlitSurface(finishHovered ? finishButtonHover : finishButtonNormal, NULL, screen, &finishButtonPos);

        SDL_Flip(screen);
    }

    for (int i = 0; i < NUM_PIECES; i++) {
        if (currentPuzzle.pieces[i].image) {
            SDL_FreeSurface(currentPuzzle.pieces[i].image);
        }
    }
    if (completePuzzleImage) {
        SDL_FreeSurface(completePuzzleImage);
    }
    if (restartButtonNormal) SDL_FreeSurface(restartButtonNormal);
    if (restartButtonHover) SDL_FreeSurface(restartButtonHover);
    if (finishButtonNormal) SDL_FreeSurface(finishButtonNormal);
    if (finishButtonHover) SDL_FreeSurface(finishButtonHover);
    if (restartText) SDL_FreeSurface(restartText);
    if (finishText) SDL_FreeSurface(finishText);
    if (successSound) {
        Mix_FreeChunk(successSound);
    }
    if (failureSound) {
        Mix_FreeChunk(failureSound);
    }
    if (clickSound) {
        Mix_FreeChunk(clickSound);
    }
    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
    }
    if (font) {
        TTF_CloseFont(font);
    }
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
