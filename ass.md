Assignment 1: Parallel programming with OpenMP
Due date : 27th January 2026
Introduction
The goal of this assignment is to acquaint you with basic parallel programming constructs and
how can we implement them using OpenMP.
You are a developer at a financial firm receiving order book entries from a stock exchange via a
network line, your tasks are to implement some features that analyse the order book as quickly
as possible.
The order book entry
Ideally, an order book entry (this is a very simplified version) will look like the following class:
1 #include <cstdint >
2
3 class OrderBookEntry
4 {
5 uint32_t stockID; //The unique ID of the stock
6 bool orderType; // Indicates whether this is a buy/sell order
7 uint8_t orderQty; // Number of stocks that need to be traded
8 uint8_t orderValue; //The price at which each stock of this order
has to be traded
9 };
Listing 1: Order book entry
However, as you are receiving raw data from a network line, this data is encoded in a bit
stream using bit stuffing.
The packet is constructed in a binary format, starting from the LSB:
1. The first 32 bits indicate the stockID.
2. The next bit indicates a buy (0) or a sell(1) order.
3. The next 8 bits indicate the orderQty.
4. The last 8 bits indicate the orderValue.
Now this 49 bit packet is padded with zeroes to prevent any occurrence of 6 consecutive 1’s
(6 consecutive 1’s represent the packet boundary). This is done by inserting a 0 after every 5
consecutive 1’s.
You are given a vector of 64 bit integers representing the encoded packets, the leading(most
significant) zeroes will represent NO information, i.e. the integer is to be interpreted as :
00000000000000xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
^ MSB ^ meaningful bits (LSB)
1
1 Part 1 : 40 points
Implement the following parallel function:
1 void updateDisplay(const vector <uint64_t > &orderBook , int32_t freq);
This function generates a snapshot of the order book after every freq entries and after the
last entry in the orderBook.
The snapshot is a file named (assuming a 0 based indexing) snap <snap no>.txt, for every
stock traded till now (i.e. the time at which the snapshot is being taken).
It lists, in the decreasing order of spread (ties are broken by the stockID, larger stockID gets
a lower index):
1. stockID
2. Last(most recent) sell value
3. Last(most recent) buy value
4. Spread (absolute difference between the last sell and buy values for this stock).
In a space separated format in a new line. You must ensure that you generate 1 + ⌊ n
f req ⌋
snapshot files, here n is the number of order book entries.
Make sure to handle the processing of each snapshot parallely by fetching threads from a
thread pool and assigning a snapshot to a free thread.
2 Part 2 : 60 points
Implement the following parallel functions:
1. Total amount traded: This function returns the total amount (in money) traded in the
exchange, i.e., ∑
all trades
(orderQty ×orderValue).
You should implement this in the function:
1 int64_t totalAmountTraded(const vector <uint64_t > &orderBook);
2. Order book statistics: This function creates a new file stats.txt. For each stockID
in increasing order, write the:
(a) stockID
(b) Lowest(minimum) sell value
(c) Highest(maximum) buy value
(d) Average order value (till 4 decimal places)
in a space separated format in a new line. You should implement this in the function:
1 void printOrderStats(const vector <uint64_t > &orderBook);
2
3 Submission instructions
You must submit a file named a1 <your entry number>.zip, i.e., if your entry number is
2021CS12345, you should submit a1 2021CS12345.zip.
The zip file should only contain :
1. functions.h
2. functions.cpp
3. report.pdf
Write a 1 page report indicating your design decisions and program performance. The report
should follow the following format, for each function, write:
•A paragraph on your design decisions.
•A graph of speedup increasing with the number of threads, this shows the scalability of
your program.
We will access the functions mentioned in the assignment from functions.h ONLY. Your
code will be compiled in the c++17 standard with the fopenmp compiler flag.
4 Grading
For each function:
•Correctness : 70% credit, the performance will be graded ONLY if your program is
correct. A tolerance of 10−3 will be allowed for decimal values. A correct program gives
a tolerable output and is free of race conditions.
•Performance : 30% credit, we will run your program for performance against our se-
quential version to measure speedup.
Submitting any binaries/extra files in the submission file will result in a straight 0.
The plagiarism policy is mentioned on the course webpage. If you are using generative AI,
make sure to comment the prompts in your code.
3