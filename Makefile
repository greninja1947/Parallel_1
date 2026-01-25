CXX = g++-15
OMPFLAGS = -fopenmp

freq ?= 20
threads ?= 8
all: main

main: main.cpp functions.cpp functions_sequential.cpp
	$(CXX) $(OMPFLAGS) -o main main.cpp functions.cpp functions_sequential.cpp

run: all
	@OMP_NUM_THREADS=$(threads) ./main $(freq)
	@./check_outputs.sh

clean:
	rm -f main testcase test 
	rm -f snap_*.txt true_snap_*.txt stats.txt true_stats.txt