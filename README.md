<div align="center">
  <h1>🧩 Greek Puzzle Game</h1>
  <p><i>A puzzle game developed in C using the SDL library on an Ubuntu virtual machine. The game features 3 progressively challenging levels, a built-in score tracking system, immersive sound effects, and an authentic visual design inspired by ancient Greek art and mythology. Piece together iconic Greek images as you advance through each level!</i></p>
 
  [![Made with SDL](https://img.shields.io/badge/Made%20with-SDL_1.2-blue.svg)](https://www.libsdl.org/)
  [![Language](https://img.shields.io/badge/Language-C-orange.svg)]()
</div>

---

## ✨ Features

- 🏛️ **Authentic Visual Design:** Journey through ancient Greece with mythological artwork, authentic imagery, and thematic elements.
- 📈 **Progressive Levels:** Face 3 unique puzzles that grow increasingly difficult as you progress.
- ⏱️ **Immersive Timer & Scoring System:** Race against the 60-second animated clock to assemble the pieces and maximize your score!
- 🎵 **Captivating Audio:** Enjoy atmospheric background music, dramatic sound effects, and a countdown narration that pull you into the mythological puzzle-solving.
- 🖱️ **Interactive Gameplay:** Enjoy smooth and intuitive drag-and-drop mechanics with responsive visual feedback, along with quick on-screen restart and finish controls.

---

## 🛠️ Prerequisites & Dependencies

To compile and run this game, ensure you have the following installed on your system (tested primarily on Ubuntu):

```bash
sudo apt-get install gcc make libsdl1.2-dev libsdl-image1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev
```

- **C Compiler** (e.g., `gcc`)
- **Make**
- **SDL 1.2 Development Libraries:**
  - `SDL`
  - `SDL_image`
  - `SDL_ttf`
  - `SDL_mixer`
  - `SDL_gfx`

---

## 🚀 Build and Run

1. Open your terminal in the root folder of the project.
2. Compile the game using the provided `makefile`:

```bash
make
```

3. Launch the game executable:

```bash
./prog
```
*(Note for Windows users: if using MinGW, it might compile as `prog.exe`, so run `.\prog` or `make` it accordingly)*

---

## 🎮 How to Play

1. **Start the Journey:** Click through the thematic welcome and rules screens to begin your ancient quest.
2. **Memorize:** A quick full-sized preview of the completed Greek artwork is briefly displayed before each puzzle begins.
3. **Assemble the Relics:** Left-click to pick up the scattered pieces, drag them into the correct position, and release your mouse to drop them onto the board.
4. **Submit for Glory:** Finish the puzzle before the 60 seconds run out! Press the **`ENTER`** key to submit and check if your pieces are perfectly placed.
5. **On-Screen Actions:** Use the **Restart** button to try the current level again, or the **Finish** button to force-check the puzzle manually before time is up.

---

<div align="center">
  <i>Developed with ❤️ using C and SDL</i>
</div>
