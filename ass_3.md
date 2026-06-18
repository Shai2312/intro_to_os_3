# OS262 Assignment 3 – Memory Management

**Course:** Operating Systems (202.1.3031)
**Semester:** Spring 2026

---

## Introduction

In this assignment, you will extend xv6 with a display subsystem and implement two different ways for userspace processes to draw pixels:

1. **Memory-Mapped I/O**
2. **Zero-Copy Page Flipping**

Along the way, you will gain experience with:

* xv6 virtual memory management
* Page table manipulation
* Device drivers
* Kernel ↔ User interfaces
* Memory-mapped devices

This assignment uses a customized xv6 template (different from the standard xv6-riscv used in previous assignments).

### Repository

https://github.com/BGU-CS-OS/os262-assignment3-dist

---

# Submission Instructions

* Ensure your code compiles without warnings or errors.
* Verify that all tasks work correctly.
* Submit **one xv6 project** containing all modifications.
* Submit as a `.zip` or `.tar.gz`.
* All tasks must coexist in the same xv6 tree.
* Use Git and commit frequently.
* Theoretical questions are **not submitted**, but you should understand them for grading.
* Submission is allowed **only in pairs** via Moodle.
* Email submissions will not be accepted.

Before submitting:

```bash
make clean
```

This removes generated files and the `obj` directory.

---

# Background

When running the supplied xv6 template, a display window will appear.

At boot you should see:

```text
Hello World
```

Demo programs:

```bash
show_map    # Task 1
show_flip   # Task 2
```

---

## Display Device and Framebuffer

QEMU provides an emulated display device.

The driver (`kernel/virtio_gpu.c`) owns a framebuffer consisting of:

* `GPU_FB_PAGES = 300`
* Resolution: `640 × 480`
* Pixel format: 32-bit pixels

Available driver functions:

```c
void virtio_gpu_init(void);
void virtio_gpu_commit(void);
```

### Notes

* The device reads pixels from a backing list of physical pages.
* Helper functions already exist to attach/detach backing pages.
* Understanding these functions is required for Task 2.

---

## Display Daemon

A kernel daemon named:

```c
display_daemon
```

calls:

```c
virtio_gpu_commit();
```

approximately every:

```text
100 ms (~10 FPS)
```

Any changes written into the framebuffer become visible on the next daemon tick.

---

# Task 1 – Memory-Mapped Framebuffer

## Goal

Allow a userspace process to write directly into the framebuffer without making system calls for every pixel operation.

The framebuffer pages should be mapped into userspace with:

```c
PTE_U | PTE_R | PTE_W
```

permissions.

---

## System Call

Implement:

```c
sys_map_display()
```

in:

```text
kernel/sysproc.c
```

### Behavior

The syscall receives:

```c
addr
```

### Case 1: addr == 0

The kernel:

* Chooses a suitable page-aligned virtual address
* Places it above `p->sz`
* Ensures enough space for the framebuffer
* Creates the mapping
* Returns the chosen virtual address

### Case 2: addr != 0

Requirements:

* Must be page-aligned
* Region:

```text
[addr,
 addr + GPU_FB_PAGES * PGSIZE)
```

must not overlap existing mappings

Kernel should:

* Install mapping
* Return `addr`

### Failure

Return:

```c
-1
```

for:

* Invalid alignment
* Address collision
* Out-of-memory
* Any other failure

---

## Testing

Run:

```bash
show_map
```

Expected behavior:

```text
show_map: type text and press Enter to display it.
Type 'exit' to clear the screen and quit.

> Hello World
```

The text should appear on the display.

---

## Running the Display

Use:

```bash
make qemu-web
```

Then open:

```text
http://localhost:6080/vnc_auto.html
```

---

## Requirements

1. Map framebuffer pages into userspace.
2. Implement `sys_map_display`.
3. Safely unmap framebuffer pages.
4. Verify using `show_map`.

### Files Expected for Submission

* `kernel/virtio_gpu.c`
* `kernel/sysproc.c`
* Any additional modified files

---

# Task 2 – Zero-Copy Page Flip

## Motivation

Task 1 shares a single framebuffer between:

* User process
* Display device

This can cause:

```text
Screen tearing
```

when the display refreshes while rendering is still in progress.

---

## Solution: Double Buffering

Instead of drawing directly into the displayed framebuffer:

1. Draw into a back buffer.
2. Atomically swap buffers.

This avoids partially rendered frames.

---

## Zero-Copy Flip

Instead of copying pixel data:

* The GPU backing list is updated.
* The display device is redirected to a new set of physical pages.

No framebuffer contents are copied.

---

## Implement

### virtio_gpu_flip

In:

```text
kernel/virtio_gpu.c
```

### sys_flip_display

In:

```text
kernel/sysproc.c
```

---

## sys_flip_display Requirements

1. Read:

```c
void *buf
```

from userspace.

2. Verify:

* Page aligned
* All `GPU_FB_PAGES` pages exist
* User-accessible

3. Call:

```c
virtio_gpu_flip(...)
```

4. Return:

```c
0
```

on success.

Return:

```c
-1
```

on failure.

---

## Double Buffering Example

```c
uint32 *buf[2];

buf[0] = (uint32 *)sbrk(FB_BYTES);
buf[1] = (uint32 *)sbrk(FB_BYTES);

for (int i = 0; i < FB_BYTES / 4; i++) {
    buf[0][i] = 0x00FF0000; // Red
    buf[1][i] = 0x000000FF; // Blue
}

for (int i = 0; i < 50; i++) {
    flip_display(buf[i & 1]);
    sleep(1);
}
```

### Notes

* `malloc()` does not guarantee page alignment.
* Use `sbrk()` to obtain page-aligned buffers.
* The display alternates between red and blue frames.

---

## Testing

Run:

```bash
show_flip Hello World
```

Expected behavior:

* Text appears immediately on the display.

---

## Requirements

1. Implement `virtio_gpu_flip`.
2. Implement `sys_flip_display`.
3. Verify using `show_flip`.

### Files Expected for Submission

* `kernel/virtio_gpu.c`
* `kernel/sysproc.c`

---

# Task 3 – Conway's Game of Life

A complete implementation is provided in:

```text
user/gol.c
```

The program supports both display mechanisms.

---

## Run

### Flip Mode

```bash
gol
```

or

```bash
gol flip
```

### Memory-Mapped Mode

```bash
gol map
```

---

## Expected Behavior

* Animate 100 generations.
* Display updates smoothly.
* Program exits cleanly.

### Submission

No additional files required.

---

# Questions (Not for Submission)

1. What is the difference between `map_display` and `flip_display` from the kernel's perspective?
2. Why must `flip_display` walk page tables instead of passing a user virtual address directly to the GPU?
3. Why is double buffering important?
4. What happens if the display daemon is not running?
5. What happens if a process exits after calling `flip_display`?
6. Compare memory bandwidth between:

   * `flip_display`
   * a syscall that copies the entire framebuffer
7. Is there a race condition when reading from the back buffer during commits?

---

# Deliverables Summary

## Task 1

* Framebuffer mapping support
* `sys_map_display`

## Task 2

* `virtio_gpu_flip`
* `sys_flip_display`

## Task 3

* Verify `gol flip`
* Verify `gol map`

---

Good luck and have fun exploring xv6 memory management and graphics programming!
