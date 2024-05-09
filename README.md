# Accelerated File Explorer (Cile Explorer)

Written Completely in C and Zig, and using Zig as a build system. this project is meant to reach the max speed that a File Explorer is supposed to go using:

1. Cache the Folder/File System
2. Detect Changes and Update Cache
3. Cache using the least storage, and fastest speed
4. GPU Powered, Parrallel

# @TODO!:

### Performance Specific

-[] Create a Function that can be used to Get All Folders in a Directory
-[] Create a Function that Searches Recursively (Multi-Threaded) through every directory, and then pushes the checking to a Filter (Booyre Moore)
-[] Add the Option to Filter Folders using OpenCL while CPU continues the Searching

### UI Specific

-[] Create a Function that returns the Icon of a Folder/File
-[] Create the Sidebar using GTK
-[] Create the TopSearch Bar using GTK
-[] Create the Screen that shows files using GTK
