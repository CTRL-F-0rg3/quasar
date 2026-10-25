# Quasar

Fast spreadsheet for TrangorgeOS. Full Excel dimensions (1,048,576 x 16,384) with a cosmic dark theme.

## Features
- Full Excel dimensions: 1 048 576 rows x 16 384 columns (XFD)
- Lazy model/view — instant scroll even at row 1,000,000
- Formula engine: SUM, AVERAGE, COUNT, MIN, MAX, ROUND, IF, IFS, AND, OR, NOT, CONCAT, TEXTJOIN, LEFT, RIGHT, MID, LEN, UPPER, LOWER, TODAY, NOW, DATE, YEAR, MONTH, DAY, DATEDIF, WORKDAY, PMT, PV, FV and more
- Type detection: numbers (green), percentages (orange), currency (gold), dates (cyan), booleans (blue), formulas (magenta)
- Interactive formula insertion: type `=` then click cells
- Cell formats: bold, italic, alignment, bg/fg colors
- Multi-sheet tabs with save/load (`.qsheet` JSON format)
- Cosmic dark UI (magenta + green accents)

## Build

    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    ./quasar

## Install (from release tarball)

    tar -xzf quasar-linux-v0.1.0.tar.gz
    cd quasar-linux-v0.1.0
    ./install.sh
    quasar

## Requirements

- Arch: `sudo pacman -S qt6-base`
- Debian/Ubuntu: `sudo apt install qt6-base-dev`

## License

GPL-3.0
