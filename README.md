# UART Terminal — Linux UART Interface via termios API

A lightweight C utility for configuring and communicating over a UART serial interface on Linux using the `termios` API and non-blocking I/O with `poll()`.

## Features

- Configure UART parameters: baud rate, data bits, parity, stop bits
- Transmit a test message over the UART interface
- Receive incoming data with a 3-second timeout via `poll()`
- Raw mode — no data alteration by the OS
- Graceful error handling for invalid paths, permission issues, and read/write failures

## Requirements

- Linux OS
- GCC
- Access to a UART device (e.g. `/dev/ttyUSB0`, `/dev/ttyS0`)

> You may need to add yourself to the `dialout` group to access serial devices without `sudo`:
> ```bash
> sudo usermod -aG dialout $USER
> ```
> Log out and back in for the change to take effect.

## Build

```bash
make
```

For a debug build (includes GDB symbols and `DEBUG` macro):

```bash
make debug
```

To clean up:

```bash
make clean
```

## Usage

```bash
./uart -a <device> [-b baud_rate] [-p parity] [-s stop_bits] [-d data_bits]
```

### Options

| Flag | Description                        | Default    |
|------|------------------------------------|------------|
| `-a` | Device path (required)             | —          |
| `-b` | Baud rate                          | `115200`   |
| `-p` | Parity: `N` (none), `E` (even), `O` (odd) | `N` |
| `-s` | Stop bits: `1` or `2`             | `1`        |
| `-d` | Data bits: `5`, `6`, `7`, or `8`  | `8`        |

### Supported Baud Rates

`9600`, `19200`, `38400`, `57600`, `115200`, `230400`, `4000000`

## Examples

Standard 8N1 configuration at 115200 baud:
```bash
./uart -a /dev/ttyUSB0 -b 115200 -p N -s 1 -d 8
```

Even parity, 2 stop bits:
```bash
./uart -a /dev/ttyUSB0 -b 9600 -p E -s 2 -d 8
```

## Project Structure

```
.
├── uart.c      # Main implementation
├── uart.h      # Types, baud rate table, and includes
├── Makefile    # Build system
└── README.md   # This file
```

## Testing Without Hardware

You can test the program using a virtual serial port pair with `socat`:

```bash
# Terminal 1 — create a virtual serial port pair
socat -d -d pty,raw,echo=0 pty,raw,echo=0
# socat will print two device paths, e.g. /dev/pts/2 and /dev/pts/3

# Terminal 2 — listen on one end
cat /dev/pts/3

# Terminal 3 — run the program on the other end
./uart -a /dev/pts/2
```
