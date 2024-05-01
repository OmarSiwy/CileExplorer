# Accelerated File Explorer (Cile Explorer)

Written Completely in C, and using Zig as a build system. this project is meant to reach the max speed that a File Explorer is supposed to go using:

1. Cache the Folder/File System
2. Detect Changes and Update Cache
3. Cache using the least storage, and fastest speed

# For Development:

1. Install Zig

# To Build:

```bash
zig build
```

# To Run:

```bash
zig build run
```

# To Test:

```bash
zig build test
```

# To Get Compile_COmmands.json for intellisense

```bash
zig build cdb
```

# @TODO!:

- [] Add GTK into the Zig Build System
- [] Create a File System Abstraction for support on multiple OS
- [] Create a UI using GTK that works with it
