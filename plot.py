import matplotlib.pyplot as plt

def plot_performance(thread_counts, speedups):
    plt.figure(figsize=(10, 6))
    plt.plot(thread_counts, speedups[0], marker='o')
    plt.plot(thread_counts, speedups[1], marker='X')
    plt.plot(thread_counts, speedups[2], marker='s')
    plt.legend(['updateDisplay', 'totalAmountTraded', 'printOrderStats'])
    plt.title('Speedup vs Number of Threads')
    plt.xlabel('Number of Threads')
    plt.ylabel('Speedup')
    plt.xticks(thread_counts)
    plt.grid(True)
    plt.savefig('performance_plot.png')
    plt.show()

if __name__ == "__main__":
    thread_counts = [1,2,4,8]
    speedups = [
        [1, 1.981, 3.571, 4.195],  # updateDisplay speedups
        [1, 1.962, 3.776, 4.432],  # totalAmountTraded speedups
        [1,  1.64, 2.365, 2.438]   # printOrderStats speedups
    ]
    plot_performance(thread_counts, speedups)