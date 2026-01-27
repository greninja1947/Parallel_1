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

struct UnstuffResult {
    uint8_t output;         
    uint8_t output_bits;    
    uint8_t input_consumed; 
    uint8_t final_ones;    
};

static UnstuffResult unstuff_lut[256][5]; 
static bool lut_initialized = false;

void initUnstuffLUT() {
    if (lut_initialized) return;
    
    for (int byte_val = 0; byte_val < 256; byte_val++) {
        for (int init_ones = 0; init_ones < 5; init_ones++) {
            uint8_t output = 0;
            uint8_t output_bits = 0;
            uint8_t input_consumed = 0;
            uint8_t consecutive_ones = init_ones;
            
            for (int bit_pos = 0; bit_pos < 8; bit_pos++) {
                uint8_t current_bit = (byte_val >> bit_pos) & 1;
                
                if (current_bit) {
                    output |= (1 << output_bits);
                    output_bits++;
                    consecutive_ones++;
                    
                    if (consecutive_ones == 5) {
                       
                        bit_pos++;
                        input_consumed = bit_pos + 1;
                        consecutive_ones = 0;
                        break;
                    }
                } else {
                    consecutive_ones = 0;
                    output_bits++;
                }
                
                input_consumed = bit_pos + 1;
            }
            
            unstuff_lut[byte_val][init_ones] = {output, output_bits, input_consumed, consecutive_ones};
        }
    }
    lut_initialized = true;
}

inline char* fastIntToStr(char* p, uint32_t value) {
    if (value == 0) {
        *p++ = '0';
        return p;
    }
    
    char temp[12];
    int len = 0;
    while (value > 0) {
        temp[len++] = '0' + (value % 10);
        value /= 10;
    }
    for (int i = len - 1; i >= 0; i--) {
        *p++ = temp[i];
    }
    return p;
}

inline char* fastFloatToStr(char* p, double value) {
    int whole = (int)value;
    int frac = (int)((value - whole) * 10000 + 0.5); 
    
    p = fastIntToStr(p, whole);
    *p++ = '.';
    
    *p++ = '0' + (frac / 1000);
    frac %= 1000;
    *p++ = '0' + (frac / 100);
    frac %= 100;
    *p++ = '0' + (frac / 10);
    *p++ = '0' + (frac % 10);
    
    return p;
}

inline void buildSnapshotLine(string& output, uint32_t stockId, int sellValue, int buyValue, int spread) {
    char line[48];
    char* p = line;
    
    p = fastIntToStr(p, stockId);
    *p++ = ' ';
    p = fastIntToStr(p, sellValue);
    *p++ = ' ';
    p = fastIntToStr(p, buyValue);
    *p++ = ' ';
    p = fastIntToStr(p, spread);
    *p++ = '\n';
    *p = '\0';
    
    output += line;
}

inline void buildStatsLine(string& output, uint32_t stockId, int minSell, int maxBuy, double average) {
    char line[64];
    char* p = line;
    
    p = fastIntToStr(p, stockId);
    *p++ = ' ';
    p = fastIntToStr(p, minSell);
    *p++ = ' ';
    p = fastIntToStr(p, maxBuy);
    *p++ = ' ';
    p = fastFloatToStr(p, average);
    *p++ = '\n';
    *p = '\0';
    
    output += line;
}

