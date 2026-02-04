#!/usr/bin/env python3
"""
Parse and list all packed data in a hypher-generated hyphenation trie .bin file.

Binary format (from hyphenation-trie-format.md):
- uint32_t root_addr_be: big-endian offset of the root node
- uint8_t levels[]: shared "levels" tape (dist/score pairs)
- uint8_t nodes[]: node records packed back-to-back

Node encoding:
- Bit 7: set when the node stores scores (levels)
- Bits 5-6: stride of target deltas (1, 2, or 3 bytes, big-endian)
- Bits 0-4: transition count (values >= 31 spill into extra byte)
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class LevelEntry:
    """A single dist/score pair from the levels tape."""
    dist: int
    score: int
    raw_byte: int


@dataclass
class Transition:
    """A single transition edge in a trie node."""
    label: int  # byte value (letter)
    label_char: str  # human-readable character
    delta: int  # signed relative offset
    target_addr: int  # absolute address of target node


@dataclass
class TrieNode:
    """Decoded view of a single trie node."""
    addr: int
    has_levels: bool
    stride: int
    child_count: int
    transitions: list[Transition]
    levels_offset: int | None
    levels_len: int
    levels: list[LevelEntry]
    raw_header: bytes


def decode_delta(buf: bytes, stride: int) -> int:
    """Convert packed stride-sized delta back into a signed offset."""
    if stride == 1:
        # Signed 8-bit
        val = buf[0]
        return val if val < 128 else val - 256
    elif stride == 2:
        # Signed 16-bit big-endian
        val = (buf[0] << 8) | buf[1]
        return val if val < 32768 else val - 65536
    else:  # stride == 3
        # 24-bit with bias of 2^23
        val = (buf[0] << 16) | (buf[1] << 8) | buf[2]
        return val - (1 << 23)


def decode_node(data: bytes, addr: int) -> TrieNode | None:
    """Decode a single trie node at the given address."""
    if addr >= len(data):
        return None
    
    pos = addr
    header = data[pos]
    pos += 1
    
    # Parse header byte
    has_levels = (header >> 7) != 0
    stride_sel = (header >> 5) & 0x03
    stride = stride_sel if stride_sel != 0 else 1
    child_count = header & 0x1F
    
    # Handle overflow child count
    if child_count == 31:
        if pos >= len(data):
            return None
        child_count = data[pos]
        pos += 1
    
    # Parse levels info if present
    levels_offset = None
    levels_len = 0
    levels = []
    if has_levels:
        if pos + 1 >= len(data):
            return None
        offset_hi = data[pos]
        offset_lo_len = data[pos + 1]
        pos += 2
        
        # 12-bit offset (hi<<4 | top nibble of second byte)
        levels_offset = (offset_hi << 4) | (offset_lo_len >> 4)
        levels_len = offset_lo_len & 0x0F
        
        # Decode the actual level entries
        if levels_offset + levels_len <= len(data):
            for i in range(levels_len):
                packed = data[levels_offset + i]
                dist = packed // 10
                score = packed % 10
                levels.append(LevelEntry(dist=dist, score=score, raw_byte=packed))
    
    # Parse transitions (labels)
    if pos + child_count > len(data):
        return None
    transition_labels = list(data[pos:pos + child_count])
    pos += child_count
    
    # Parse target deltas
    targets_size = child_count * stride
    if pos + targets_size > len(data):
        return None
    
    transitions = []
    for i in range(child_count):
        label = transition_labels[i]
        delta_bytes = data[pos + i * stride:pos + (i + 1) * stride]
        delta = decode_delta(delta_bytes, stride)
        target_addr = addr + delta
        
        # Convert label to readable char
        if 32 <= label <= 126:
            label_char = chr(label)
        else:
            label_char = f"\\x{label:02x}"
        
        transitions.append(Transition(
            label=label,
            label_char=label_char,
            delta=delta,
            target_addr=target_addr
        ))
    
    return TrieNode(
        addr=addr,
        has_levels=has_levels,
        stride=stride,
        child_count=child_count,
        transitions=transitions,
        levels_offset=levels_offset,
        levels_len=levels_len,
        levels=levels,
        raw_header=data[addr:pos + targets_size]
    )


def walk_trie(data: bytes, root_offset: int) -> list[TrieNode]:
    """Walk the trie starting from root and collect all reachable nodes."""
    visited = set()
    nodes = []
    queue = [root_offset]
    
    while queue:
        addr = queue.pop(0)
        if addr in visited or addr < 0 or addr >= len(data):
            continue
        visited.add(addr)
        
        node = decode_node(data, addr)
        if node is None:
            continue
        
        nodes.append(node)
        
        # Queue child nodes
        for trans in node.transitions:
            if trans.target_addr not in visited:
                queue.append(trans.target_addr)
    
    # Sort by address for cleaner output
    nodes.sort(key=lambda n: n.addr)
    return nodes


def print_node(node: TrieNode, verbose: bool = False) -> None:
    """Print a single node's information."""
    print(f"\n{'='*60}")
    print(f"Node @ 0x{node.addr:04X} (addr={node.addr})")
    print(f"  stride={node.stride}, children={node.child_count}, has_levels={node.has_levels}")
    
    if node.has_levels and node.levels:
        print(f"  levels_offset=0x{node.levels_offset:03X}, levels_len={node.levels_len}")
        print(f"  levels: ", end="")
        level_strs = [f"(dist={l.dist}, score={l.score})" for l in node.levels]
        print(", ".join(level_strs))
    
    if node.transitions:
        print(f"  transitions:")
        for t in node.transitions:
            print(f"    '{t.label_char}' (0x{t.label:02X}) -> delta={t.delta:+d} -> target=0x{t.target_addr:04X}")
    
    if verbose:
        print(f"  raw: {node.raw_header.hex()}")


