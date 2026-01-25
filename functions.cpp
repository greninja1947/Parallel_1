#include "functions.h"
#include <omp.h>
using namespace std;
struct OrderBookEntry {
    uint32_t stockID;
    bool orderType; 
    uint8_t orderQty;
    uint8_t orderValue;
};

OrderBookEntry decodePacket(uint64_t packet) {
    OrderBookEntry entry;
    entry.stockID = (packet >> 0) & 0xFFFFFFFF;
    entry.orderType = ((packet >> 32) & 0x1) == 1;
    entry.orderQty = (packet >> 33) & 0xFF;
    entry.orderValue = (packet >> 41) & 0xFF;
    
    return entry;
}

uint64_t unstuffBits(uint64_t packet) {
    uint64_t result = 0;
    int outputBitPos = 0;
    int consecutiveOnes = 0;
    const int TARGET_BITS = 49;
    
    int i = 0;
    while (i < 64 && outputBitPos < TARGET_BITS) {
        bool bit = (packet >> i) & 0x1;
        
        if (bit) {
            consecutiveOnes++;
            result |= (1ULL << outputBitPos);
            outputBitPos++;
            
            if (consecutiveOnes == 5) {
                consecutiveOnes = 0;
                i++; 
            }
        } else {
            consecutiveOnes = 0;
            outputBitPos++;
        }
        i++;
    }
    
    return result;
}

struct StockInfo {
    uint32_t stockID;
    uint8_t lastSellValue;
    uint8_t lastBuyValue;
    uint8_t minSellValue;
    uint8_t maxBuyValue;
    int64_t sumOrderValue;  
    int orderCount;         
    bool hasSellForMin;     
    bool hasBuyForMax;      
    
    StockInfo() : lastSellValue(0), lastBuyValue(0), 
                  minSellValue(255), maxBuyValue(0),
                  sumOrderValue(0), orderCount(0),
                  hasSellForMin(false), hasBuyForMax(false) {}
    
    int getSpread() const {
        return abs((int)lastSellValue - (int)lastBuyValue);
    }
};

void updateDisplay(const std::vector<uint64_t> &orderBook, int32_t freq) {
    std::map<uint32_t, StockInfo> stockMap;
    int snapCount = 0;
    
    for (size_t i = 0; i < orderBook.size(); i++) {
        uint64_t unstuffed = unstuffBits(orderBook[i]);
        OrderBookEntry entry = decodePacket(unstuffed);
        
        StockInfo& stock = stockMap[entry.stockID];
        stock.stockID = entry.stockID;
        
        if (entry.orderType) {
            stock.lastSellValue = entry.orderValue;
            stock.hasSellForMin = true;
            if (entry.orderValue < stock.minSellValue) {
                stock.minSellValue = entry.orderValue;
            }
        } else {
            stock.lastBuyValue = entry.orderValue;
            stock.hasBuyForMax = true;
            if (entry.orderValue > stock.maxBuyValue) {
                stock.maxBuyValue = entry.orderValue;
            }
        }
        
        if ((i + 1) % freq == 0) {
            std::vector<StockInfo> stocks;
            for (auto& pair : stockMap) {
                stocks.push_back(pair.second);
            }
            
            std::sort(stocks.begin(), stocks.end(), [](const StockInfo& a, const StockInfo& b) {
                int spreadA = a.getSpread();
                int spreadB = b.getSpread();
                if (spreadA != spreadB) {
                    return spreadA > spreadB;
                }
                return a.stockID > b.stockID;
            });
            
            std::string filename = "true_snap_" + std::to_string(snapCount) + ".txt";
            FILE* file = fopen(filename.c_str(), "w");
            
            for (const auto& stock : stocks) {
                int spread = stock.getSpread();
                fprintf(file, "%u %d %d %d\n",
                        stock.stockID,
                        (int)stock.lastSellValue,
                        (int)stock.lastBuyValue,
                        spread);
            }
            
            fclose(file);
            snapCount++;
        }

        if (i == orderBook.size() - 1) {
            std::vector<StockInfo> stocks;
            for (auto& pair : stockMap) {
                stocks.push_back(pair.second);
            }
            
            std::sort(stocks.begin(), stocks.end(), [](const StockInfo& a, const StockInfo& b) {
                int spreadA = a.getSpread();
                int spreadB = b.getSpread();
                if (spreadA != spreadB) {
                    return spreadA > spreadB;
                }
                return a.stockID > b.stockID;
            });
            
            std::string filename = "true_snap_" + std::to_string(snapCount) + ".txt";
            FILE* file = fopen(filename.c_str(), "w");
            
        
            for (const auto& stock : stocks) {
                int spread = stock.getSpread();
                fprintf(file, "%u %d %d %d\n",
                        stock.stockID,
                        (int)stock.lastSellValue,
                        (int)stock.lastBuyValue,
                        spread);
            }
            
            fclose(file);
            snapCount++;
        }
    }
}

