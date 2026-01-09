# How to use Fat32Format

# Windows
1. Download the ``Fat32Format.exe`` binary in the [releases section](https://github.com/Germ-99/Fat32Format/releases)
2. Run it
3. Choose a drive to format and then format it

**NOTE: Whatever drive you format will be completely wiped** 

# Linux
Assuming you already have fat32format installed (if you don't, go [here](https://github.com/Germ-99/Fat32Format/tree/main?tab=readme-ov-file#linux)), this tool is used via the terminal on linux.

For just formatting the drive, the command is as simple as:
```
sudo fttf <device>
```

Example:
```
sudo fttf /dev/sda
```


This tool also has 3 flags that can optionally be used.
- ``--force`` - Will skip confirmation and just do the operation. Example: ``sudo fttf /dev/sda --force``

- ``--verbose`` - Will show more information during the operation. Example: ``sudo fttf /dev/sda --verbose``

These flags can also be used together
```
sudo fttf /dev/sda --verbose --force
```

- ``--help`` - This just shows some information on the tool.

Expected output:
```
Usage: fttf [OPTIONS] <device>

Options:
  -f, --force     Skip confirmation prompt
  -v, --verbose   Show detailed progress information
  -h, --help      Display this help message

Example:
  fttf /dev/sdb
  fttf -f -v /dev/sdc

WARNING: This will erase all data on the specified device!
```

These flags are also available with short style naming and work the same way

- ``--force`` = ``-f``
- ``--verbose`` = ``-v``
- ``--help`` = ``-h``
