#!/bin/bash

# Change to snapshots directory
cd outputs

# Flag to track if all diffs are empty
snaps_correct=true
stats_correct=true

# Loop through all snap_*.txt files (excluding true_snap_*.txt)
for snap_file in snap_*.txt; do
    # Check if any snap files exist
    if [ ! -f "$snap_file" ]; then
        echo "Error: No snap files found"
        snaps_correct=false
        break
    fi
    
    # Extract the number from snap_X.txt
    number=$(echo "$snap_file" | sed 's/snap_\(.*\)\.txt/\1/')
    true_snap_file="true_snap_${number}.txt"
    
    # Check if corresponding true_snap file exists
    if [ ! -f "$true_snap_file" ]; then
        echo "Error: $true_snap_file not found"
        snaps_correct=false
        continue
    fi
    
    # Perform diff and check if there are differences
    if ! diff -q "$snap_file" "$true_snap_file" > /dev/null 2>&1; then
        snaps_correct=false
    fi
done

if ! diff -q stats.txt true_stats.txt > /dev/null 2>&1; then
    stats_correct=false
fi

# Print result
if [ "$snaps_correct" = true ]; then
    echo "snapshots are correct"
else
    echo "snapshots are not correct"
fi

if [ "$stats_correct" = true ]; then
    echo "stats are correct"
else
    echo "stats are not correct"
fi

