#!/bin/bash

ELF_FILE=".pio/build/emulated/firmware.elf"

# Function to list top 20 largest symbols in a section
list_top_symbols() {
    local section_pattern="$1"
    local section_name="$2"
    
    echo "============================================"
    echo "Top 20 largest symbols in section: $section_name"
    echo "============================================"
    
    # objdump -t format: address flags section size name
    # Filter by section, extract size and name, sort by size descending
    objdump -t "$ELF_FILE" | \
        awk -v pattern="$section_pattern" '$4 ~ pattern { print $5, $6 }' | \
        while read hex name; do
            dec=$((16#$hex))
            echo "$dec $hex $name"
        done | \
        sort -k1 -r -n | \
        head -20 | \
        awk '{ 
            size_kb = $1 / 1024
            printf "  %10s (%7.2f KB)  %s\n", $2, size_kb, $3
        }'
    
    echo ""
}

# List top 20 for each section
list_top_symbols "\\.dram0\\.bss" ".dram0.bss"
list_top_symbols "\\.dram0\\.data" ".dram0.data"
list_top_symbols "\\.flash\\.rodata" ".flash.rodata"
list_top_symbols "\\.flash\\.text" ".flash.text"
list_top_symbols "\\.iram0\\.text" ".iram0.text"