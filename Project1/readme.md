# DBMS Project - Task 1 (Storage Component)

This project implements the **storage component** of a database management system.  
It stores NBA game data from a TSV/CSV file into a simulated heap file using slotted pages.

---

## Project Structure

```
dbms_task1/
├─ include/
│  ├─ common.hpp
│  ├─ disk.hpp
│  ├─ buffer.hpp
│  ├─ slotted_page.hpp
│  ├─ heap_file.hpp
│  ├─ serde.hpp
├─ src/
│  ├─ main.cc
│  ├─ disk.cc
│  ├─ buffer.cc
│  ├─ loader.cc
```

---

##  Requirements

- Windows with **MSYS2 MinGW** or Linux with `g++` installed.
- C++17 compatible compiler (tested with **g++ 15.2.0**).
- Visual Studio Code (optional, for development).

---

##  Setup Instructions

### 1. Install MSYS2 & MinGW (Windows)
1. Download [MSYS2](https://www.msys2.org/).
2. Open **MSYS2 MinGW 64-bit shell** and install toolchain:
   ```bash
   pacman -S --needed base-devel mingw-w64-x86_64-toolchain
   ```
3. Add MinGW to your PATH (e.g., `C:\msys64\mingw64\bin`).

### 2. Verify installation
```bash
g++ --version
```

You should see something like:
```
g++.exe (Rev8, Built by MSYS2 project) 15.2.0
```

---

## Building the Project

From the project root (`Project1`), run:

```bash
g++ -std=c++17 src/main.cpp src/disk.cpp src/buffer.cpp src/loader.cpp -Iinclude -o bin/dbms.exe
```

This will compile the project and create an executable: `**dbms.exe**` inside the `bin/` folder.

- Note: The `bin/` directory and all its contents (like `dbms.exe`) are ignored by Git via `.gitignore`.

---

##  Running

The program requires two arguments:

1. Path to the NBA dataset (`games.txt`)
2. Path to the disk file (simulated database file, will be created)

Example (PowerShell or CMD):

```bash
.\dbms.exe "C:\Users\chong\Downloads\games.txt" "C:\Users\chong\Downloads\nba.disk"
```

---

## Output

```
=== Task 1: Storage Stats ===
Record size (bytes): 28
Number of records: 26552
Records per block: 123
Number of data blocks: 216
Load time (ms): 261
```

---

## Explanation of Stats

- **Record size**: Each NBA game record = 28 bytes (struct layout).
- **Number of records**: Total rows successfully loaded from dataset.
- **Records per block**: How many records fit in a 4 KB page (here = 123).
- **Number of data blocks**: Total pages needed for all records (here = 216).
- **Load time**: Time taken to read dataset, serialize, and store records.

---

