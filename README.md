# Embedded_miniHarp for Raspberry Pi Pico W

## Known Issues

### Issue 1: Memory Management
- **Problem**:
    - When using `TIME_WINDOW >= 0.01`, the system runs out of memory even with only 10 packets. GCC_PHAT also needs a lot of memory for `TIME_WINDOW >= 0.01`
    - With `TIME_WINDOW = 0.002`, the system can handle more packets, but it cannot process continuous inputs in real-time because the data buffer will be quickly overflow or out of memory.
    - The data processor becomes too slow to keep up with the listener's rate of adding packets to the buffer, leading to a memory overflow.

- **Root Cause**:
    - The buffer fills up faster than the system can process the packets, causing memory exhaustion.
    - GCC_PHAT needs more memory with bigger `TIME_WINDOW`

- **Potential Solution**:
    - Adjust the minimum allocated heap size in `memmap_default.ld`. However, the compiler can only allocate up to 125KB for heap memory, which limits the available space.
    - Reducing `TIME_WINDOW` in `custom_types.h`
    - Make listener also decode the packet before appending into a queue

---

### Issue 2: Lack of Persistent Storage
- **Problem**:
    - The Raspberry Pi Pico W lacks onboard storage to save output files.
    - The current setup requires an SD card and SD card adaptor to write files, which adds complexity.

- **Current Behavior**:
    - Instead of writing output to a file, the system currently only prints messages.

- **Potential Solution**:
  - Writing to SD card. Need to refer to pico w documentation.



---

### Issue 3: No Error Handling (No Try-Catch Mechanism)
- **Problem**:
    - The Raspberry Pi Pico W does not have a built-in `try-catch` mechanism for error handling.
    - When an exception occurs, the system hangs and stops functioning.

- **Future Work**:
    - Explore alternative methods for handling errors on the Pico W to prevent the system from hanging when an exception is encountered.