int64_t totalAmountTraded(const std::vector<uint64_t> &orderBook) {
    int64_t total = 0;
    
    for (uint64_t packet : orderBook) {
        uint64_t unstuffed = unstuffBits(packet);
        OrderBookEntry entry = decodePacket(unstuffed);
        
        total += (int64_t)entry.orderQty * (int64_t)entry.orderValue;
    }
    
    return total;
}

void printOrderStats(const std::vector<uint64_t> &orderBook) {
    std::map<uint32_t, StockInfo> stockMap;
    
    for (uint64_t packet : orderBook) {
        uint64_t unstuffed = unstuffBits(packet);
        OrderBookEntry entry = decodePacket(unstuffed);
        
        StockInfo& stock = stockMap[entry.stockID];
        stock.stockID = entry.stockID;
        
        if (entry.orderType) {
            stock.hasSellForMin = true;
            if (entry.orderValue < stock.minSellValue) {
                stock.minSellValue = entry.orderValue;
            }
        } else {
            stock.hasBuyForMax = true;
            if (entry.orderValue > stock.maxBuyValue) {
                stock.maxBuyValue = entry.orderValue;
            }
        }
        
        stock.sumOrderValue += (int64_t)entry.orderValue;
        stock.orderCount++;
    }
    
    FILE* file = fopen("true_stats.txt", "w");
    
    for (auto& pair : stockMap) {
        const StockInfo& stock = pair.second;
            
        double avgOrderValue = 0.0;
        if (stock.orderCount > 0) {
            avgOrderValue = static_cast<double>(stock.sumOrderValue) /
                            static_cast<double>(stock.orderCount);
        }
            
        uint8_t minSell = stock.hasSellForMin ? stock.minSellValue : 0;
        uint8_t maxBuy  = stock.hasBuyForMax  ? stock.maxBuyValue  : 0;
    
        fprintf(file, "%u %d %d %.4f\n",
                stock.stockID,
                static_cast<int>(minSell),
                static_cast<int>(maxBuy),
                avgOrderValue);
    }
    
    fclose(file);
}

int main(int argc, char* argv[]){

    int num_threads = omp_get_max_threads();

    
    if(argc != 2){
        cout<<"Usage: ./functions_parallel <freq>\n";
        return 1;
    }
    int32_t freq = atoi(argv[1]);
    vector<uint64_t> temp;
    readPacketsFromFile(temp,"testcases.txt");
    double time_start1 = omp_get_wtime();
    updateDisplay(temp,freq);
    double time_end1 = omp_get_wtime();
    
    double time_start2 = omp_get_wtime();
    int64_t total = totalAmountTraded(temp);
    double time_end2 = omp_get_wtime();
    
    cout<<"total amount traded: "<<total<<"\n";

    double time_start3 = omp_get_wtime();
    printOrderStats(temp);
    double time_end3 = omp_get_wtime();
    
    cout<<"update display: "<<time_end1 - time_start1<<" seconds\n";
    cout<<"total amount traded computation time: "<<time_end2 - time_start2<<" seconds\n";
    cout<<"print order stats computation time: "<<time_end3 - time_start3<<" seconds\n";

    return 0;
}
