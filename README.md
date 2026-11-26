# TCP Loopback Communication on QEMU ARM with Buildroot Linux

A small embedded Linux project: I built an ARM Linux image with Buildroot, booted it in QEMU (ARM VersatilePB), and wrote a TCP client and server in C that I cross-compiled and ran on the emulated board.

I did this as a technical assignment for RN Embedded Solutions (June 2026). The goal was to learn the whole embedded Linux workflow end to end, without needing real ARM hardware.

![Server and client output](screenshots/server%26client_outputs.png)

## What I built

- A bootable ARM Linux system (kernel, device tree and root filesystem) generated with Buildroot
- A QEMU virtual machine running that system on an emulated ARM926EJ-S CPU
- `server.c`: a TCP echo server listening on port 5000
- `client.c`: a client that sends `Hello RN Embedded` and prints what the server echoes back
- Both programs cross-compiled on my x86_64 Ubuntu machine and copied into the VM's root filesystem

When it works, you see:

```
Waiting for client...
Received: Hello RN Embedded
Loopback Received: Hello RN Embedded
```

## Environment

| | Host | Target (QEMU) |
|---|---|---|
| OS | Ubuntu 20.04.4 LTS, x86_64 | Buildroot Linux 2026.05-rc3 |
| Kernel | n/a | Linux 6.18.7 |
| CPU | n/a | ARM926EJ-S (ARMv5TEJ) |
| RAM | 5.7 GB | 256 MB |
| Tools | QEMU 4.2.1 | arm-buildroot-linux-gnueabi-gcc 14.3.0 |

## Repository layout

```
application/     server.c, client.c
documentation/   full project report (PDF)
screenshots/     terminal screenshots from the run
```

## How I did it

### 1. Build the ARM image with Buildroot

```bash
cd ~/rn_assignment/buildroot
make versatile_defconfig
make
```

This produced three files in `output/images/`: `zImage` (kernel), `versatile-pb.dtb` (device tree) and `rootfs.ext2` (root filesystem).

### 2. Cross-compile the programs

The cross compiler was not on my `PATH`, so I called it by its full path:

```bash
cd ~/rn_assignment/buildroot
./output/host/bin/arm-buildroot-linux-gnueabi-gcc ~/loopback_app/server.c -o ~/loopback_app/server_arm
./output/host/bin/arm-buildroot-linux-gnueabi-gcc ~/loopback_app/client.c -o ~/loopback_app/client_arm
```

### 3. Put the binaries into the root filesystem

```bash
sudo mount -o loop output/images/rootfs.ext2 /mnt
sudo cp ~/loopback_app/server_arm /mnt/root/
sudo cp ~/loopback_app/client_arm /mnt/root/
sudo umount /mnt
```

### 4. Boot the VM

```bash
qemu-system-arm -M versatilepb -nographic -m 256M \
    -kernel output/images/zImage \
    -dtb output/images/versatile-pb.dtb \
    -drive file=output/images/rootfs.ext2,if=scsi,format=raw \
    -append "root=/dev/sda console=ttyAMA0"
```

Log in as `root` (no password). Press `Ctrl+A`, then `X` to quit QEMU.

### 5. Run the programs inside the VM

```bash
ip addr add 10.0.2.15/24 dev lo
/root/server_arm &
/root/client_arm
```

I run the server in the background with `&` so I can use one terminal for both programs (see the second problem below for why).

## How the programs work

The server creates a TCP socket, binds to port 5000 on all interfaces, listens, and accepts one client. It reads the message, prints it, and sends the same bytes back. The client creates a socket, connects to `10.0.2.15:5000`, sends `Hello RN Embedded`, reads the reply and prints it.

I picked TCP because it gives reliable, ordered delivery, so a matching echo shows the data arrived intact. Port 5000 is in the user range and does not clash with services already running in the VM.

## Networking in the VM

QEMU's default user-mode networking has a built-in DHCP server. When `eth0` comes up, the VM gets:

- IP `10.0.2.15`, mask `255.255.255.0`
- Gateway `10.0.2.2`, DNS `10.0.2.3`

I checked this with `ip addr`, `ip addr show eth0`, and `ping 10.0.2.2` (replies in about 1 to 2 ms).

## Problems I ran into

**1. Buildroot configuration errors.** My first builds failed because of wrong settings in `menuconfig`. I went through the options one by one for the VersatilePB board until the build finished and produced all the image files. Starting from `versatile_defconfig` was the fix.

**2. "Failed to get write lock" in QEMU.** I wanted the server and client in two terminals, so I started a second QEMU instance on the same disk image, and QEMU refused because the image was already in use. I fixed it by running the server in the background in one VM.

**3. `eth0` sometimes did not come up.** On some boots the log showed this:

```
Starting network: Waiting for interface eth0 to appear.......... timeout!
run-parts: /etc/network/if-pre-up.d/wait_iface: exit status 1
FAIL
```

I tried setting the address manually with `ifconfig eth0 10.0.2.15 ...` and got `No such device`, and `modprobe smc91x` did not find the module either. My workaround was `ip addr add 10.0.2.15/24 dev lo`. The client connects to `10.0.2.15`, and the kernel needs that address to belong to some interface on the machine, so putting it on the loopback interface let the client reach the server even with `eth0` down. When `eth0` does come up with DHCP, the address already exists and this step isn't needed. I have not fixed the underlying `eth0` problem. I'd look at the QEMU network device options next.

**4. `command not found` for the cross compiler.** The toolchain lives in `output/host/bin` and isn't on `PATH`. I used the full path.

**5. Getting binaries into the VM.** There is no package manager or network copy in this minimal image. I learned that I can loop-mount `rootfs.ext2` on the host and copy files into `/root` before booting.

## What I learned

- How Buildroot turns one configuration into a kernel, root filesystem and matching toolchain
- Why code built with the host `gcc` won't run on the ARM target, and how cross-compilation fixes that
- How QEMU emulates a board, and how its user-mode networking assigns addresses
- The TCP socket sequence on both sides: `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `close`
- Debugging a missing network interface when the obvious fix (setting the address on `eth0`) didn't work

## Screenshots

| | |
|---|---|
| Boot and Buildroot login | ![](screenshots/BUILTROOT_login.png) |
| `ip addr` output | ![](screenshots/ip_addr.png) |
| `eth0` details and gateway ping | ![](screenshots/eth0_ip.png) |
| Cross-compilation | ![](screenshots/compilation_output.png) |
| Server and client runs | ![](screenshots/server%26client_outputs.png) |
| The `eth0` timeout, then the `lo` workaround working | ![](screenshots/loopback_eth_output.png) |

## Known limitations

The code is deliberately minimal, since it was written for the assignment. Things I would improve next:

- Check the return values of `socket`, `bind`, `listen`, `accept`, `connect`, `send` and `recv`, and print errors with `perror()`
- Use the byte count from `recv()` instead of relying on the null terminator
- Add `SO_REUSEADDR` so the server can restart immediately
- Handle more than one client, and loop instead of exiting after one message
- Find the root cause of the intermittent `eth0` problem instead of using the `lo` workaround

## Full report

The complete write-up is in the [`documentation`](documentation/) folder.

## About me

Beere Bhavana, B.Tech Electronics and Communication Engineering at RGIT Kadapa (2023-2027), working towards a career in embedded engineering.


- Email: `bhavanabeere6@gmail.com
- `
