#!/bin/bash

echo "Running performance experiments..."
echo "=================================="

rm -f report.txt

echo "Performance Analysis Report" > report.txt
echo "===========================" >> report.txt
echo "" >> report.txt
echo "Test Configuration: freq=10000" >> report.txt
echo "Each experiment run 3 times, average reported" >> report.txt
echo "" >> report.txt

for threads in 1 2 4 8; do
    echo "Testing with $threads thread(s)..."
    
    update_sum=0
    total_sum=0
    stats_sum=0
    seq_update_sum=0
    seq_total_sum=0
    seq_stats_sum=0
    
    # Run 3 times
    for run in 1 2 3; do
        output=$(OMP_NUM_THREADS=$threads ./main 10000 2>&1)
        
        update=$(echo "$output" | grep "^update display:" | awk '{print $3}')
        total=$(echo "$output" | grep "^total amount traded computation time:" | awk '{print $5}')
        stats=$(echo "$output" | grep "^print order stats computation time:" | awk '{print $5}')
        
        seq_update=$(echo "$output" | grep "^seq. update display:" | awk '{print $4}')
        seq_total=$(echo "$output" | grep "^seq. total amount traded computation time:" | awk '{print $6}')
        seq_stats=$(echo "$output" | grep "^seq. print order stats computation time:" | awk '{print $6}')
        
        update_sum=$(echo "$update_sum + $update" | bc)
        total_sum=$(echo "$total_sum + $total" | bc)
        stats_sum=$(echo "$stats_sum + $stats" | bc)
        seq_update_sum=$(echo "$seq_update_sum + $seq_update" | bc)
        seq_total_sum=$(echo "$seq_total_sum + $seq_total" | bc)
        seq_stats_sum=$(echo "$seq_stats_sum + $seq_stats" | bc)
    done
    
    # Calculate averages
    update_avg=$(echo "scale=6; $update_sum / 3" | bc)
    total_avg=$(echo "scale=6; $total_sum / 3" | bc)
    stats_avg=$(echo "scale=6; $stats_sum / 3" | bc)
    seq_update_avg=$(echo "scale=6; $seq_update_sum / 3" | bc)
    seq_total_avg=$(echo "scale=6; $seq_total_sum / 3" | bc)
    seq_stats_avg=$(echo "scale=6; $seq_stats_sum / 3" | bc)
    
    # Calculate speedups
    update_speedup=$(echo "scale=2; $seq_update_avg / $update_avg" | bc)
    total_speedup=$(echo "scale=2; $seq_total_avg / $total_avg" | bc)
    stats_speedup=$(echo "scale=2; $seq_stats_avg / $stats_avg" | bc)
    
    # Write to report
    echo "Threads: $threads" >> report.txt
    echo "----------" >> report.txt
    echo "" >> report.txt
    echo "updateDisplay:" >> report.txt
    echo "  Parallel Time: $update_avg seconds" >> report.txt
    echo "  Sequential Time: $seq_update_avg seconds" >> report.txt
    echo "  Speedup: ${update_speedup}x" >> report.txt
    echo "" >> report.txt
    echo "totalAmountTraded:" >> report.txt
    echo "  Parallel Time: $total_avg seconds" >> report.txt
    echo "  Sequential Time: $seq_total_avg seconds" >> report.txt
    echo "  Speedup: ${total_speedup}x" >> report.txt
    echo "" >> report.txt
    echo "printOrderStats:" >> report.txt
    echo "  Parallel Time: $stats_avg seconds" >> report.txt
    echo "  Sequential Time: $seq_stats_avg seconds" >> report.txt
    echo "  Speedup: ${stats_speedup}x" >> report.txt
    echo "" >> report.txt
    echo "----------------------------------------" >> report.txt
    echo "" >> report.txt
done

echo "Experiments complete! Results written to report.txt"
cat report.txt
