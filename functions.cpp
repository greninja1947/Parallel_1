#include "functions.h"
#include <omp.h>
using namespace std;
struct OrderBookEntry{
    uint32_t stockID;
    bool orderType;
    uint8_t orderQty;
    uint8_t orderValue;
    int index;
};

OrderBookEntry decodePacket(uint64_t packet,int index){
    OrderBookEntry ob;
    ob.stockID = packet & (0xFFFFFFFF);
    ob.orderType = (packet >> 32) & (0x1);
    ob.orderQty = (packet >> 33) & (0xFF);
    ob.orderValue = (packet >>41) & (0xFF);
    ob.index = index;
    return ob;
}

uint64_t unstuffBits(uint64_t packet){
    uint64_t result = 0;
    uint8_t consecutive_ones = 0;
    uint8_t outputbit = 0;
    uint8_t i = 0;
    while(i<64){
        bool type = (packet>>i) & 0x1;
        if(type){
            result |= (1ULL << outputbit);
            outputbit++;
            consecutive_ones++;
            if(consecutive_ones == 5){
                i++;
                consecutive_ones = 0;
            }
        }else{
            consecutive_ones = 0;
            outputbit++;
        }
        i++;
    }
    return result;
}

struct StockInfo{
    uint32_t StockId;
    uint8_t lastSellValue;
    uint8_t lastBuyValue;
    uint8_t minSellValue;
    uint8_t maxBuyValue;
    uint64_t sumOrderValue;  
    uint64_t orderCount;         
    bool hasSellForMin;     
    bool hasBuyForMax;

    StockInfo() : lastSellValue(0), lastBuyValue(0), 
                  minSellValue(255), maxBuyValue(0),
                  sumOrderValue(0), orderCount(0),
                  hasSellForMin(false), hasBuyForMax(false){}
    
    uint8_t getSpread() const {
        return abs((int)lastSellValue - (int)lastBuyValue);
    }
};

void updateDisplay(const std::vector<uint64_t> &orderBook, int32_t freq){
    double check1 = omp_get_wtime();
    
    int max_stocks = 1e5+1;
    int n = orderBook.size();
    int number_of_snapshots = n / freq + 1;
    vector<vector<OrderBookEntry>>allEntries(max_stocks);
    vector<OrderBookEntry> decoded(n);
    #pragma parallel for schedule(static)
    for(int i = 0;i<n;i++){
        decoded[i] = decodePacket(unstuffBits(orderBook[i]),i);
    }

    set<int> unique_stocks;
    for(int i = 0;i<n;i++){
        OrderBookEntry ob = decoded[i];
        unique_stocks.insert(ob.stockID);
        allEntries[ob.stockID].push_back(ob);
    }
    
    int number_of_stocks = unique_stocks.size();
    vector<int>unique_stocks_vector(unique_stocks.begin(),unique_stocks.end());
    vector<vector<StockInfo>> finalData(number_of_snapshots,vector<StockInfo>(max_stocks));
    
    double check2 = omp_get_wtime();

    #pragma omp parallel for 
    for(int i = 0;i<number_of_stocks;i++){
        // i = id of stock in allEntries
        int stockid = unique_stocks_vector[i];
        vector<OrderBookEntry> &entries = allEntries[stockid];
        StockInfo currentStock;
        currentStock.StockId = stockid;
        int snapshotIndex = 0;
        for(int j = 0;j<entries.size();j++){
            OrderBookEntry &entry = entries[j];

            if(entry.orderType){
                currentStock.lastSellValue = entry.orderValue;
                currentStock.hasSellForMin = true;
                if(entry.orderValue < currentStock.minSellValue){
                    currentStock.minSellValue = entry.orderValue;
                }
            }else{
                currentStock.lastBuyValue = entry.orderValue;
                currentStock.hasBuyForMax = true;
                if(entry.orderValue > currentStock.maxBuyValue){
                    currentStock.maxBuyValue = entry.orderValue;
                }
            }
            if(j==0){
                int currIndex = entry.index;
                while(currIndex > ((snapshotIndex + 1) * freq - 1)){
                    // no transactuion for this stock in this snapshot
                    snapshotIndex++;
                }
            }
            if(j!= entries.size() - 1){
                int nextIndex = entries[j+1].index;
                // if the next entry is in the same snapshot no need to update finalData yet
               
                while((nextIndex / freq) != snapshotIndex){
                    StockInfo tempStock = currentStock;
                    tempStock.StockId = entry.stockID;
                    finalData[snapshotIndex][stockid] = tempStock;
                    snapshotIndex++;
                }
            }else{
                // last entry for this stock
                while(snapshotIndex < number_of_snapshots){
                    StockInfo tempStock = currentStock;
                    tempStock.StockId = entry.stockID;
                    finalData[snapshotIndex][stockid] = tempStock;
                    snapshotIndex++;
                }
            }            
        }
    }
    
    double check3 = omp_get_wtime();

    #pragma omp parallel for
    for(int i = 0;i<number_of_snapshots;i++){
        vector<vector<StockInfo>> bucketSort(256);
        for(int j = 0;j<number_of_stocks;j++){
            int stockid = unique_stocks_vector[j];
            if(finalData[i][stockid].hasBuyForMax || finalData[i][stockid].hasSellForMin){
                int spread = finalData[i][stockid].getSpread();
                bucketSort[spread].push_back(finalData[i][stockid]);
            }
        }
        
        string filename = "snap_" + to_string(i) + ".txt";
        FILE* file = fopen(filename.c_str(), "w");
        for(int spread = 255;spread>=0;spread--){
            for(int k = bucketSort[spread].size()-1;k>=0;k--){
                StockInfo stock = bucketSort[spread][k];
                int spread = stock.getSpread();
                fprintf(file, "%u %d %d %d\n", 
                        stock.StockId, 
                        (int)stock.lastSellValue, 
                        (int)stock.lastBuyValue, 
                        spread);
            }
        }
        fclose(file);
    }

    double check4 = omp_get_wtime();

  
}

