#include "functions.h"
#include <omp.h>
using namespace std;

void readPacketsFromFile(std::vector<uint64_t>& packets,const std::string& filename){
    std::ifstream infile(filename);
    uint64_t value;

    while (infile >> value) {
        packets.push_back(value);
    }
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

    time_start1 = omp_get_wtime();
    _updateDisplay(temp,freq);
    time_end1 = omp_get_wtime();
    
    time_start2 = omp_get_wtime();
    total = _totalAmountTraded(temp);
    time_end2 = omp_get_wtime();
    
    cout<<"total amount traded: "<<total<<"\n";

    time_start3 = omp_get_wtime();
    _printOrderStats(temp);
    time_end3 = omp_get_wtime();
    
    cout<<"seq. update display: "<<time_end1 - time_start1<<" seconds\n";
    cout<<"seq. total amount traded computation time: "<<time_end2 - time_start2<<" seconds\n";
    cout<<"seq. print order stats computation time: "<<time_end3 - time_start3<<" seconds\n";


    return 0;
}
