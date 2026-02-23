# LAN865x (LAN8651) T1S MAC-PHY Kernel-Treiber Architektur

**Chip:** Microchip LAN8651 (10BASE-T1L MAC-PHY)  
**Kernel Version:** Linux Custom (Microchip BSP 2025.12)  
**Datum:** Februar 2026

## Inhaltsverzeichnis

- [Überblick](#überblick)
- [Komponenten im Detail](#komponenten-im-detail)
  - [1. PHY-Treiber: `microchip_t1s.c`](#1-phy-treiber-microchip_t1sc)
    - [Unterstützte Chips](#unterstützte-chips)
    - [Funktionen](#funktionen)
    - [Kernel-Integration](#kernel-integration)
  - [2. MAC-PHY Treiber: `lan865x.c`](#2-mac-phy-treiber-lan865xc)
    - [Funktionen](#funktionen-1)
    - [Kernel-Integration](#kernel-integration-1)
    - [Private Struktur](#private-struktur)
  - [3. OA TC6 Framework: `oa_tc6.c`](#3-oa-tc6-framework-oa_tc6c)
    - [Standard-Konformität](#standard-konformität)
    - [Kern-Struktur](#kern-struktur)
    - [Exportierte API](#exportierte-api)
- [Kommunikationsarchitektur](#kommunikationsarchitektur)
  - [Physische Verbindung](#physische-verbindung)
  - [Software-Architektur](#software-architektur)
    - [Für MAC-Funktionen (Direct Path)](#für-mac-funktionen-direct-path)
    - [Für PHY-Funktionen (MDIO Bridge Path)](#für-phy-funktionen-mdio-bridge-path)
  - [MDIO-to-SPI Bridge](#mdio-to-spi-bridge)
  - [Register-Mapping](#register-mapping)
    - [OA TC6 Standard Register](#oa-tc6-standard-register)
    - [LAN865x MAC Register](#lan865x-mac-register)
- [Initialisierung und Binding](#initialisierung-und-binding)
  - [1. Device Tree Konfiguration](#1-device-tree-konfiguration)
  - [2. Treiber-Binding Sequence](#2-treiber-binding-sequence)
    - [MAC-PHY Treiber Probe](#mac-phy-treiber-probe)
    - [OA TC6 Initialization](#oa-tc6-initialization)
    - [PHY-Treiber Binding](#phy-treiber-binding)
- [Datenfluss](#datenfluss)
  - [TX (Transmit) Path](#tx-transmit-path)
  - [RX (Receive) Path](#rx-receive-path)
  - [PHY Management Path](#phy-management-path)
- [Kernel-Konfiguration](#kernel-konfiguration)
  - [Erforderliche Config-Optionen](#erforderliche-config-optionen)
  - [Automatische Aktivierung](#automatische-aktivierung)
- [PLCA (Physical Layer Collision Avoidance)](#plca-physical-layer-collision-avoidance)
  - [Device Tree Konfiguration](#device-tree-konfiguration)
  - [Runtime Kontrolle](#runtime-kontrolle)
  - [PHY-Treiber PLCA Support](#phy-treiber-plca-support)
- [Debugging und Diagnose](#debugging-und-diagnose)
  - [Kernel Log Messages](#kernel-log-messages)
  - [Register-Zugriff (Development)](#register-zugriff-development)
  - [Erweiterte Diagnose](#erweiterte-diagnose)
- [Zusammenfassung](#zusammenfassung)

## Überblick

Der **LAN8651** ist ein **MAC-PHY Chip** der sowohl MAC- (Layer 2) als auch PHY-Funktionen (Layer 1) in einem Chip integriert. Die Linux-Integration erfolgt über **drei Komponenten**:

1. **PHY-Treiber** (`microchip_t1s.c`) - PHY-Funktionen
2. **MAC-PHY Treiber** (`lan865x.c`) - Network Interface + Hardware Control  
3. **OA TC6 Framework** (`oa_tc6.c`) - SPI-Kommunikationsprotokoll

## Komponenten im Detail

### 1. PHY-Treiber: `microchip_t1s.c`

**Pfad:** `drivers/net/phy/microchip_t1s.c`  
**Typ:** `phy_driver`  
**Zweck:** Physical Layer Funktionen

#### Unterstützte Chips:
```c
#define PHY_ID_LAN867X_REVB1 0x0007C162  // Reine T1S PHY Chips (LAN8670/1/2)
#define PHY_ID_LAN865X_REVB0 0x0007C1B3  // MAC-PHY Chips (LAN8650/1)
```

#### Funktionen:
- **PHY-Initialisierung:** Chip-spezifische Register-Konfiguration
- **Link-Status Management:** Link Up/Down Detection
- **PLCA Support:** Physical Layer Collision Avoidance
- **10BASE-T1L Konfiguration:** T1S-spezifische Parameter

#### Kernel-Integration:
```c
module_phy_driver(microchip_t1s_driver);

static struct phy_driver microchip_t1s_driver[] = {
    {
        PHY_ID_MATCH_EXACT(PHY_ID_LAN865X_REVB0),
        .name               = "LAN865X Rev.B0 Internal Phy",
        .features           = PHY_BASIC_T1S_P2MP_FEATURES,
        .config_init        = lan865x_revb0_config_init,
        .read_status        = lan86xx_read_status,
        .read_mmd           = lan865x_phy_read_mmd,      // Spezielle SPI-Zugriffe
        .write_mmd          = lan865x_phy_write_mmd,     // über OA TC6
        .get_plca_cfg       = genphy_c45_plca_get_cfg,
        .set_plca_cfg       = genphy_c45_plca_set_cfg,
        .get_plca_status    = genphy_c45_plca_get_status,
    },
};
```

### 2. MAC-PHY Treiber: `lan865x.c`

**Pfad:** `drivers/net/ethernet/microchip/lan865x/lan865x.c`  
**Typ:** `spi_driver` + Network Interface  
**Zweck:** Vollständiger MAC-PHY Support

#### Funktionen:
- **Network Device Registration:** Erstellt `eth0` Interface
- **Ethernet Frame Handling:** TX/RX von Ethernet-Paketen
- **MAC Address Management:** Hardware MAC-Adresse setzen/lesen
- **Multicast/Broadcast Filtering:** Hash-basiertes Filtering
- **Ethtool Support:** Netzwerk-Statistiken und -Konfiguration
- **SPI Device Binding:** Direkter Hardware-Zugriff

#### Kernel-Integration:
```c
module_spi_driver(lan865x_driver);

static struct spi_driver lan865x_driver = {
    .driver = {
        .name = DRV_NAME,
        .of_match_table = lan865x_dt_ids,
    },
    .probe = lan865x_probe,
    .remove = lan865x_remove,
    .id_table = lan865x_ids,
};

static const struct of_device_id lan865x_dt_ids[] = {
    { .compatible = "microchip,lan8650" },
    { .compatible = "microchip,lan8651" },
    { /* Sentinel */ }
};
```

#### Private Struktur:
```c
struct lan865x_priv {
    struct work_struct multicast_work;
    struct net_device *netdev;
    struct spi_device *spi;
    struct oa_tc6 *tc6;              // OA TC6 Interface
};
```

### 3. OA TC6 Framework: `oa_tc6.c`

**Pfad:** `drivers/net/ethernet/oa_tc6.c`  
**Typ:** Framework-Bibliothek  
**Zweck:** OPEN Alliance 10BASE-T1x MAC-PHY Serial Interface

#### Standard-Konformität:
```c
/*
 * OPEN Alliance 10BASE‑T1x MAC‑PHY Serial Interface framework
 *
 * Link: https://opensig.org/download/document/OPEN_Alliance_10BASET1x_MAC-PHY_Serial_Interface_V1.1.pdf
 */
```

#### Kern-Struktur:
```c
struct oa_tc6 {
    struct device *dev;
    struct net_device *netdev;
    struct phy_device *phydev;
    struct mii_bus *mdiobus;           // MDIO-to-SPI Bridge
    struct spi_device *spi;
    struct mutex spi_ctrl_lock;        // SPI-Zugriffskontrolle
    spinlock_t tx_skb_lock;            // TX Buffer Synchronisation
    void *spi_ctrl_tx_buf;             // Control Command Buffer
    void *spi_ctrl_rx_buf;
    void *spi_data_tx_buf;             // Data Transfer Buffer
    void *spi_data_rx_buf;
    u16 tx_credits;                    // Flow Control
    u8 rx_chunks_available;
    // ...
};
```

#### Exportierte API:
```c
// Initialization
struct oa_tc6 *oa_tc6_init(struct spi_device *spi, struct net_device *netdev);
void oa_tc6_exit(struct oa_tc6 *tc6);

// Register Access (für MAC-PHY Treiber)
int oa_tc6_write_register(struct oa_tc6 *tc6, u32 address, u32 value);
int oa_tc6_read_register(struct oa_tc6 *tc6, u32 address, u32 *value);

// Network Operations
netdev_tx_t oa_tc6_start_xmit(struct oa_tc6 *tc6, struct sk_buff *skb);
```

## Kommunikationsarchitektur

### Physische Verbindung
```
Linux Kernel
     ↓ SPI Bus
LAN8651 Hardware (MAC-PHY Chip)
```

**Einziges Hardware-Interface:** SPI (15 MHz max)

### Software-Architektur

#### Für MAC-Funktionen (Direct Path):
```
MAC-PHY Treiber (lan865x.c)
     ↓ Direct API calls
OA TC6 Framework (oa_tc6.c)  
     ↓ SPI Transactions
LAN8651 Hardware
```

#### Für PHY-Funktionen (MDIO Bridge Path):
```
PHY-Treiber (microchip_t1s.c)
     ↓ Standard MDIO calls
Linux PHY-Subsystem
     ↓ MDIO Bus Operations  
OA TC6 MDIO-Bridge (oa_tc6.c)
     ↓ MDIO-to-SPI Translation
     ↓ SPI Transactions
LAN8651 Hardware
```

### MDIO-to-SPI Bridge

Das OA TC6 Framework stellt einen **virtuellen MDIO Bus** bereit:

```c
// oa_tc6.c - MDIO Bridge Implementation
static int oa_tc6_mdiobus_read(struct mii_bus *bus, int addr, int regnum)
{
    struct oa_tc6 *tc6 = bus->priv;
    u32 regval;
    
    // MDIO Register → SPI Register Address Translation
    ret = oa_tc6_read_register(tc6, OA_TC6_PHY_STD_REG_ADDR_BASE |
                               (regnum & OA_TC6_PHY_STD_REG_ADDR_MASK),
                               &regval);
    return regval;
}

static int oa_tc6_mdiobus_write_c45(struct mii_bus *bus, int addr, int devnum,
                                    int regnum, u16 val)
{
    // MMD/C45 MDIO Zugriffe werden zu SPI-Registern umgeleitet
    // ...
}
```

### Register-Mapping

#### OA TC6 Standard Register:
```c
/* Standard Capabilities Register */
#define OA_TC6_REG_STDCAP                    0x0002
#define STDCAP_DIRECT_PHY_REG_ACCESS         BIT(8)

/* Reset Control and Status Register */
#define OA_TC6_REG_RESET                     0x0003
#define RESET_SWRESET                        BIT(0)

/* Configuration Register #0 */
#define OA_TC6_REG_CONFIG0                   0x0004
#define CONFIG0_SYNC                         BIT(15)
#define CONFIG0_ZARFE_ENABLE                 BIT(12)

/* Buffer Status Register */
#define OA_TC6_REG_BUFFER_STATUS             0x000B
#define BUFFER_STATUS_TX_CREDITS_AVAILABLE   GENMASK(15, 8)
#define BUFFER_STATUS_RX_CHUNKS_AVAILABLE    GENMASK(7, 0)
```

#### LAN865x MAC Register:
```c
/* MAC Network Control Register */
#define LAN865X_REG_MAC_NET_CTL              0x00010000
#define MAC_NET_CTL_TXEN                     BIT(3)
#define MAC_NET_CTL_RXEN                     BIT(2)

/* MAC Network Configuration Reg */
#define LAN865X_REG_MAC_NET_CFG              0x00010001
#define MAC_NET_CFG_PROMISCUOUS_MODE         BIT(4)
#define MAC_NET_CFG_MULTICAST_MODE           BIT(6)

/* MAC Specific Addr 1 Bottom/Top Reg */
#define LAN865X_REG_MAC_L_SADDR1             0x00010022
#define LAN865X_REG_MAC_H_SADDR1             0x00010023
```

## Initialisierung und Binding

### 1. Device Tree Konfiguration
```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target = <&flx2>;
        __overlay__ {
            spi2: spi@400 {
                ethernet@0 {
                    compatible = "microchip,lan8651", "microchip,lan8650";
                    reg = <0>;
                    interrupt-parent = <&gpio>;
                    interrupts = <36 IRQ_TYPE_EDGE_FALLING>;
                    spi-max-frequency = <15000000>;
                };
            };
        };
    };
};
```

### 2. Treiber-Binding Sequence

#### MAC-PHY Treiber Probe:
```c
static int lan865x_probe(struct spi_device *spi)
{
    // 1. Network Device erstellen
    netdev = alloc_etherdev(sizeof(struct lan865x_priv));
    
    // 2. OA TC6 Framework initialisieren
    priv->tc6 = oa_tc6_init(spi, netdev);
    
    // 3. Network Device registrieren  
    ret = register_netdev(netdev);
    
    return 0;
}
```

#### OA TC6 Initialization:
```c
struct oa_tc6 *oa_tc6_init(struct spi_device *spi, struct net_device *netdev)
{
    // 1. OA TC6 Struktur allokieren
    tc6 = devm_kzalloc(&spi->dev, sizeof(*tc6), GFP_KERNEL);
    
    // 2. SPI-Buffer allokieren
    tc6->spi_ctrl_tx_buf = devm_kzalloc(&tc6->spi->dev, OA_TC6_CTRL_SPI_BUF_SIZE, GFP_KERNEL);
    
    // 3. MAC-PHY Software Reset
    ret = oa_tc6_sw_reset_macphy(tc6);
    
    // 4. MDIO Bus registrieren (für PHY-Treiber)
    ret = oa_tc6_mdiobus_register(tc6);
    
    // 5. PHY-Treiber initialisieren
    ret = oa_tc6_phy_init(tc6);
    
    return tc6;
}
```

#### PHY-Treiber Binding:
```c
static int oa_tc6_phy_init(struct oa_tc6 *tc6)
{
    // 1. PHY auf MDIO Bus finden
    tc6->phydev = phy_find_first(tc6->mdiobus);
    
    // 2. PHY mit Network Interface verbinden
    ret = phy_connect_direct(tc6->netdev, tc6->phydev,
                             &oa_tc6_handle_link_change,
                             PHY_INTERFACE_MODE_INTERNAL);
    
    // Linux PHY-Subsystem matched automatisch PHY-ID mit microchip_t1s_driver
    return 0;
}
```

## Datenfluss

### TX (Transmit) Path:
```
Network Stack
     ↓ sk_buff
lan865x_start_xmit() (lan865x.c)
     ↓
oa_tc6_start_xmit() (oa_tc6.c)
     ↓ OA TC6 Protocol + SPI
LAN8651 Hardware
     ↓ Ethernet PHY
10BASE-T1L Network
```

### RX (Receive) Path:
```
10BASE-T1L Network  
     ↓ Ethernet PHY
LAN8651 Hardware
     ↓ SPI Interrupt + OA TC6 Protocol
oa_tc6 RX Handler (oa_tc6.c)
     ↓ sk_buff creation  
Network Stack (netif_rx)
```

### PHY Management Path:
```
User Space (ethtool, ifconfig)
     ↓ ioctl
Linux PHY-Subsystem
     ↓ MDIO Operations
oa_tc6_mdiobus_*() Functions
     ↓ MDIO-to-SPI Translation  
OA TC6 Register Access
     ↓ SPI
LAN8651 PHY Registers
```

## Kernel-Konfiguration

### Erforderliche Config-Optionen:
```bash
# PHY-Support
CONFIG_PHYLIB=y
CONFIG_MICROCHIP_T1S_PHY=y

# Network Device Support  
CONFIG_NETDEVICES=y
CONFIG_ETHERNET=y

# SPI Support
CONFIG_SPI=y
CONFIG_SPI_MASTER=y

# OA TC6 Framework (automatisch durch lan865x aktiviert)
# Kein separates CONFIG - wird als Bibliothek gelinkt
```

### Automatische Aktivierung:
Das BSP-Patch aktiviert automatisch:
```make
# package/lan8651-kernel-config/lan8651-kernel-config.mk
CONFIG_MICROCHIP_T1S_PHY=y
```

## PLCA (Physical Layer Collision Avoidance)

### Device Tree Konfiguration:
```bash
# /etc/network/interfaces (automatisch durch BSP-Patch)
auto eth0
iface eth0 inet static
    address 192.168.0.5
    netmask 255.255.0.0
    post-up ethtool --set-plca-cfg eth0 enable on node-id 0 node-cnt 8
```

### Runtime Kontrolle:
```bash
# PLCA Status prüfen
ethtool --get-plca-cfg eth0

# PLCA Konfiguration setzen  
ethtool --set-plca-cfg eth0 enable on node-id 0 node-cnt 8
```

### PHY-Treiber PLCA Support:
```c
// microchip_t1s.c
.get_plca_cfg    = genphy_c45_plca_get_cfg,
.set_plca_cfg    = genphy_c45_plca_set_cfg, 
.get_plca_status = genphy_c45_plca_get_status,
```

## Debugging und Diagnose

### Kernel Log Messages:
```bash
# Treiber-Loading
dmesg | grep -E "(lan865x|microchip_t1s|oa_tc6)"

# PHY-Status
dmesg | grep -E "(PHY|link)"

# Network Interface
ip addr show eth0
ip link show eth0
```

### Register-Zugriff (Development):
```bash
# PHY Register über MDIO
# (Implementierung device-spezifisch)

# Network Statistics
ethtool -S eth0

# PHY-spezifische Informationen  
ethtool eth0
```

### Erweiterte Diagnose:
```bash
# T1S PHY-Unterstützung prüfen
grep CONFIG_MICROCHIP_T1S_PHY /proc/config.gz || zcat /proc/config.gz | grep T1S

# PLCA Konfiguration
ethtool --get-plca-cfg eth0

# Link Status
cat /sys/class/net/eth0/carrier
```

## Call Tree - Funktionsaufrufe zwischen den Drei Komponenten

### 1. Initialisierung-Call-Chain

#### Kernel Module Loading:
```
module_spi_driver(lan865x_driver)
└── lan865x_probe()                    // lan865x.c
    ├── alloc_etherdev()               // Linux Network Subsystem  
    ├── oa_tc6_init()                  // oa_tc6.c
    │   ├── oa_tc6_sw_reset_macphy()   // oa_tc6.c internal
    │   ├── oa_tc6_mdiobus_register()  // oa_tc6.c internal
    │   └── oa_tc6_phy_init()          // oa_tc6.c internal
    │       └── phy_connect_direct()   // Linux PHY-Subsystem
    │           └── microchip_t1s_driver matched by PHY_ID
    ├── oa_tc6_write_register()        // oa_tc6.c → TSU Timer Config
    ├── oa_tc6_zero_align_receive_frame_enable() // oa_tc6.c
    ├── lan865x_set_hw_macaddr()       // lan865x.c internal
    │   ├── lan865x_set_hw_macaddr_low_bytes() // lan865x.c internal
    │   │   └── oa_tc6_write_register() // oa_tc6.c → MAC Register
    │   └── oa_tc6_write_register()    // oa_tc6.c → MAC Register
    └── register_netdev()              // Linux Network Subsystem
```

### 2. PHY-Treiber Call Tree (microchip_t1s.c)

#### PHY Initialization:
```
Linux PHY-Subsystem
└── .config_init callback
    ├── lan865x_revb0_config_init()           // für LAN8651
    │   ├── phy_write_mmd() [x28]             // Standard MDIO → MDIO Bridge
    │   └── lan865x_setup_cfgparam()
    │       ├── lan865x_generate_cfg_offsets()
    │       │   └── lan865x_revb0_indirect_read()
    │       │       ├── phy_write_mmd()       // → MDIO Bridge
    │       │       └── phy_read_mmd()        // → MDIO Bridge
    │       ├── lan865x_read_cfg_params()
    │       │   └── phy_read_mmd() [x5]       // → MDIO Bridge 
    │       └── lan865x_write_cfg_params()
    │           └── phy_write_mmd() [x5]      // → MDIO Bridge
    └── lan867x_revb1_config_init()           // für LAN8670/1/2
        ├── phy_read_mmd()                    // → MDIO Bridge
        └── phy_modify_mmd() [x12]            // → MDIO Bridge
```

#### PHY Register Access (Spezial für LAN8651):
```
Linux PHY-Subsystem
├── .read_mmd = lan865x_phy_read_mmd()       // microchip_t1s.c
│   └── __mdiobus_c45_read()                 // Linux MDIO → MDIO Bridge
└── .write_mmd = lan865x_phy_write_mmd()     // microchip_t1s.c  
    └── __mdiobus_c45_write()                // Linux MDIO → MDIO Bridge
```

### 3. OA TC6 Framework Call Tree (oa_tc6.c)

#### MDIO-to-SPI Bridge:
```
Linux MDIO Subsystem (von PHY-Treiber)
├── oa_tc6_mdiobus_read()                    // oa_tc6.c
│   └── oa_tc6_read_register()               // oa_tc6.c
├── oa_tc6_mdiobus_write()                   // oa_tc6.c  
│   └── oa_tc6_write_register()              // oa_tc6.c
├── oa_tc6_mdiobus_read_c45()                // oa_tc6.c
│   └── oa_tc6_read_register()               // oa_tc6.c
└── oa_tc6_mdiobus_write_c45()               // oa_tc6.c
    └── oa_tc6_write_register()              // oa_tc6.c
```

#### Register Access Chain:
```
oa_tc6_read_register()                       // oa_tc6.c - Public API
└── oa_tc6_read_registers()                  // oa_tc6.c - Internal
    └── oa_tc6_perform_ctrl_spi_transfer()   // oa_tc6.c - Internal
        └── spi_sync()                       // Linux SPI Subsystem

oa_tc6_write_register()                      // oa_tc6.c - Public API  
└── oa_tc6_write_registers()                 // oa_tc6.c - Internal
    └── oa_tc6_perform_ctrl_spi_transfer()   // oa_tc6.c - Internal
        └── spi_sync()                       // Linux SPI Subsystem  
```

### 4. MAC-PHY Treiber Call Tree (lan865x.c)

#### Network Interface Operations:
```
Linux Network Subsystem
├── .ndo_open = lan865x_net_open()
│   ├── lan865x_hw_enable()                  // lan865x.c
│   │   ├── oa_tc6_read_register()           // oa_tc6.c
│   │   └── oa_tc6_write_register()          // oa_tc6.c
│   ├── phy_start()                          // Linux PHY Subsystem
│   └── netif_start_queue()                  // Linux Network Subsystem
├── .ndo_stop = lan865x_net_close()
│   ├── netif_stop_queue()                   // Linux Network Subsystem
│   ├── phy_stop()                           // Linux PHY Subsystem  
│   └── lan865x_hw_disable()                 // lan865x.c
│       ├── oa_tc6_read_register()           // oa_tc6.c
│       └── oa_tc6_write_register()          // oa_tc6.c
├── .ndo_start_xmit = lan865x_send_packet()
│   └── oa_tc6_start_xmit()                  // oa_tc6.c
└── .ndo_set_rx_mode = lan865x_set_multicast_list()
    └── schedule_work(&priv->multicast_work)
        └── lan865x_multicast_work_handler() // lan865x.c
            ├── lan865x_set_all_multicast_addr()    // lan865x.c
            ├── lan865x_set_specific_multicast_addr() // lan865x.c  
            ├── lan865x_clear_all_multicast_addr()   // lan865x.c
            └── oa_tc6_write_register() [multiple]   // oa_tc6.c
```

#### MAC Address Management:
```
Linux Network Subsystem  
└── .ndo_set_mac_address = lan865x_set_mac_address()  // lan865x.c
    └── lan865x_set_hw_macaddr()                      // lan865x.c
        ├── lan865x_set_hw_macaddr_low_bytes()        // lan865x.c
        │   └── oa_tc6_write_register()               // oa_tc6.c
        └── oa_tc6_write_register()                   // oa_tc6.c
```

### 5. Datenfluss Call Tree

#### TX Path:
```
Network Stack
└── sk_buff → lan865x_send_packet()          // lan865x.c (.ndo_start_xmit)
             └── oa_tc6_start_xmit()          // oa_tc6.c
                 └── oa_tc6_spi_thread()      // oa_tc6.c (Background Thread)
                     └── spi_sync()           // Linux SPI → Hardware
```

#### RX Path:
```
Hardware SPI Interrupt
└── oa_tc6_spi_thread()                      // oa_tc6.c (Background Thread)  
    └── oa_tc6_try_spi_transfer()            // oa_tc6.c
        └── oa_tc6_rx_handler()              // oa_tc6.c  
            └── netif_rx()                   // Linux Network Stack
```

### 6. Kritische API-Schnittstellen

#### Von lan865x.c → oa_tc6.c:
```c
// Hauptschnittstellen
oa_tc6_init()                    // Initialisierung
oa_tc6_exit()                    // Cleanup  
oa_tc6_start_xmit()              // TX-Daten
oa_tc6_read_register()           // Register lesen
oa_tc6_write_register()          // Register schreiben
oa_tc6_zero_align_receive_frame_enable() // Hardware-Konfiguration
```

#### Von oa_tc6.c → Linux Subsystems:
```c  
// SPI-Kommunikation
spi_sync()                       // Synchrone SPI-Transfers

// MDIO Bridge → PHY-Treiber  
mii_bus callbacks:              
.read = oa_tc6_mdiobus_read()    
.write = oa_tc6_mdiobus_write()
.read_c45 = oa_tc6_mdiobus_read_c45()
.write_c45 = oa_tc6_mdiobus_write_c45()
```

#### Von microchip_t1s.c → Linux/OA TC6:
```c
// Standard PHY APIs (über MDIO Bridge)
phy_read_mmd()
phy_write_mmd()  
phy_modify_mmd()

// Speziell für LAN8651 (direkte C45-MDIO)
.read_mmd = lan865x_phy_read_mmd()   // → __mdiobus_c45_read()
.write_mmd = lan865x_phy_write_mmd() // → __mdiobus_c45_write()
```

## Zusammenfassung

Die **LAN8651 T1S MAC-PHY** Integration in Linux nutzt eine **elegante Drei-Schichten-Architektur**:

1. **Standardisierter PHY-Treiber** für PHY-Funktionen (wiederverwendbar)
2. **Spezialisierter MAC-PHY Treiber** für Network Interface und Hardware-Kontrolle  
3. **OA TC6 Framework** als universelle SPI-Kommunikationsschicht

**Vorteile:**
- **Modularer Aufbau** - Komponenten sind wiederverwendbar
- **Standard-Konformität** - Nutzt Linux PHY-Subsystem und OPEN Alliance TC6
- **Transparente Kommunikation** - MDIO-to-SPI Bridge ermöglicht Standard-PHY-API
- **Skalierbarkeit** - Framework unterstützt verschiedene MAC-PHY Chips

**Hardware:** Ein einziges **SPI-Interface** für alle Kommunikation  
**Software:** Abstraktions-Layer ermöglichen Standard-Linux-Networking-API

**Call Tree Erkenntnisse:**
- **Zentrale Rolle des OA TC6 Frameworks** - Alle Hardware-Kommunikation läuft über oa_tc6.c
- **Clevere MDIO-Bridge** - PHY-Treiber nutzen Standard-Linux-APIs, werden transparent zu SPI übersetzt
- **Klare Trennung** - Jede Schicht hat eine definierte Verantwortlichkeit und API
- **Threaded Architecture** - SPI-Kommunikation läuft in separatem Kernel-Thread (oa_tc6_spi_thread)

Diese Architektur zeigt die **Kraft der Linux-Kernel-Abstraktion** - komplexe Hardware wird durch saubere API-Schichten für Anwendungen transparent gemacht.