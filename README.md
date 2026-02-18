# LAN8651 BSP Patcher

This repository contains scripts and patches for building and customizing a Microchip BSP (Board Support Package) with LAN8651 Ethernet PHY support and AIoT (Artificial Intelligence of Things) customizations.

## Overview

The BSP Patcher automates the process of downloading, patching, and building a customized version of the Microchip BSP with specific network configurations, MQTT support, and update mechanisms tailored for embedded AIoT applications.

## Architecture

```
┌─────────────────────┐    ┌──────────────────────┐    ┌─────────────────────┐
│  Build Script       │    │  Patch File          │    │  Target Hardware    │
│  build_with_        │───▶│  prebuild_lan8651_   │───▶│  LAN966x + LAN8651  │
│  lan8651_patch.sh   │    │  customizations...   │    │  Embedded System    │
└─────────────────────┘    └──────────────────────┘    └─────────────────────┘
```

## Files

### Core Files
- `build_with_lan8651_patch.sh` - Main build automation script
- `prebuild_lan8651_customizations_clean.patch` - Comprehensive patch with AIoT customizations

## Quick Start

### Prerequisites

- Linux system with bash shell
- Git (for patch application)
- Either `curl` or `wget` for downloads
- Internet access for BSP download (~2GB)
- Sufficient disk space (>10GB recommended)

### Basic Usage

```bash
# Make the script executable
chmod +x build_with_lan8651_patch.sh

# Run the build process
./build_with_lan8651_patch.sh
```

The script will:
1. Download the MSC BSP source tarball (2025.12 version)
2. Extract the source code
3. Apply LAN8651 customizations
4. Build the complete BSP with buildroot

## Build Process Details

### Step 1: BSP Download
- **Source**: `http://mscc-ent-open-source.s3-eu-west-1.amazonaws.com/public_root/bsp/mscc-brsdk-source-2025.12.tar.gz`
- **Size**: ~2GB compressed
- **Target Directory**: `mchp-brsdk-source-2025.12`

### Step 2: Patch Application
The patch includes multiple customizations:

#### Network Configuration
- **eth0**: Static IP `192.268.0.5/16` (AIoT network segment)
- **eth1**: Static IP `192.168.178.20/16` (Management network)
- **eth2**: Intentionally unconfigured (flexible use)

#### MQTT Broker Setup
Mosquitto MQTT broker with:
- Port: 1883
- Anonymous access enabled
- Persistent message storage
- Suitable for AIoT device communication

#### Device Tree Modifications
- LAN8651 PHY configuration for 40-pin adapter
- GPIO36 interrupt configuration for proper PHY operation

### Step 3: Build Configuration
- Target: ARM standalone configuration
- Build system: Buildroot
- External tree: `./external`
- Output directory: `./output/mybuild`

## Customizations in Detail

### 1. Network Interface Setup
Location: `board/mscc/common/rootfs_overlay/etc/network/interfaces`

```ini
auto eth0
iface eth0 inet static
    address 192.268.0.5      # AIoT network
    netmask 255.255.0.0

auto eth1
iface eth1 inet static
    address 192.168.178.20   # Management network
    netmask 255.255.0.0
```

### 2. MQTT Broker Configuration
Location: `board/mscc/common/rootfs_overlay/etc/mosquitto/mosquitto.conf`

Features:
- Anonymous publishing/subscribing
- Message persistence
- Suitable for AIoT sensor data collection

### 3. U-Boot Environment Configuration
Location: `board/mscc/common/rootfs_overlay/etc/fw_env.config`

Enables Linux-based U-Boot environment manipulation:
- Primary environment: 0x180000 (1.5MB offset)
- Backup environment: 0x1C0000 (1.75MB offset)
- Dual-slot boot support for reliable updates

### 4. Update System
Location: `board/mscc/common/rootfs_overlay/sbin/update.sh`

Comprehensive A/B partition update system:
- TFTP-based image download
- Automatic slot detection (mmcblk0p5 ↔ mmcblk0p6)
- Failsafe dual-boot configuration
- Automated U-Boot environment switching

### 5. Security Configuration
- Root password: `microchip` (configurable in buildroot)
- SSH access via Dropbear
- Web server: Hiawatha

## Hardware Configuration

### Target Platform
- **SoC**: Microchip LAN966x series
- **PHY**: LAN8651 (10BASE-T1L Ethernet PHY)
- **Board**: PCB8291 or compatible
- **Interface**: 40-pin adapter connection

### LAN8651 PHY Settings
- **SPI Interface**: Up to 15MHz
- **Interrupt**: GPIO36 (falling edge)
- **Protocol**: 10BASE-T1L (10 Mbps over single pair)

## Build Output

After successful build completion:
```
./mchp-brsdk-source-2025.12/output/mybuild/images/
├── rootfs.squashfs          # Root filesystem
├── rootfs.tar               # Root filesystem archive  
├── u-boot.bin              # U-Boot bootloader
├── zImage                  # Linux kernel
└── *.dtb                   # Device tree binaries
```

## Update Procedure

### Development Workflow
1. Build new image using this patcher
2. Extract `rootfs.ext4.gz` from build output
3. Set up TFTP server with the image
4. Run update script on target device

### Target Device Update
```bash
# On the embedded device
/sbin/update.sh

# Follow prompts for:
# - Local IP configuration
# - TFTP server IP
# - Automatic image download and installation
```

### Manual U-Boot Configuration
```bash
# Stop U-Boot during boot
# At U-Boot prompt:
setenv mmc_cur 6    # Switch to partition 6
setenv mmc_bak 5    # Set partition 5 as backup
saveenv             # Save configuration
boot                # Continue boot
```

## Troubleshooting

### Common Issues

#### Build Failures
- **Missing dependencies**: Ensure all buildroot prerequisites are installed
- **Disk space**: Verify >10GB available space
- **Network issues**: Check firewall settings for BSP download

#### Network Configuration
- **IP conflicts**: Verify network segments don't conflict with existing infrastructure
- **AIoT connectivity**: Ensure 192.268.0.x subnet is routed properly

#### Update Problems
- **TFTP timeout**: Verify server accessibility and file presence
- **Partition errors**: Check eMMC health and partition table integrity
- **Boot failures**: Use U-Boot console to manually switch partitions

### Debug Commands
```bash
# Check network status
ip addr show

# Verify MQTT broker
mosquitto_sub -t test/topic

# U-Boot environment status
fw_printenv mmc_cur mmc_bak

# Partition information
fdisk -l /dev/mmcblk0
```

## Development Notes

### Customization Points
- **Network addresses**: Modify `interfaces` file in patch
- **MQTT settings**: Adjust `mosquitto.conf` parameters
- **Build options**: Edit `arm_standalone_defconfig`
- **Hardware settings**: Modify Device Tree overlays

### Adding New Features
1. Modify the patch file with new configurations
2. Test patch application: `git apply --check prebuild_lan8651_customizations_clean.patch`
3. Rebuild using the main script

## License & Attribution

This patcher is based on:
- **Microchip BSP**: Official Microchip LAN966x BSP (2025.12)
- **Buildroot**: Cross-compilation framework
- **Device Tree**: LAN8651 PHY configurations

## Support

For AIoT-specific customizations or integration support, consult the documentation of the underlying BSP components and the LAN8651 datasheet for hardware-specific configurations.

---
*Generated for BSP version 2025.12 with LAN8651 AIoT customizations*