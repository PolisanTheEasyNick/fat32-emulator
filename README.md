# FAT32-Emulator

This emulator program can manage FAT32 filesystem backed up by a file.

## Building

```bash
$ cmake .
$ make
$ ./FAT32-emulator -f fat32.disk
```

## Command line parameters

* -f --file [path] - specifing I/O FAT32 file path
* -v --verbose - whether to print debug info
* -s --size - file size in bytes, defaults to 20971520 (20 MBs)

## Command line functions
* cd [path] - changes the current directory to specified directory. Only absolute path is allowed.  
* format - creates FAT32 filesystem inside the file.  
* ls [path] - shows the files and directories inside a given directory, or in the current directory af a path is not specified.  
* mkdir [name] - creates a directory with a given name in the current directory.  
* touch [name] - creates a file with a given name in the current directory.  
