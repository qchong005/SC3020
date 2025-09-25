#include "disk.hpp"
#include <iostream>

using namespace std;

Disk::Disk(const string& fname) {
    filename = fname;

    // If file exists with data, clear the data
    // Else create a new file with filename
    file.open(filename, ios::out | ios::binary | ios::trunc);  
    file.close();
    file.open(filename, ios::in | ios::out | ios::binary);  // Open file in read/write mode

    if (!file.is_open()) {
        throw runtime_error("Cannot open file: " + fname);
    }
}


Disk::~Disk() {
    if (file.is_open()) {
        file.close();
    }
}


void Disk::writeBlock(const Block& block) {
    if (!file.is_open()) {
        cerr << "File not open for writing." << endl;
        return;
    }

    file.seekp(0, ios::end);  // Move to end of file
    file.write(reinterpret_cast<const char*>(&block), sizeof(Block));   // Write the block
    file.flush();  // flush the buffer to make sure data is written to disk
}


Block Disk::readBlock(size_t index) {
    if (!file.is_open()) {
        cerr << "File not open for reading." << endl;
        return Block();  // Return empty block
    }

    file.seekg(0, ios::end);
    int file_size = file.tellg();    // Get current file size
    if (index * sizeof(Block) >= file_size) {
        cerr << "Out of range" << endl;
        return Block();  // Return empty block
    }

    file.seekg(index * sizeof(Block), ios::beg);  // Move file pointer to the start of the block to read

    Block block;
    file.read(reinterpret_cast<char*>(&block), sizeof(Block));  // Read the block
    return block;
}


int Disk::getNumBlocks() {
    file.seekg(0, ios::end);
    int file_size = file.tellg();    // Get current file size
    return file_size / sizeof(Block);
}


int Disk::getTotalRecords() {
    int total = 0;
    int num_blocks = getNumBlocks();
    for (int i = 0; i < num_blocks; i++) {
        Block b = readBlock(i);
        total += b.num_records;
    }

    return total;
}

