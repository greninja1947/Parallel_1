#!/bin/bash

for thread_count in 1 2 4 8 
do
    make run threads=$thread_count freq=10000 > $thread_count."txt"
done