# Introduction

This repository is a project for ** Introduction to Database Management Systems**. The project explores and analyzes the performance of various indexing techniques used in database systems. Indexing is a critical aspect of database management, as it significantly impacts query performance by reducing the number of disk I/O operations required.

The project analyses various different indexing techniques and here we present our simulation of the major three indexing techniques:
1. **Bitmap Indexing**
2. **B+ Tree Indexing**
3. **Hash Indexing**

Each indexing technique is implemented and simulated to measure disk seek and transfer metrics for different types of queries. The goal is to understand the trade-offs and efficiency of these indexing methods in various scenarios, such as equality queries, range queries, and logical operations.

# File Descriptions

## `bitmapIndex.cpp`
This file implements a bitmap indexing system. It allows for efficient querying of database records by creating bitmaps for specific column values. The bitmaps are stored in blocks, and operations like equality, AND, and OR queries are supported. Disk access metrics such as seeks and transfers are tracked to simulate real-world performance.

## `btreeIndex.cpp`
This file implements a B+ Tree indexing system. It supports insertion, equality search, and range queries (e.g., less than or greater than). The B+ Tree is designed to optimize disk access by maintaining a linked list of leaf nodes for efficient traversal.

## `hashIndex.cpp`
This file implements a static hash indexing system. It uses a hash function to map keys to buckets and handles overflow using linked overflow blocks. The implementation tracks disk access metrics and supports insertion and search operations.

# Assumptions
## Bitmap Indexing
1. The database records are stored sequentially so that when we create a bitmap, we only need one seek to get to the first entry and then all the entries are read by simply incrementing the pointer.
2. The RAM has enough storage to store the bitmap for an entire column worth so that the entire bitmap can be calculated one by one and then written to disk only once.
3. The bitmaps are stored in a different partition whose block can only store `x` bits, whereas the database entries are stored in a different partition where `y` tuples per block are stored. This is done due to the variable size of the database entries and for ease of simulation.

## B+Tree Indexing
1. There are no duplicate values in the database.
2. All the entries of a leaf node are stored in the same data block.