int64_t totalAmountTraded(const std::vector<uint64_t> &orderBook){
    int64_t total = 0;
    #pragma omp parallel for reduction(+:total)
    for(uint64_t packet : orderBook){
        uint64_t unstuffed = unstuffBits(packet);
        int qty = (unstuffed >> 33) & (0xFF);
        int value = (unstuffed >>41) & (0xFF);
        total += value * qty;
    }
    return total;
}

void printOrderStats(const std::vector<uint64_t> &orderBook){
    int num_threads = omp_get_max_threads();
    vector<unordered_map<uint32_t,StockInfo>> threadStockMaps(num_threads);

    int n = orderBook.size();

    #pragma omp parallel for
    for(int i = 0;i<n;i++){
        int thread_id = omp_get_thread_num();
        uint64_t packet = orderBook[i];
        uint64_t unstuffed = unstuffBits(packet);
        OrderBookEntry entry = decodePacket(unstuffed,i);

        StockInfo &stock = threadStockMaps[thread_id][entry.stockID];
        stock.StockId = entry.stockID;

        if(entry.orderType){
            stock.hasSellForMin = true;
            if(entry.orderValue < stock.minSellValue){
                stock.minSellValue = entry.orderValue;
            }
        }else{
            stock.hasBuyForMax = true;
            if(entry.orderValue > stock.maxBuyValue){
                stock.maxBuyValue = entry.orderValue;
            }
        }

        stock.sumOrderValue += (int64_t)entry.orderValue;
        stock.orderCount++;
    }

    // merge the maps from different threads

    unordered_map<uint32_t,StockInfo> finalStockMap;
    for(int t = 0;t<num_threads;t++){
        for(auto &pair : threadStockMaps[t]){
            uint32_t stockId = pair.first;
            StockInfo &stockFromThread = pair.second;

            StockInfo &finalStock = finalStockMap[stockId];
            finalStock.StockId = stockId;

            if(stockFromThread.hasSellForMin){
                finalStock.hasSellForMin = true;
                if(stockFromThread.minSellValue < finalStock.minSellValue){
                    finalStock.minSellValue = stockFromThread.minSellValue;
                }
            }

            if(stockFromThread.hasBuyForMax){
                finalStock.hasBuyForMax = true;
                if(stockFromThread.maxBuyValue > finalStock.maxBuyValue){
                    finalStock.maxBuyValue = stockFromThread.maxBuyValue;
                }
            }

            finalStock.sumOrderValue += stockFromThread.sumOrderValue;
            finalStock.orderCount += stockFromThread.orderCount;
        }
    }
    string filename = "stats.txt";
    ofstream outFile(filename);
    int max_stocks = 1e5+1;
    vector<StockInfo> stocks(max_stocks);

    for(const auto& pr : finalStockMap){
        stocks[pr.first] = pr.second;
    }
    
    FILE* file = fopen(filename.c_str(), "w");
    for (const auto& stock : stocks) {
        if(stock.orderCount == 0) continue;
        double avg = static_cast<double>(stock.sumOrderValue) / static_cast<double>(stock.orderCount);
        int minsell = stock.hasSellForMin ? stock.minSellValue : 0;
        int maxbuy = stock.hasBuyForMax ? stock.maxBuyValue : 0;
        fprintf(file, "%u %d %d %.4f\n", 
                stock.StockId, minsell, maxbuy, avg);
    }
    
    fclose(file);
}

