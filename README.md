# Heap-Allocator
An implicit and explicit implementation of a heap allocator. Programmed in C.
An implicit allocator entails managing free heap space through a first-fit method over the total number of blocks in the heap.
An explicit allocator entails managing free heap space through a first-fit method over the a linked list of free blocks in the heap, optimizing throughput. In addition, the explicit allocator, unlike the implicit, supports coalescing of free blocks and an in-place realloc (also utilizing coalescing) to improve utilization.
Hope you enjoy!
