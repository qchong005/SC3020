# SC3020

SC3020 Database System Principle

## Building the Project

1. Create a build directory:

```bash
mkdir -p build
cd build
```

2. Generate build files with CMake:

```bash
cmake ..
```

3. Build the project:

```bash
make
```

The executable `main` will be created in the project root directory.

## Running the Program

From the project root directory:

```bash
./main
```

## Additional Tools

To compile the B+ Tree parameter calculation utility:

```bash
g++ -std=c++17 calculate_optimal_n.cpp -o calculate_n
./calculate_n
```
