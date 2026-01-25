#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>

void updateDisplay(const std::vector<uint64_t> &orderBook, int32_t freq);
int64_t totalAmountTraded(const std::vector<uint64_t> &orderBook);
void printOrderStats(const std::vector<uint64_t> &orderBook);
void readPacketsFromFile(std::vector<uint64_t>& packets,const std::string& filename){
    std::ifstream infile(filename);
    uint64_t value;

    while (infile >> value) {
        packets.push_back(value);
    }
}






