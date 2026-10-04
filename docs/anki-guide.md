# Anki Flashcard Setup & Testing Guide

This guide explains how to convert, install, and test Anki decks on CrossInk e-reader devices (such as the Xteink X4 Pro, X3, and reTerminal Sticky) as well as the native desktop emulator.

---

## 1. How Decks Work on the E-Reader

The e-reader firmware **cannot directly read raw `.apkg` or `.zip` files** because an `.apkg` is an SQLite database with uncompiled media archives, which requires too much RAM and processing overhead for an ESP32 microcontroller to query on the fly.

Instead, CrossInk reads **optimized, pre-indexed binary deck files** (`deck.dat`, `cards.dat`, and `meta.dat`) located in `/study/<deck-name>/` on the SD card:

```text
SD_CARD/
└── study/
    └── <deck-name>/          <-- e.g. "mandarin", "sat-vocab", "spanish"
        ├── deck.dat          (Immutable note & card content)
        ├── cards.dat         (FSRS scheduling state: stability, difficulty, due dates)
        ├── meta.dat          (Deck parameters, limits, and configuration)
        └── revlog.dat        (Append-only review log for syncing back to Anki)
```

---

## 2. Converting an `.apkg` / Zip into Device Format

### Method A: Web Installer (Recommended — 100% No-Terminal)

1. Open **[crossplay.ma-r-s.com/study](https://crossplay.ma-r-s.com/study/)** in any modern web browser.
2. Drag and drop your `.apkg` file onto the page.
   > **Note:** If you extracted or renamed your file to `.zip`, ensure it has the `.apkg` extension before dropping it.
3. The page runs entirely inside your browser tab using WebAssembly/Pyodide (nothing is uploaded to external servers).
4. Review your cards in the on-screen preview.
5. Click **Save to SD Card** (or download the deck folder).
6. Copy the resulting folder into the `/study/` directory on your SD card.

---

### Method B: Automated Python CLI (From Anki Collection)

If you have Anki installed on your computer with your deck already imported:

1. Insert your SD card into your computer (or mount your device via USB Drive mode).
2. From the repository root directory, run:
   ```bash
   ./tools_local/study/study.py setup
   ```
3. The interactive setup wizard will:
   - Auto-detect your local Anki profile.
   - List available decks and card counts.
   - Auto-detect your mounted SD card.
   - Convert notes, build subsetted fonts, and pack images.
   - Write directly to `/study/<deck-name>/` on your SD card.

---

### Method C: Converting Directly from an `.apkg` File

If you have an exported `.apkg` file and want to convert it via terminal without importing into desktop Anki:

```bash
# Verify and unpack the package
python3 tools_local/study/try_apkg.py /path/to/your_deck.apkg
```

---

## 3. Testing on the Desktop Simulator

You can test decks without flashing a physical device using CrossInk's native simulator:

1. Copy your converted deck folder into the simulator's virtual SD directory (`fs_/study/`):
   ```bash
   mkdir -p fs_/study/<your-deck-name>
   cp -r /path/to/converted_deck/* fs_/study/<your-deck-name>/
   ```

2. Build and launch the native simulator:
   ```bash
   pio run -e x4-pro-simulator
   .pio/build/x4-pro-simulator/program
   ```

3. On the simulator screen:
   - Click **Apps** from the Home screen.
   - Click **ANKI**.
   - Your deck will load immediately. Click **STUDY** to test flipping cards, ratings (Again, Hard, Good, Easy), and FSRS scheduling calculations.

---

## 4. Testing on the Physical Device (Xteink X4 Pro)

1. Connect your SD card to your computer (or enable USB Drive mode on your device).
2. Ensure your deck files are placed inside:
   ```text
   /study/<deck-name>/
   ```
   *(For example: `/study/spanish/deck.dat`, `/study/spanish/cards.dat`, `/study/spanish/meta.dat`)*
3. Eject the SD card and insert it into your Xteink X4 Pro.
4. On the Home screen, tap **Apps** > **ANKI**.
5. Tap **STUDY** to begin your review session.
   - Tap anywhere on the card to flip between Question and Answer.
   - Rate your recall using the 4 rating buttons: `AGAIN`, `HARD`, `GOOD`, or `EASY`.
   - If multiple decks are installed, tap **CHANGE DECK** to switch between them.
