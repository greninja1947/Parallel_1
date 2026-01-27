#include "functions.h"
using namespace std;

struct OrderBookEntry {
    uint32_t stockID;
    bool orderType;     // 0 = buy, 1 = sell
    uint8_t orderQty;
    uint8_t orderValue;
};

// Insert a 0 after every 5 consecutive 1s (bit stuffing)
vector<int> bitStuff(const vector<int>& bits) {
    vector<int> stuffed;
    int ones = 0;
    for (int b : bits) {
        stuffed.push_back(b);
        if (b == 1) {
            ones++;
            if (ones == 5) {
                stuffed.push_back(0);
                ones = 0;
            }
        } else {
            ones = 0;
        }
    }
    return stuffed;
}

// Convert OrderBookEntry to stuffed 64-bit packet
uint64_t encodeOrder(const OrderBookEntry& e) {
    vector<int> bits;

    // stockID (32 bits, LSB first)
    for (int i = 0; i < 32; i++)
        bits.push_back((e.stockID >> i) & 1);

    // orderType (1 bit)
    bits.push_back(e.orderType);

    // orderQty (8 bits)
    for (int i = 0; i < 8; i++)
        bits.push_back((e.orderQty >> i) & 1);

    // orderValue (8 bits)
    for (int i = 0; i < 8; i++)
        bits.push_back((e.orderValue >> i) & 1);

    // Bit stuffing
    vector<int> stuffed = bitStuff(bits);

    // Pack into uint64_t (LSB aligned, MSB padded with 0s)
    uint64_t packet = 0;
    for (size_t i = 0; i < stuffed.size() && i < 64; i++) {
        if (stuffed[i])
            packet |= (1ULL << i);
    }

    return packet;
}

int main(int argc, char* argv[]) {
    if(argc != 2){
        cout<<"Usage: ./test_case_generator <num_test_cases>\n";
        return 1;
    }
    int num_test_cases = atoi(argv[1]);
    srand(time(nullptr));

    int n = num_test_cases; // number of test packets

    vector<uint64_t> packets;
    string file = "testcases.txt";
    ofstream outFile(file);
    for (int i = 0; i < n; i++) {
        OrderBookEntry e;
        e.stockID = rand()%10;
        e.orderType = rand() % 2;
        e.orderQty = rand() % 256;
        e.orderValue = rand() % 256;

        outFile << encodeOrder(e) << "\n";

    }
   
    outFile.close();

    return 0;
}
