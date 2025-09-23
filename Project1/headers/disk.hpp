#pragma once
#include <string>
#include <fstream>
#include "block.hpp"

using namespace std;

class Disk {
    private:
        string filename;
        fstream file;

    public:
        Disk(const string& fname);
        ~Disk();

        void writeBlock(const Block& block);
        Block readBlock(size_t index);
        int getNumBlocks();
        int getTotalRecords();
};