uint64_t unstuffBits(uint64_t packet) {
    if (!lut_initialized) initUnstuffLUT();
    
    uint64_t result = 0;
    uint32_t consecutive_ones = 0;
    uint32_t output_pos = 0;
    uint32_t input_pos = 0;
    
    while (input_pos < 64) {
        uint32_t remaining_bits = 64 - input_pos;
        
        if (remaining_bits >= 8) {
            uint8_t input_byte = (packet >> input_pos) & 0xFF;
            UnstuffResult& res = unstuff_lut[input_byte][consecutive_ones];
            
            result |= ((uint64_t)res.output << output_pos);
            output_pos += res.output_bits;
            input_pos += res.input_consumed;
            consecutive_ones = res.final_ones;
        } else {
            uint32_t bit = (packet >> input_pos) & 1;
            
            if (bit) {
                result |= (1ULL << output_pos);
                consecutive_ones++;
                if (consecutive_ones == 5) {
                    input_pos++; 
                    consecutive_ones = 0;
                }
            } else {
                consecutive_ones = 0;
            }
            
            output_pos++;
            input_pos++;
        }
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

struct SnapshotStock {
    uint8_t lastSellValue;
    uint8_t lastBuyValue;
    uint8_t flags;  
    
    SnapshotStock() : lastSellValue(0), lastBuyValue(0), flags(0) {}
    
    bool hasData() const { return flags != 0; }
    uint8_t getSpread() const { return abs((int)lastSellValue - (int)lastBuyValue); }
};

void updateDisplay(const std::vector<uint64_t> &orderBook, int64_t freq){
    
    constexpr int MAX_STOCKS = 100001;
    int64_t n = orderBook.size();
    int number_of_snapshots = n / freq + 1;
    int num_threads = omp_get_max_threads();
    
    vector<OrderBookEntry> decoded(n);
    #pragma omp parallel for schedule(static)
    for(int i = 0; i < n; i++){
        decoded[i] = decodePacket(unstuffBits(orderBook[i]), i);
    }
    
    vector<vector<int>> threadCounts(num_threads, vector<int>(MAX_STOCKS, 0));
    
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        vector<int>& myCounts = threadCounts[tid];
        
        #pragma omp for schedule(static)
        for(int i = 0; i < n; i++){
            myCounts[decoded[i].stockID]++;
        }
    }
    
    vector<int> stockCounts(MAX_STOCKS, 0);
    vector<int> unique_stocks_vector;
    unique_stocks_vector.reserve(MAX_STOCKS);
    
    for(int s = 0; s < MAX_STOCKS; s++){
        int total = 0;
        for(int t = 0; t < num_threads; t++){
            total += threadCounts[t][s];
        }
        stockCounts[s] = total;
        if(total > 0){
            unique_stocks_vector.push_back(s);
        }
    }
    
    int number_of_stocks = unique_stocks_vector.size();
    
    vector<vector<OrderBookEntry>> allEntries(MAX_STOCKS);
    for(int s : unique_stocks_vector){
        allEntries[s].reserve(stockCounts[s]);
    }
    
    for(int i = 0; i < n; i++){
        allEntries[decoded[i].stockID].push_back(decoded[i]);
    }
    
    decoded.clear();
    decoded.shrink_to_fit();
    threadCounts.clear();
    threadCounts.shrink_to_fit();
    stockCounts.clear();
    stockCounts.shrink_to_fit();
    
    
    vector<vector<SnapshotStock>> finalData(number_of_snapshots, vector<SnapshotStock>(MAX_STOCKS));
    
    #pragma omp parallel for schedule(dynamic, 64)
    for(int i = 0; i < number_of_stocks; i++){
        int stockid = unique_stocks_vector[i];
        vector<OrderBookEntry>& entries = allEntries[stockid];
        if(entries.empty()) continue;
        
        SnapshotStock currentStock;
        int snapshotIndex = 0;
        
        for(size_t j = 0; j < entries.size(); j++){
            OrderBookEntry& entry = entries[j];
            
            if(entry.orderType){
                currentStock.lastSellValue = entry.orderValue;
                currentStock.flags |= 1;  
            } else {
                currentStock.lastBuyValue = entry.orderValue;
                currentStock.flags |= 2;  
            }
            
            if(j == 0){
                int currIndex = entry.index;
                while(currIndex > ((snapshotIndex + 1) * freq - 1)){
                    snapshotIndex++;
                }
            }
            
            if(j != entries.size() - 1){
                int nextIndex = entries[j+1].index;
                while((nextIndex / freq) != snapshotIndex){
                    finalData[snapshotIndex][stockid] = currentStock;
                    snapshotIndex++;
                }
            } else {
                while(snapshotIndex < number_of_snapshots){
                    finalData[snapshotIndex][stockid] = currentStock;
                    snapshotIndex++;
                }
            }
        }
    }
    
    
    #pragma omp parallel
    {
        vector<vector<int>> bucketIndices(256);  
        char filenameBuf[32];
        
        #pragma omp for schedule(dynamic)
        for(int i = 0; i < number_of_snapshots; i++){
            for(int b = 0; b < 256; b++){
                bucketIndices[b].clear();
            }
            
            for(int j = 0; j < number_of_stocks; j++){
                int stockid = unique_stocks_vector[j];
                SnapshotStock& stock = finalData[i][stockid];
                if(stock.hasData()){
                    int spread = stock.getSpread();
                    bucketIndices[spread].push_back(stockid);
                }
            }
            
            int totalStocks = 0;
            for(int spread = 0; spread < 256; spread++){
                totalStocks += bucketIndices[spread].size();
            }
            
            string output;
            output.reserve(totalStocks * 40);
            
           
            for(int spread = 255; spread >= 0; spread--){
                vector<int>& bucket = bucketIndices[spread];
                for(int k = bucket.size() - 1; k >= 0; k--){
                    int stockid = bucket[k];
                    SnapshotStock& stock = finalData[i][stockid];
                    buildSnapshotLine(output, stockid, 
                                    (int)stock.lastSellValue, 
                                    (int)stock.lastBuyValue, 
                                    spread);
                }
            }
            
            sprintf(filenameBuf, "snap_%d.txt", i);
            FILE* file = fopen(filenameBuf, "w");
            setvbuf(file, nullptr, _IOFBF, 65536);
            fwrite(output.c_str(), 1, output.length(), file);
            fclose(file);
        }
    }
    
    
    // cout<<"Decoding Time: "<<(check2 - check1)<<" seconds\n";
    // cout<<"Processing Time: "<<(check3 - check2)<<" seconds\n";
    // cout<<"Writing Time: "<<(check4 - check3)<<" seconds\n";
}

int64_t totalAmountTraded(const std::vector<uint64_t> &orderBook){
    int64_t total = 0;
    #pragma omp parallel for schedule(static) reduction(+:total)
    for(uint64_t packet : orderBook){
        uint64_t unstuffed = unstuffBits(packet);
        int qty = (unstuffed >> 33) & (0xFF);
        int value = (unstuffed >>41) & (0xFF);
        total += value * qty;
    }
    return total;
}

struct StockStatsCompact {
    uint64_t sumOrderValue;
    uint32_t orderCount;
    uint8_t minSellValue;
    uint8_t maxBuyValue;
    uint8_t hasSellForMin;
    uint8_t hasBuyForMax;
    
    StockStatsCompact() : sumOrderValue(0), orderCount(0), 
                          minSellValue(255), maxBuyValue(0),
                          hasSellForMin(0), hasBuyForMax(0) {}
};

void printOrderStats(const std::vector<uint64_t> &orderBook){
    
    constexpr int MAX_STOCKS = 100001;
    int num_threads = omp_get_max_threads();
    int64_t n = orderBook.size();
    
    vector<StockStatsCompact> allThreadStats(num_threads * MAX_STOCKS);
    
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        StockStatsCompact* __restrict__ myStats = &allThreadStats[thread_id * MAX_STOCKS];
        
        #pragma omp for schedule(static)
        for(int i = 0; i < n; i++){
            uint64_t unstuffed = unstuffBits(orderBook[i]);
            
            uint32_t stockID = unstuffed & 0xFFFFFFFF;
            bool orderType = (unstuffed >> 32) & 0x1;
            uint8_t orderValue = (unstuffed >> 41) & 0xFF;
            
            StockStatsCompact& stock = myStats[stockID];
            
            if(orderType){
                stock.hasSellForMin = 1;
                if(orderValue < stock.minSellValue){
                    stock.minSellValue = orderValue;
                }
            } else {
                stock.hasBuyForMax = 1;
                if(orderValue > stock.maxBuyValue){
                    stock.maxBuyValue = orderValue;
                }
            }
            
            stock.sumOrderValue += orderValue;
            stock.orderCount++;
        }
    }
    
    
    StockStatsCompact* finalStats = &allThreadStats[0];
    
    #pragma omp parallel for schedule(static, 2048)
    for(int stockId = 0; stockId < MAX_STOCKS; stockId++){
        StockStatsCompact& final_stock = finalStats[stockId];
        
        for(int t = 1; t < num_threads; t++){
            StockStatsCompact& thread_stock = allThreadStats[t * MAX_STOCKS + stockId];
            
            if(thread_stock.orderCount > 0){
                if(thread_stock.hasSellForMin){
                    final_stock.hasSellForMin = 1;
                    if(thread_stock.minSellValue < final_stock.minSellValue){
                        final_stock.minSellValue = thread_stock.minSellValue;
                    }
                }
                
                if(thread_stock.hasBuyForMax){
                    final_stock.hasBuyForMax = 1;
                    if(thread_stock.maxBuyValue > final_stock.maxBuyValue){
                        final_stock.maxBuyValue = thread_stock.maxBuyValue;
                    }
                }
                
                final_stock.sumOrderValue += thread_stock.sumOrderValue;
                final_stock.orderCount += thread_stock.orderCount;
            }
        }
    }
    
    
    string output;
    output.reserve(MAX_STOCKS * 50);
    
    for(int stockId = 0; stockId < MAX_STOCKS; stockId++){
        const StockStatsCompact& stock = finalStats[stockId];
        if(stock.orderCount == 0) continue;
        
        double avg = static_cast<double>(stock.sumOrderValue) / static_cast<double>(stock.orderCount);
        int minsell = stock.hasSellForMin ? stock.minSellValue : 0;
        int maxbuy = stock.hasBuyForMax ? stock.maxBuyValue : 0;
        
        buildStatsLine(output, stockId, minsell, maxbuy, avg);
    }
    
    FILE* file = fopen("stats.txt", "w");
    setvbuf(file, nullptr, _IOFBF, 65536);
    fwrite(output.c_str(), 1, output.length(), file);
    fclose(file);
    
    
    // cout<<"Stats finding Time: "<<(t2 - t1)<<" seconds\n";
    // cout<<"Merging Time: "<<(t3 - t2)<<" seconds\n";
    // cout<<"Stats Writing Time: "<<(t4 - t3)<<" seconds\n";
}

