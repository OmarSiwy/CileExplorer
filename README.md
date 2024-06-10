# Accelerated File Explorer (Cile Explorer)

Written Completely in C and Zig, using Zig as a build system. this project is meant to reach the max speed that a File Explorer is supposed to go above and beyond using these new techniques:

1. String comparisong using aho-corasick <= (GPU Powered)
2. Cache the Folder/File System to avoid researching same directory if already searched (**working on this currently**)

# Why Zig?

Zig Offers a much smaller build option rather than having different C Compilers for each Operating System (Result: 200mb build system -> 40mb)

Zig Also Offers in-house testing by importing C header files straight into it for testing, this beats Google Testing for C and removes the need for a C++ Compiler on board.

# @TODO!:

### Performance Specific

-[] Add the Option to Filter Folders using OpenCL while CPU continues the Searching

### UI Specific

-[] Create a Function that returns the Icon of a Folder/File
-[] Create the Sidebar using GTK
-[] Create the TopSearch Bar using GTK
-[] Create the Screen that shows files using GTK
