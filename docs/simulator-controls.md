# Simulator Controls Reference

This document details all keyboard, mouse, and gesture controls for the CrossInk SDL2 desktop simulator on macOS and Linux.

---

## 1. Physical Button Mappings (Keyboard)

The simulator maps physical device buttons on the ESP32-S3 (Xteink X4 Pro / Sticky / Xteink X4) directly to standard keyboard keys:

| Hardware Button | Keyboard Key | Description |
|---|---|---|
| **Back / Exit** | `Escape` | Go back, leave the current activity, or dismiss a popup/dialog |
| **Confirm / Select** | `Return` / `Enter` | Confirm selection, open a highlighted item, or accept prompts |
| **Up Button** | `↑` (Up Arrow) | Navigate up in lists/menus, scroll up, or page backward |
| **Down Button** | `↓` (Down Arrow) | Navigate down in lists/menus, scroll down, or page forward |
| **Left Button** | `←` (Left Arrow) | Navigate left / previous page |
| **Right Button** | `→` (Right Arrow) | Navigate right / next page |
| **Home Button (X4 Pro)** | `H` | **Tap:** Toggle reader menu or return to Home<br>**Hold (≥700ms):** Quick reader menu |
| **Power Button** | `P` | Simulates the physical hardware power button press |
| **Sleep Mode Toggle** | `S` | Puts the device into sleep mode (renders chosen sleep screen: Note / Cover / Custom / Dashboard) |

---

## 2. Touchscreen & Gesture Controls (Mouse)

On touch-enabled device profiles (`x4-pro-simulator`, `sticky-simulator`), mouse input translates into capacitive multi-touch coordinates:

| Gesture | Mouse Action | Description |
|---|---|---|
| **Tap** | **Left Click** | Tap any interactive button, note checkbox, menu item, card flip, or rating button |
| **Long Press** | **Click and Hold** (> 500ms) | Trigger long-press context menus or word dictionary lookups |
| **Swipe (Page Turn / Navigation)** | **Click and Drag** | Drag horizontally or vertically across the screen to swipe |
| **Back Gesture** | **Swipe right from the left edge** | Global edge gesture to return to the previous screen/activity |
| **Home Gesture** | **Swipe up from the bottom edge** | Global bottom-edge gesture returning directly to the Home screen |

---

## 3. App-Specific Controls

### Notes App
* **Checkboxes:** Left-click on any task checkbox to mark it done or undone (persisted immediately to markdown).
* **Scroll / Paging:** Press `↑` or `↓` to page through long checklists or multi-page notes.
* **Creating Items:** Click `+ List` or `+ Note` at the bottom to open the virtual on-screen touch keyboard.
* **Note Actions Menu:** Click the top-right button inside any open note to open the sheet menu (**Put on Sleep Screen**, **Rename**, **Delete**, **Type on Phone**).
* **Return / Exit:** Press `Escape` to go from an open note back to the notes deck, and press `Escape` again to return to the Apps shelf.

### Study (Anki Flashcards)
* **Flip Card:** Left-click anywhere on the flashcard body to flip between Question and Answer.
* **Grading / Spaced Repetition:** After flipping to the answer, click **Again**, **Hard**, **Good**, or **Easy** at the bottom (FSRS scheduler immediately updates card memory stability and difficulty).
* **Deck Actions Menu:** Click the top-right menu icon to inspect review statistics, change active decks, or configure sync.
* **Return / Exit:** Press `Escape` to exit the flashcard session and return to the Apps shelf.

### Book Reader (EPUB / TXT / XTC)
* **Turn Page Forward:** Click the right half of the screen, or press `↓` or `→`.
* **Turn Page Backward:** Click the left half of the screen, or press `↑` or `←`.
* **Open Reader Menu:** Click in the center/top band of the screen, or press `H`.
* **Quick Bookmark:** Tap the top-right corner bookmark indicator.

---

## 4. Prerequisites & Installation

To run the desktop simulator, SDL2 development libraries are required:

### macOS
```bash
brew install sdl2
```

### Linux (Debian / Ubuntu)
```bash
sudo apt update && sudo apt install libsdl2-dev
```

---

## 5. Building and Running the Simulator

### Build Commands

Choose the environment profile matching the device target:

```bash
# Xteink X4 Pro (ESP32-S3, 800x480, Touchscreen, Home key, Frontlight) [Recommended]
pio run -e x4-pro-simulator

# Default / X3 / X4 (ESP32-C3, Buttons only, No touch)
pio run -e simulator

# Seeed reTerminal Sticky (ESP32-S3, Touchscreen)
pio run -e sticky-simulator
```

---

### Run Commands

You can run the built executable directly or via PlatformIO:

#### Method 1: Direct Execution
```bash
# Run the X4 Pro simulator
.pio/build/x4-pro-simulator/program

# Run the default buttons simulator
.pio/build/simulator/program
```

#### Method 2: PlatformIO Target
```bash
# Builds and runs in one command
pio run -e x4-pro-simulator -t run_simulator
```

---

### App Autostart Shortcuts

To bypass the Home screen during development and boot directly into a specific app:

```bash
# Boot straight into the Notes app
CROSSPLAY_AUTOSTART=notes .pio/build/x4-pro-simulator/program

# Boot straight into the Study (Anki) flashcard app
CROSSPLAY_AUTOSTART=study .pio/build/x4-pro-simulator/program
```

---

### Running Automated Smoke Tests

To verify UI flows, book rendering, and menu transitions without opening an interactive window:

```bash
python3 scripts/run_simulator_smoke_test.py --env x4-pro-simulator
```

---

## 6. Simulated Filesystem (`fs_/`)

The desktop simulator simulates the device's micro-SD card by mapping paths to the `./fs_/` directory in the project root:

| Device SD Path | Simulator Host Directory | Purpose |
|---|---|---|
| `/notes/` | `./fs_/notes/` | Markdown notes and task checklists (`.md`) |
| `/study/` | `./fs_/study/` | Flashcard decks, review logs (`revlog.dat`), and custom fonts |
| `/books/` | `./fs_/books/` | EPUB, TXT, and XTC books |
| `/.crosspoint/` | `./fs_/.crosspoint/` | Persistent settings, sleep screen config, and book render caches |

> [!TIP]
> **Clearing Stale Caches:**
> If you change layout math, fonts, or file formats and need a fresh start, delete the cache folder:
> ```bash
> rm -rf fs_/.crosspoint/
> ```

