# TM4C123 LED Blinky with UART Control

A TM4C123GH6PM LaunchPad program that blinks the on-board RGB LED at a
configurable rate and colour, shows that state on a 4-digit multiplexed
7-segment display, and lets you control everything over a UART serial
connection instead of (or alongside) the two on-board switches.

## What it does

- Blinks the RGB LED (Port F) at one of 8 speeds and one of 8 colours.
- Drives a 4-digit 7-segment display showing: a fixed `S` label, the
  current rate, the current colour, and `r` (running) or `P` (paused).
- SW1 (on-board) cycles the blink rate; SW2 cycles the colour; pressing
  both together pauses or resumes.
- UART0 (115200-8-N-1) mirrors and controls the same state from a PC
  terminal.

## Hardware

- TM4C123GH6PM LaunchPad
- 4-digit multiplexed 7-segment display on Port A (digit select,
  PA4-PA7) and Port B (segments, PB0-PB7)
- On-board switches SW1 (PF4) and SW2 (PF0), on-board RGB LED (PF1-PF3)
- USB connection to the LaunchPad's debug port doubles as the UART0
  serial link (no extra wiring needed)

## Building

Standard Code Composer Studio project using the GNU ARM compiler.
`main.c` is self-contained — no external driverlib/TivaWare library is
required, only the project's own `inc/tm4c123gh6pm.h`.

## Using the UART interface

1. Flash the board.
2. Open a serial terminal (CCS's built-in terminal, PuTTY, Tera Term,
   etc.) on the LaunchPad's virtual COM port.
3. Settings: **115200 baud, 8 data bits, no parity, 1 stop bit.**
4. Type a command and press Enter:

   | Command  | Effect                                   |
   |----------|-------------------------------------------|
   | `RATE`   | Advances the blink rate by one (wraps 7→0) |
   | `COLOUR` | Advances the colour by one (wraps 7→0)     |
   | `PAUSE`  | Pauses the lights (no effect if already paused) |
   | `RESUME` | Resumes the lights (no effect if already running) |
   | `STATUS` | Reports the current rate, colour, and state |

Commands are **case-sensitive** and must be typed in capitals exactly
as shown above.

## Notes

- Switches and UART commands drive the same state, so either input is
  reflected immediately regardless of which one changed it.
- `PAUSE`/`RESUME` sent when the lights are already in that state print
  a short message instead of changing anything.
- The `STATUS` reply is preceded by the literal word `status` with no
  line break before it, so it appears run together with the status
  line that follows on the same line of your terminal — this is a
  known quirk of the current handler, not a hardware issue.
