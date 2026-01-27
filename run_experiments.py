#!/usr/bin/env python3
import subprocess
import re
import statistics

def run_experiment(threads, freq=10000, runs=3):
    results = {
        'update': [], 'total': [], 'stats': [],
        'seq_update': [], 'seq_total': [], 'seq_stats': []
    }
    
    for _ in range(runs):
        env = {'OMP_NUM_THREADS': str(threads)}
        output = subprocess.check_output(['./main', str(freq)], 
                                        env={**subprocess.os.environ, **env},
                                        text=True, stderr=subprocess.STDOUT)
        
        # Parse output
        for line in output.split('\n'):
            line = line.strip()
            if line.startswith('update display:'):
                results['update'].append(float(line.split()[2]))
            elif line.startswith('total amount traded computation time:'):
                results['total'].append(float(line.split()[5]))
            elif line.startswith('print order stats computation time:'):
                results['stats'].append(float(line.split()[5]))
            elif line.startswith('seq. update display:'):
                results['seq_update'].append(float(line.split()[3]))
            elif line.startswith('seq. total amount traded computation time:'):
                results['seq_total'].append(float(line.split()[6]))
            elif line.startswith('seq. print order stats computation time:'):
                results['seq_stats'].append(float(line.split()[6]))
    
    # Calculate averages
    return {
        'update_avg': statistics.mean(results['update']),
        'total_avg': statistics.mean(results['total']),
        'stats_avg': statistics.mean(results['stats']),
        'seq_update_avg': statistics.mean(results['seq_update']),
        'seq_total_avg': statistics.mean(results['seq_total']),
        'seq_stats_avg': statistics.mean(results['seq_stats']),
    }

def main():
    print("Running performance experiments...")
    print("==================================\n")
    
    with open('report.txt', 'w') as f:
        f.write("Performance Analysis Report\n")
        f.write("===========================\n\n")
        f.write("Test Configuration: freq=10000\n")
        f.write("Each experiment run 3 times, average reported\n\n")
        
        for threads in [1, 2, 4, 8]:
            print(f"Testing with {threads} thread(s)...")
            results = run_experiment(threads)
            
            # Calculate speedups
            update_speedup = results['seq_update_avg'] / results['update_avg']
            total_speedup = results['seq_total_avg'] / results['total_avg']
            stats_speedup = results['seq_stats_avg'] / results['stats_avg']
            
            # Write to file
            f.write(f"Threads: {threads}\n")
            f.write("-" * 40 + "\n\n")
            
            f.write("updateDisplay:\n")
            f.write(f"  Parallel Time:   {results['update_avg']:.6f} seconds\n")
            f.write(f"  Sequential Time: {results['seq_update_avg']:.6f} seconds\n")
            f.write(f"  Speedup:         {update_speedup:.2f}x\n\n")
            
            f.write("totalAmountTraded:\n")
            f.write(f"  Parallel Time:   {results['total_avg']:.6f} seconds\n")
            f.write(f"  Sequential Time: {results['seq_total_avg']:.6f} seconds\n")
            f.write(f"  Speedup:         {total_speedup:.2f}x\n\n")
            
            f.write("printOrderStats:\n")
            f.write(f"  Parallel Time:   {results['stats_avg']:.6f} seconds\n")
            f.write(f"  Sequential Time: {results['seq_stats_avg']:.6f} seconds\n")
            f.write(f"  Speedup:         {stats_speedup:.2f}x\n\n")
            
            f.write("=" * 40 + "\n\n")
    
    print("\nExperiments complete! Results written to report.txt")
    with open('report.txt', 'r') as f:
        print(f.read())

if __name__ == '__main__':
    main()
