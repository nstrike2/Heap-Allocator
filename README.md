# Heap-Allocator
An implicit and explicit implementation of a heap allocator. Programmed in C.

An implicit allocator entails managing free heap space through a first-fit method over the total number of blocks in the heap.

An explicit allocator entails managing free heap space through a first-fit method over a linked list of free blocks in the heap, optimizing throughput. In addition, the explicit allocator, unlike the implicit, supports coalescing of free blocks and an in-place realloc (also utilizing coalescing) to improve utilization.


The project also includes a test_harness file, which reads and interprets text-based script files (that the user can create and input) containing a sequence of allocator requests. Allocator requests are formatted as follows:

---

A script file containing:

a 0 24

a 1 100

f 0

r 1 300

f 1

is converted into these calls in an allocator:

void *ptr0 = mymalloc(24);

void *ptr1 = mymalloc(100);

myfree(ptr0);

ptr1 = myrealloc(ptr1, 300);

myfree(ptr1);

---

The test_harness runs the allocator and its various functionalities (mymallc, myrealloc, myfree) on a script and validates results (validate_heap) for correctness. When compiled using "make", it will create 3 different compiled versions of this program, one using each type of heap allocator (bump, implicit, and explicit).

Hope you enjoy!
