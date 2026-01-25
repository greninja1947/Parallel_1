CXX = clang++
OMPFLAGS = -fopenmp

freq ?= 20

all: sequence parallel

sequence: functions.cpp
	$(CXX) $(OMPFLAGS) -o sequence functions.cpp

parallel: functions_parallel.cpp
	$(CXX) $(OMPFLAGS) -o parallel functions_parallel.cpp

run: all
	@echo "Running sequential version..."
	@./sequence $(freq)
	@echo "\nRunning parallel version..."
	@./parallel $(freq)
	@./check_outputs.sh


clean:
	rm -f sequence parallel testcase test 
	rm -f snap_*.txt true_snap_*.txt stats.txt true_stats.txt