def analyze_levels_tape(data: bytes, nodes: list[TrieNode]) -> None:
    """Analyze and print the levels tape usage."""
    print("\n" + "="*60)
    print("LEVELS TAPE ANALYSIS")
    print("="*60)
    
    # Find all unique levels offsets
    levels_refs = {}
    for node in nodes:
        if node.has_levels and node.levels_offset is not None:
            key = (node.levels_offset, node.levels_len)
            if key not in levels_refs:
                levels_refs[key] = []
            levels_refs[key].append(node.addr)
    
    # Sort by offset
    sorted_refs = sorted(levels_refs.items(), key=lambda x: x[0][0])
    
    print(f"Found {len(sorted_refs)} unique level sequences:")
    for (offset, length), node_addrs in sorted_refs[:50]:  # Limit output
        raw = data[offset:offset + length]
        entries = [(b // 10, b % 10) for b in raw]
        entries_str = ", ".join(f"d{d}s{s}" for d, s in entries)
        print(f"  @0x{offset:03X} len={length}: [{entries_str}] (used by {len(node_addrs)} nodes)")
    
    if len(sorted_refs) > 50:
        print(f"  ... and {len(sorted_refs) - 50} more")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Parse and list all packed data in a hypher hyphenation trie .bin file"
    )
    parser.add_argument("input", type=Path, help="Path to the .bin trie file")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show raw bytes")
    parser.add_argument("-n", "--max-nodes", type=int, default=100,
                        help="Maximum number of nodes to print (default: 100, use 0 for all)")
    parser.add_argument("--stats-only", action="store_true", 
                        help="Only show statistics, not individual nodes")
    args = parser.parse_args()
    
    # Read the binary file
    data = args.input.read_bytes()
    print(f"File: {args.input}")
    print(f"Total size: {len(data)} bytes")
    
    # Parse root offset (first 4 bytes, big-endian)
    if len(data) < 4:
        print("Error: File too small (< 4 bytes)")
        sys.exit(1)
    
    root_offset = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
    print(f"Root offset: 0x{root_offset:04X} ({root_offset})")
    
    if root_offset >= len(data):
        print(f"Error: Root offset {root_offset} exceeds file size {len(data)}")
        sys.exit(1)
    
    # Walk and collect all nodes
    print("\nWalking trie from root...")
    nodes = walk_trie(data, root_offset)
    print(f"Found {len(nodes)} reachable nodes")
    
    # Statistics
    print("\n" + "="*60)
    print("STATISTICS")
    print("="*60)
    
    nodes_with_levels = [n for n in nodes if n.has_levels]
    total_transitions = sum(n.child_count for n in nodes)
    stride_counts = {1: 0, 2: 0, 3: 0}
    for n in nodes:
        stride_counts[n.stride] = stride_counts.get(n.stride, 0) + 1
    
    print(f"Total nodes: {len(nodes)}")
    print(f"Nodes with levels: {len(nodes_with_levels)}")
    print(f"Total transitions: {total_transitions}")
    print(f"Stride distribution: 1-byte={stride_counts[1]}, 2-byte={stride_counts[2]}, 3-byte={stride_counts[3]}")
    
    # Analyze levels tape
    analyze_levels_tape(data, nodes)
    
    if args.stats_only:
        return
    
    # Print individual nodes
    print("\n" + "="*60)
    print("NODE DETAILS")
    print("="*60)
    
    max_nodes = args.max_nodes if args.max_nodes > 0 else len(nodes)
    for node in nodes[:max_nodes]:
        print_node(node, args.verbose)
    
    if len(nodes) > max_nodes:
        print(f"\n... and {len(nodes) - max_nodes} more nodes (use --max-nodes 0 to show all)")


if __name__ == "__main__":
    main()
