# reram-tools

Tools to manage ReRAM/flash memories based on Linux SPIDEV API.

## Usage

A single executable provides access to different commands using symbolic link.


### rrdump

```
Usage: rrdump [options]
Options

    -h, --help        show this help message and exit
    -b, --spi=<str>   SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)
    -s, --freq=<int>  SPI clock frequency in MHz (default: 1 - 1 MHz)
    --start=<int>     start address to dump (default: 0)
    --end=<int>       end address to dump (default: 256)
    -f, --full        full dump
```

### rrfill

```
Usage: rrfill [options]
Options

    -h, --help        show this help message and exit
    -b, --spi=<str>   SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)
    -s, --freq=<int>  SPI clock frequency in MHz (default: 1 - 1 MHz)
    -c, --byte=<int>  byte to fill whole memory (default: 0)
    -y, --yes         do not ask confirmation
```

### rrinfo

```
Usage: rrinfo [options]
Options

    -h, --help        show this help message and exit
    -b, --spi=<str>   SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)
    -s, --freq=<int>  SPI clock frequency in MHz (default: 1 - 1 MHz)
```

### rrread

```
Usage: rrread [options]
Options

    -h, --help        show this help message and exit
    -b, --spi=<str>   SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)
    -s, --freq=<int>  SPI clock frequency in MHz (default: 1 - 1 MHz)
    -a, --addr=<int>  address to read (default: 0)
    -n, --nb=<int>    number of bytes to read (default: 1)
```

### rrwrite

```
Usage: rrwrite [options]
Options

    -h, --help        show this help message and exit
    -b, --spi=<str>   SPI bus number and chip select (default: 0.0 - /dev/spidev0.0)
    -s, --freq=<int>  SPI clock frequency in MHz (default: 1 - 1 MHz)
    -a, --addr=<int>  address to write (default: 0)
    -c, --byte=<int>  byte to write (default: 0)
    -f, --file=<str>  file to write
    -y, --yes         do not ask confirmation
```
