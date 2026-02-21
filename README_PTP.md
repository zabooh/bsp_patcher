# Hardware PTP-Unterstützung in Linux

Eine ausführliche Anleitung zum Verständnis der Hardware PTP (Precision Time Protocol) Implementierung in Linux, mit praktischen Beispielen anhand des Microchip LAN743x Treibers.

## Inhaltsverzeichnis

1. [Hintergründe und Motivation](#hintergründe-und-motivation)
   - 1.1. [Warum Hardware PTP?](#warum-hardware-ptp)
   - 1.2. [Herausforderungen der Software-Synchronisation](#herausforderungen-der-software-synchronisation)

2. [PTP Grundlagen](#ptp-grundlagen)
   - 2.1. [IEEE 1588v2 Protokoll](#ieee-1588v2-protokoll)
   - 2.2. [Message-Typen](#message-typen)

3. [Linux PTP Framework](#linux-ptp-framework)
   - 3.1. [Kernel-Architektur](#kernel-architektur)
   - 3.2. [Wichtige Datenstrukturen](#wichtige-datenstrukturen)
     - 3.2.1. [ptp_clock_info Struktur](#ptp_clock_info-struktur)
   - 3.3. [Socket-basierte Timestamping](#socket-basierte-timestamping)
     - 3.3.1. [Hardware Timestamping aktivieren](#hardware-timestamping-aktivieren)
     - 3.3.2. [SO_TIMESTAMPING Socket-Option](#so_timestamping-socket-option)

4. [Hardware vs. Software Timestamping](#hardware-vs-software-timestamping)
   - 4.1. [Vergleich der Architekturen](#vergleich-der-architekturen)
     - 4.1.1. [Software Timestamping](#software-timestamping)
     - 4.1.2. [Hardware Timestamping](#hardware-timestamping)
   - 4.2. [Timestamping-Modi](#timestamping-modi)

5. [LAN743x PTP Implementierung](#lan743x-ptp-implementierung)
   - 5.1. [Hardware-Features des LAN743x](#hardware-features-des-lan743x)
   - 5.2. [Warum PCIe für PTP nicht kritisch ist](#warum-pcie-für-ptp-nicht-kritisch-ist)
     - 5.2.1. [Hardware-Timestamping erfolgt am Ethernet-Port](#hardware-timestamping-erfolgt-am-ethernet-port)
     - 5.2.2. [PTP-Clock läuft autonom](#ptp-clock-läuft-autonom)
     - 5.2.3. [Aber: PCIe beeinflusst indirekt die Performance](#aber-pcie-beeinflusst-indirekt-die-performance)
     - 5.2.4. [Vergleich: Embedded vs. PCIe PTP-Controller](#vergleich-embedded-vs-pcie-ptp-controller)
     - 5.2.5. [Zusammenfassung: PCIe vs. PTP Performance](#zusammenfassung-pcie-vs-ptp-performance)
   - 5.3. [Treiber-Struktur Übersicht](#treiber-struktur-übersicht)
   - 5.4. [LAN865x T1S PTP-Implementierung (Zukünftig)](#lan865x-t1s-ptp-implementierung-zukünftig)
     - 5.4.1. [MAC-PHY vs. separater PHY: Wo gehört PTP hin?](#mac-phy-vs-separater-phy-wo-gehört-ptp-hin)
     - 5.4.2. [Aktuelle Hardware-Evidenz im LAN865x Treiber](#aktuelle-hardware-evidenz-im-lan865x-treiber)
     - 5.4.3. [Architektur-Entscheidung: MAC vs. PHY-Treiber](#architektur-entscheidung-mac-vs-phy-treiber)
     - 5.4.4. [Vorgeschlagene LAN865x PTP-Implementierung](#vorgeschlagene-lan865x-ptp-implementierung)
     - 5.4.5. [Herausforderungen bei SPI-basierten PTP](#herausforderungen-bei-spi-basierten-ptp)
     - 5.4.6. [Nächste Schritte für LAN865x PTP](#nächste-schritte-für-lan865x-ptp)

6. [Code-Beispiele und Analyse](#code-beispiele-und-analyse)
   - 6.1. [PTP Clock Registration](#1-ptp-clock-registration)
   - 6.2. [Frequency Adjustment (adjfine)](#2-frequency-adjustment-adjfine)
   - 6.3. [Time Adjustment (adjtime)](#3-time-adjustment-adjtime)
   - 6.4. [TX Timestamp Handling](#4-tx-timestamp-handling)
   - 6.5. [GPIO und Event Channel Management](#5-gpio-und-event-channel-management)
   - 6.6. [Periodic Output (PPS) Generation](#6-periodic-output-pps-generation)

7. [Konfiguration und Verwendung](#konfiguration-und-verwendung)
   - 7.1. [Kernel-Konfiguration](#1-kernel-konfiguration)
   - 7.2. [Hardware-Erkennung](#2-hardware-erkennung)
   - 7.3. [LinuxPTP Konfiguration](#3-linuxptp-konfiguration)
     - 7.3.1. [ptp4l.conf für LAN743x](#ptp4lconf-für-lan743x)
     - 7.3.2. [Starten der PTP Services](#starten-der-ptp-services)
   - 7.4. [GPIO und Event Configuration](#4-gpio-und-event-configuration)
     - 7.4.1. [PPS Output konfigurieren](#pps-output-konfigurieren)
     - 7.4.2. [External Timestamping](#external-timestamping)
   - 7.5. [Real-world Deployment Beispiel](#5-real-world-deployment-beispiel)
     - 7.5.1. [Industrielle Automatisierung Setup](#industrielle-automatisierung-setup)

8. [Debugging und Monitoring](#debugging-und-monitoring)
   - 8.1. [Hardware-Status prüfen](#1-hardware-status-prüfen)
     - 8.1.1. [Register-Dumps (nur Debug-Builds)](#register-dumps-nur-debug-builds)
     - 8.1.2. [PHC Tools](#phc-tools)
   - 8.2. [PTP Message Analysis](#2-ptp-message-analysis)
     - 8.2.1. [tcpdump für PTP Traffic](#tcpdump-für-ptp-traffic)
     - 8.2.2. [Wireshark Analysis](#wireshark-analysis)
   - 8.3. [Performance Monitoring](#3-performance-monitoring)
     - 8.3.1. [PTP Servo Statistics](#ptp-servo-statistics)
     - 8.3.2. [System Performance Impact](#system-performance-impact)
   - 8.4. [Common Issues und Solutions](#4-common-issues-und-solutions)
     - 8.4.1. [Problem: Hoher Jitter](#problem-hoher-jitter)
     - 8.4.2. [Problem: Timestamp Errors](#problem-timestamp-errors)
     - 8.4.3. [Problem: Clock Drift](#problem-clock-drift)
   - 8.5. [Automatisierte Tests](#5-automatisierte-tests)
     - 8.5.1. [PTP Conformance Test](#ptp-conformance-test)

9. [Strategie: PTP-Support im LAN865x Treiber implementieren](#strategie-ptp-support-im-lan865x-treiber-implementieren)
   - 9.1. [Überblick der Implementierungsstrategie](#überblick-der-implementierungsstrategie)
     - 9.1.1. [Phasenplan](#phasenplan)
   - 9.2. [Detaillierte Implementierung](#detaillierte-implementierung)
     - 9.2.1. [Schritt 1: Header-Datei erweitern](#schritt-1-header-datei-erweitern)
     - 9.2.2. [Schritt 2: Basis-Datenstruktur erweitern](#schritt-2-basis-datenstruktur-erweitern)
     - 9.2.3. [Schritt 3: PTP Clock Interface implementieren](#schritt-3-ptp-clock-interface-implementieren)
     - 9.2.4. [Schritt 4: Integration in den Haupttreiber](#schritt-4-integration-in-den-haupttreiber)
     - 9.2.5. [Schritt 5: Timestamping Integration (Zukunft)](#schritt-5-timestamping-integration-zukunft)
   - 9.3. [Build-System Integration](#build-system-integration)
     - 9.3.1. [Makefile erweitern](#makefile-erweitern)
     - 9.3.2. [Kconfig erweitern](#kconfig-erweitern)
   - 9.4. [Testing-Strategie](#testing-strategie)
     - 9.4.1. [Unit Tests](#unit-tests)
     - 9.4.2. [Integration Tests](#integration-tests)
   - 9.5. [Troubleshooting-Checkliste](#troubleshooting-checkliste)
     - 9.5.1. [Häufige Probleme](#häufige-probleme)
     - 9.5.2. [Debug-Kommandos](#debug-kommandos)
   - 9.6. [Performance-Erwartungen](#performance-erwartungen)
     - 9.6.1. [Realistische Ziele](#realistische-ziele)
     - 9.6.2. [Optimierungen](#optimierungen)

10. [Anhang](#anhang)
    - 10.1. [Register-Referenz LAN743x](#a-register-referenz-lan743x)
      - 10.1.1. [PTP Clock Registers](#ptp-clock-registers)
    - 10.2. [Nützliche Links](#b-nützliche-links)
    - 10.3. [Glossar](#c-glossar)

---

## Hintergründe und Motivation

### Warum Hardware PTP?

In modernen vernetzten Systemen ist präzise Zeitsynchronisation kritisch für:

- **Industrielle Automatisierung**: Koordination von Produktionsanlagen
- **Telekommunikation**: 5G-Netzwerke benötigen Nanosekunden-Genauigkeit
- **Finanzhandel**: Timestamping für Transaktionen
- **Verteilte Messsysteme**: Sensornetzwerke mit zeitkritischen Daten
- **Audio/Video-Streaming**: Synchrone Wiedergabe über Netzwerk

### Herausforderungen der Software-Synchronisation

**Software PTP Limitierungen:**
```
Application Layer    │ Variable Delay: 10-100µs
Transport Layer      │ Stack Processing: 1-10µs  
Network Layer        │ Kernel Context Switch: 1-5µs
Data Link Layer      │ Driver Processing: 0.1-1µs
Physical Layer       │ Hardware: <100ns
```

**Hardware PTP Vorteile:**
- **Deterministisch**: Konstante Hardware-Latenz
- **Präzise**: Nanosekunden-Genauigkeit statt Mikrosekunden
- **CPU-effizient**: Offload von zeitkritischen Operationen

---

## PTP Grundlagen

### IEEE 1588v2 Protokoll

PTP verwendet einen Master-Slave-Mechanismus zur Zeitsynchronisation:

```
Master                           Slave
  │                               │
  │─── Sync Message ────────────→ │ (t1)
  │                               │
  │←── Delay_Req ──────────────── │ (t2)
  │                               │
  │─── Delay_Resp ─────────────→ │ (t3,t4)
  │                               │
```

**Zeitberechnung:**
```
Offset = ((t2 - t1) - (t4 - t3)) / 2
Delay  = ((t2 - t1) + (t4 - t3)) / 2
```

### Message-Typen

| Type | Beschreibung | Hardware-Relevanz |
|------|--------------|-------------------|
| Sync | Zeitreferenz vom Master | TX-Timestamping |
| Follow_Up | Präziser Sync-Zeitstempel | - |
| Delay_Req | Delay-Messung vom Slave | TX-Timestamping |
| Delay_Resp | Delay-Antwort vom Master | RX-Timestamping |

---

## Linux PTP Framework

### Kernel-Architektur

```
Userspace Applications (ptp4l, phc2sys)
        │
   ┌────▼────┐
   │ PTP API │ (/dev/ptpX, Socket Options)
   └────┬────┘
        │
   ┌────▼────────────────┐
   │ PTP Clock Framework │ (ptp_clock_kernel.h)
   │ - ptp_clock_info    │
   │ - ptp_clock_ops     │
   └────┬────────────────┘
        │
   ┌────▼────┐    ┌──────────────┐
   │ Network │    │ PTP Hardware │
   │ Driver  │◄──►│ (PHY/MAC)    │
   └─────────┘    └──────────────┘
```

### Wichtige Datenstrukturen

#### ptp_clock_info Struktur
```c
struct ptp_clock_info {
    struct module *owner;
    char name[16];
    s32 max_adj;                    /* max frequency adjustment (ppb) */
    int n_alarm;                    /* number of alarms */
    int n_ext_ts;                   /* number of external timestamps */
    int n_per_out;                  /* number of periodic outputs */
    int n_pins;                     /* number of input/output pins */
    int pps;                        /* indicates whether the clock supports PPS */
    
    /* Function pointers for clock operations */
    int (*adjfine)(struct ptp_clock_info *ptp, long scaled_ppm);
    int (*adjtime)(struct ptp_clock_info *ptp, s64 delta);
    int (*gettime64)(struct ptp_clock_info *ptp, struct timespec64 *ts);
    int (*settime64)(struct ptp_clock_info *ptp, const struct timespec64 *ts);
    int (*enable)(struct ptp_clock_info *ptp, struct ptp_clock_request *req, int on);
    int (*verify)(struct ptp_clock_info *ptp, unsigned int pin, enum ptp_pin_function func, unsigned int chan);
};
```

### Socket-basierte Timestamping

#### Hardware Timestamping aktivieren
```c
#include <linux/net_tstamp.h>
#include <linux/sockios.h>

struct hwtstamp_config hwts_config;
struct ifreq ifr;

hwts_config.flags = 0;
hwts_config.tx_type = HWTSTAMP_TX_ON;
hwts_config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;

ifr.ifr_data = (void*)&hwts_config;
strcpy(ifr.ifr_name, "eth0");

if (ioctl(sockfd, SIOCSHWTSTAMP, &ifr) < 0) {
    perror("SIOCSHWTSTAMP failed");
}
```

#### SO_TIMESTAMPING Socket-Option
```c
int timestamping_flags = SOF_TIMESTAMPING_TX_HARDWARE |
                        SOF_TIMESTAMPING_RX_HARDWARE |
                        SOF_TIMESTAMPING_RAW_HARDWARE;

if (setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING,
               &timestamping_flags, sizeof(timestamping_flags)) < 0) {
    perror("setsockopt SO_TIMESTAMPING failed");
}
```

---

## Hardware vs. Software Timestamping

### Vergleich der Architekturen

#### Software Timestamping
```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│ Application │◄──►│ Linux Kernel │◄──►│ Network HW  │
│             │    │ (Interrupt   │    │ (Standard   │
│ ptp4l       │    │  Handler)    │    │  Ethernet)  │
└─────────────┘    └──────────────┘    └─────────────┘
     ↑                      ↑                   ↑
     └─ Timestamp ──────────┘                   │
                                                │
                                          Packets arrive
                                          without HW timestamps
```
**Genauigkeit**: ±1-10 µs | **Jitter**: Hoch | **CPU-Last**: Hoch

#### Hardware Timestamping
```
┌─────────────┐    ┌──────────────┐    ┌─────────────────┐
│ Application │◄──►│ Linux Kernel │◄──►│ PTP-capable HW  │
│             │    │ (PTP Driver) │    │ - Hardware Clock│
│ ptp4l       │    │              │    │ - TX/RX Stamps  │
│             │    │              │    │ - Event Channels│
└─────────────┘    └──────────────┘    └─────────────────┘
     ↑                      ↑                   ↑
     └─ Precise ────────────┴──── HW ──────────┘
        Timestamp                 Timestamp
```
**Genauigkeit**: ±10-100 ns | **Jitter**: Niedrig | **CPU-Last**: Niedrig

### Timestamping-Modi

| Modus | Beschreibung | Use Case |
|-------|--------------|----------|
| **One-Step** | Timestamp direkt ins Paket | Geringste Latenz |
| **Two-Step** | Timestamp in Follow-Up | Flexibilität |
| **P2P** | Peer-to-Peer Delay | Switched Networks |
| **E2E** | End-to-End Delay | Routed Networks |

---

## LAN743x PTP Implementierung 

### Hardware-Features des LAN743x

Der Microchip LAN743x (LAN7430/7431) ist ein **PCIe-zu-Gigabit-Ethernet-Controller** mit integrierter Hardware PTP-Unterstützung. Die PTP-Funktionalität ist primär im **Ethernet-Controller** implementiert, nicht im PCIe-Interface:

```
┌─────────────────────────────────────────────────────────────────┐
│                         LAN743x Controller                      │
│  ┌─────────────┐    ┌──────────────────┐    ┌─────────────────┐ │
│  │ PCIe        │    │ PTP Engine       │    │ Ethernet        │ │
│  │ Interface   │    │ - 125MHz PTP Clk │◄──►│ MAC/PHY         │ │
│  │ (Gen2 x1)   │◄──►│ - HW Timestamps  │    │ - TX/RX Stamps  │ │
│  │ - Registers │    │ - Event Channels │    │ - Link Speed    │ │
│  │ - Interrupts│    │ - GPIO Control   │    │ - Frame Filter  │ │
│  └─────────────┘    └──────────────────┘    └─────────────────┘ │
│        ▲                       ▲                        ▲        │
│        │                       │                        │        │
│   CPU Access              Independent              Wire Speed     │
│   (~100ns)                PTP Clock               Timestamping    │
└─────────────────────────────────────────────────────────────────┘
```

**Wichtige Architektur-Aspekte:**
- **PTP-Clock läuft unabhängig**: 125 MHz Crystal, nicht von PCIe-Clock abgeleitet
- **Hardware-Timestamping**: Erfolgt direkt am Ethernet PHY/MAC (Wire-Speed)
- **PCIe-Relevanz**: Nur für Register-Zugriff und Interrupt-Handling

**Spezifikationen:**
- **Clock-Frequenz**: 125 MHz (8ns Auflösung)
- **Frequenz-Adjustment**: ±31.25 ppm
- **Event-Kanäle**: 2 (Timer A/B)
- **GPIO-Pins**: 4 (LAN7430) / 12 (LAN7431)
- **Timestamp-FIFO**: Hardware-gepuffert
- **PCIe-Interface**: Gen2 x1 (für Register-Zugriff)

### Warum PCIe für PTP nicht kritisch ist

Die **PTP-Core-Funktionalität** ist unabhängig vom PCIe-Interface implementiert:

#### **Hardware-Timestamping erfolgt am Ethernet-Port:**
```c
// Timestamp wird direkt beim Frame-Ein/Ausgang erstellt
// NICHT beim PCIe-Transfer zur CPU
Frame RX: Ethernet PHY ──[HW-Timestamp]──► FIFO ──PCIe──► CPU
Frame TX: CPU ──PCIe──► FIFO ──[HW-Timestamp]──► Ethernet PHY
```

#### **PTP-Clock läuft autonom:**
- **125 MHz Crystal**: Unabhängige Referenz (nicht PCIe-abgeleitet)
- **Hardware-Servo**: Frequenz-Adjustments ohne CPU-Intervention
- **Event-Generation**: GPIO-Pulse direkt von PTP-Clock gesteuert

#### **Aber: PCIe beeinflusst indirekt die Performance:**

| Aspekt | PCIe-Einfluss | Typischer Impact |
|--------|---------------|------------------|
| **Register-Zugriff** | PCIe-Latency (~100-200ns) | Vernachlässigbar für PTP |
| **Interrupt-Latency** | PCIe + OS Overhead (~1-10µs) | Kann TX-Timestamp-Abruf verzögern |
| **Timestamp-FIFO** | PCIe-Bandwidth | Bei hoher Last relevant |
| **Clock-Adjustments** | CPU → Register via PCIe | ~100ns für `adjfine()` |

#### **Vergleich: Embedded vs. PCIe PTP-Controller:**

```
Embedded Controller (z.B. ARM + PHY):
├── CPU ──[Register Bus]──► PTP Engine ──► Ethernet  (10-50ns Register-Zugriff)

PCIe Controller (z.B. LAN743x):  
├── CPU ──[PCIe TLP]──► PTP Engine ──► Ethernet      (100-200ns Register-Zugriff)
```

**Fazit**: PCIe ist **nicht** der limitierende Faktor für PTP-Genauigkeit, da Timestamping hardwarenah erfolgt.

**Zusammenfassung: PCIe vs. PTP Performance**

- ✅ **PTP-Core arbeitet autonom**: Hardware-Clock, Timestamping, Event-Generation
- ✅ **Nanosekunden-Genauigkeit**: Unabhängig von PCIe-Latency (100-200ns)  
- ⚠️ **PCIe-Einfluss begrenzt**: Nur Register-Zugriffe und Interrupt-Handling
- ⚠️ **High-Load-Szenarien**: PCIe-Bandwidth kann Timestamp-Abruf verzögern
- ✅ **Praktisch**: LAN743x erreicht <100ns PTP-Genauigkeit trotz PCIe-Interface

**Beispiel-Messung** (Typische Werte):
```
PTP Sync Message: 
├── Hardware RX-Timestamp:     1645000000.123456789 (Nanosekunden-Genau)
├── PCIe Interrupt Delivery:   ~2-5µs später
├── CPU Register-Read:         ~100-200ns
└── ptp4l Processing:          ~10-50µs

→ Hardware-Timestamp bleibt präzise, nur Software-Processing verzögert
```

---

## LAN865x T1S PTP-Implementierung (Zukünftig)

### MAC-PHY vs. separater PHY: Wo gehört PTP hin?

Der **LAN865x (LAN8650/8651)** ist ein **10BASE-T1S MAC-PHY-Controller** mit **integrierter PTP-Hardware**, aber noch **ohne Treiber-Unterstützung**. Die Architektur unterscheidet sich bedeutend vom LAN743x:

```
LAN743x (PCIe-Ethernet-Controller):
├── CPU ──[PCIe]──► LAN743x ──► Ethernet PHY (extern)
│                      │
│                      └────► PTP Engine (im Controller)

LAN865x (T1S MAC-PHY):  
├── CPU ──[SPI]──► LAN865x ──► T1S Line (integriert)
│                     │
│                     ├────► MAC Layer
│                     ├────► PHY Layer  
│                     └────► PTP Engine (wo genau?)
```

### Aktuelle Hardware-Evidenz im LAN865x Treiber

Bereits im bestehenden Code finden sich **PTP-Hardware-Hinweise**:

```c
/* MAC TSU Timer Increment Register */
#define LAN865X_REG_MAC_TSU_TIMER_INCR     0x00010077
#define MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS 0x0028  // 40ns = 25MHz

/* Aus lan865x_probe(): */
/* LAN865x Rev.B0/B1 configuration parameters from AN1760
 * configure the MAC to set time stamping at the end of the 
 * Start of Frame Delimiter (SFD) and set the Timer Increment 
 * reg to 40 ns to be used as a 25 MHz internal clock.
 */
ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
                            MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
```

**Interpretation:**
- **TSU** = Time Sync Unit (PTP-Hardware vorhanden!)
- **SFD Timestamping** = Hardware-Timestamping am Wire
- **25 MHz Clock** = PTP-Referenz-Takt (40ns Auflösung)

### Architektur-Entscheidung: MAC vs. PHY-Treiber

Für den **LAN865x** sollte PTP im **MAC-Treiber** implementiert werden:

#### ✅ **Argumente für MAC-Treiber (`lan865x.c`):**

1. **Integrierter MAC-PHY**: Keine Trennung von MAC/PHY-Treibern
2. **Register-Zugriff**: PTP-Register sind über SPI/OA-TC6 erreichbar (MAC-Domain)  
3. **Network-Device**: `struct net_device` existiert bereits im MAC-Treiber
4. **Timestamping**: Hardware-Timestamping erfolgt im MAC-Layer (SFD-Ende)
5. **Architektur-Konsistenz**: Analog zu LAN743x (alle PTP-Features im MAC-Treiber)

#### ❌ **Argumente gegen PHY-Treiber:**

1. **Doppelte Implementierung**: MAC-Treiber müsste trotzdem PTP-Clock registrieren
2. **Komplexere Koordination**: MAC ⟷ PHY Kommunikation für Timestamping
3. **Register-Zugriff**: PTP-Register sind nicht über MDIO erreichbar
4. **10BASE-T1S Spezialfall**: Integrierter Controller, nicht separater PHY

### Vorgeschlagene LAN865x PTP-Implementierung

#### **Struktur-Erweiterung:**
```c
struct lan865x_priv {
    struct work_struct multicast_work;
    struct net_device *netdev;
    struct spi_device *spi;
    struct oa_tc6 *tc6;
    
    /* PTP-Erweiterungen */
    struct ptp_clock_info ptp_clock_info;
    struct ptp_clock *ptp_clock;
    struct mutex ptp_lock;
    
    /* Timestamps */
    spinlock_t tx_ts_lock;
    struct sk_buff *tx_ts_skb_queue[LAN865X_PTP_MAX_TX_TS];
    u32 tx_ts_queue_size;
    
    u32 ptp_flags;
};
```

#### **Fehlende Register definieren** (Basis auf TSU-Evidenz):
```c
/* PTP/TSU Registers (abgeleitet vom vorhandenen TSU_TIMER_INCR) */
#define LAN865X_REG_PTP_CLOCK_SEC          0x00010070  // PTP Seconds
#define LAN865X_REG_PTP_CLOCK_NS           0x00010071  // PTP Nanoseconds  
#define LAN865X_REG_PTP_CLOCK_SUBNS        0x00010072  // PTP Sub-Nanoseconds
#define LAN865X_REG_PTP_RATE_ADJ           0x00010073  // Frequency Adjustment
#define LAN865X_REG_PTP_CMD_CTL            0x00010074  // PTP Commands
#define LAN865X_REG_PTP_TX_TS_SEC          0x00010075  // TX Timestamp Sec
#define LAN865X_REG_PTP_TX_TS_NS           0x00010076  // TX Timestamp NS
#define LAN865X_REG_MAC_TSU_TIMER_INCR     0x00010077  // Timer Increment (bereits definiert)

/* PTP Control Flags */
#define LAN865X_PTP_ENABLE                 BIT(0)
#define LAN865X_PTP_TX_TSTAMP_EN           BIT(1)
#define LAN865X_PTP_RX_TSTAMP_EN           BIT(2)
```

#### **Integration in bestehenden Workflow:**
```c
static int lan865x_probe(struct spi_device *spi)
{
    /* ... bestehender Code ... */
    
    /* PTP initialisieren */
    ret = lan865x_ptp_init(priv);
    if (ret) {
        dev_err(&spi->dev, "Failed to init PTP: %d\n", ret);
        goto oa_tc6_exit;
    }
    
    /* TSU schon konfiguriert */
    ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
                               MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
    
    /* PTP aktivieren */  
    ret = lan865x_ptp_open(priv);
    if (ret) {
        dev_warn(&spi->dev, "PTP initialization failed: %d\n", ret);
        /* Nicht-fatal, Netzwerk funktioniert ohne PTP */
    }
    
    /* ... restlicher Code ... */
}
```

### Herausforderungen bei SPI-basierten PTP

Im Gegensatz zum **PCIe LAN743x** hat der **SPI LAN865x** zusätzliche Latency-Quellen:

| Aspekt | LAN743x (PCIe) | LAN865x (SPI) |
|--------|----------------|---------------|
| **Register-Zugriff** | 100-200ns | 1-10µs (SPI-Clock abhängig) |
| **Interrupt-Latency** | ~1µs | ~5-20µs (SPI + GPIO) |
| **Timestamp-Abruf** | Hardware-FIFO | SPI-Read-Sequenz |
| **Clock-Adjustment** | Sofort | Via SPI-Transaktion |

**Aber**: Die **PTP-Genauigkeit bleibt unberührt**, da Hardware-Timestamping am SFD erfolgt!

### Nächste Schritte für LAN865x PTP

1. **Hardware-Dokumentation**: Microchip LAN865x Register-Map für PTP-Register
2. **Treiber-Erweiterung**: PTP-Framework-Integration in `lan865x.c`
3. **SPI-Optimierung**: Effiziente Register-Zugriffe für PTP-Operations
4. **Timestamping**: TX/RX Timestamp-Handling über OA-TC6
5. **Testing**: Verification gegen Hardware mit bekanntem PTP-Master

### Treiber-Struktur Übersicht

```c
struct lan743x_ptp {
    /* PTP Clock Framework Integration */
    struct ptp_clock_info ptp_clock_info;
    struct ptp_clock *ptp_clock;
    
    /* Hardware timestamp queues */
    struct sk_buff *tx_ts_skb_queue[LAN743X_PTP_NUMBER_OF_TX_TIMESTAMPS];
    u32 tx_ts_seconds_queue[LAN743X_PTP_NUMBER_OF_TX_TIMESTAMPS];
    u32 tx_ts_nseconds_queue[LAN743X_PTP_NUMBER_OF_TX_TIMESTAMPS];
    u32 tx_ts_header_queue[LAN743X_PTP_NUMBER_OF_TX_TIMESTAMPS];
    
    /* Synchronization */
    spinlock_t tx_ts_lock;
    struct mutex command_lock;
    
    /* Event channels and GPIO */
    struct lan743x_perout perout[LAN743X_PTP_N_EVENT_CHAN];
    struct lan743x_extts extts[LAN743X_PTP_N_EXTTS];
    struct ptp_pin_desc pin_config[LAN743X_PTP_N_GPIO];
    
    /* Status flags */
    u32 flags;
    u32 used_event_ch;
};
```

---

## Code-Beispiele und Analyse

### 1. PTP Clock Registration

Der LAN743x-Treiber registriert sich beim Linux PTP Framework:

```c
int lan743x_ptp_open(struct lan743x_adapter *adapter)
{
    struct lan743x_ptp *ptp = &adapter->ptp;
    
    /* Hardware initialisieren */
    lan743x_ptp_reset(adapter);
    lan743x_ptp_sync_to_system_clock(adapter);
    
    /* PTP Clock Info konfigurieren */
    ptp->ptp_clock_info.owner = THIS_MODULE;
    snprintf(ptp->ptp_clock_info.name, 16, "%pm", adapter->netdev->dev_addr);
    ptp->ptp_clock_info.max_adj = LAN743X_PTP_MAX_FREQ_ADJ_IN_PPB;  // 31249999
    ptp->ptp_clock_info.n_ext_ts = LAN743X_PTP_N_EXTTS;             // 8
    ptp->ptp_clock_info.n_per_out = LAN743X_PTP_N_EVENT_CHAN;       // 2
    ptp->ptp_clock_info.n_pins = n_pins;                            // 4 oder 12
    
    /* Function Pointers setzen */
    ptp->ptp_clock_info.adjfine = lan743x_ptpci_adjfine;
    ptp->ptp_clock_info.adjtime = lan743x_ptpci_adjtime;
    ptp->ptp_clock_info.gettime64 = lan743x_ptpci_gettime64;
    ptp->ptp_clock_info.settime64 = lan743x_ptpci_settime64;
    ptp->ptp_clock_info.enable = lan743x_ptpci_enable;
    
    /* PTP Clock registrieren */
    ptp->ptp_clock = ptp_clock_register(&ptp->ptp_clock_info,
                                        &adapter->pdev->dev);
    if (IS_ERR(ptp->ptp_clock)) {
        netif_err(adapter, ifup, adapter->netdev,
                  "ptp_clock_register failed\n");
        return PTR_ERR(ptp->ptp_clock);
    }
    
    /* Interrupts aktivieren */
    lan743x_csr_write(adapter, INT_EN_SET, INT_BIT_1588_);
    lan743x_csr_write(adapter, PTP_INT_EN_SET,
                      PTP_INT_BIT_TX_SWTS_ERR_ | PTP_INT_BIT_TX_TS_);
    
    return 0;
}
```

### 2. Frequency Adjustment (adjfine)

Präzise Frequenzanpassung für Clock-Synchronisation:

```c
static int lan743x_ptpci_adjfine(struct ptp_clock_info *ptpci, long scaled_ppm)
{
    struct lan743x_ptp *ptp = container_of(ptpci, struct lan743x_ptp, ptp_clock_info);
    struct lan743x_adapter *adapter = container_of(ptp, struct lan743x_adapter, ptp);
    
    u32 lan743x_rate_adj = 0;
    u64 u64_delta;
    
    /* Eingabe-Validation: ±31.25 ppm Max */
    if ((scaled_ppm < (-LAN743X_PTP_MAX_FINE_ADJ_IN_SCALED_PPM)) ||
        scaled_ppm > LAN743X_PTP_MAX_FINE_ADJ_IN_SCALED_PPM) {
        return -EINVAL;
    }
    
    /* Berechnung des Hardware-Adjustment-Werts */
    if (diff_by_scaled_ppm(1ULL << 35, scaled_ppm, &u64_delta))
        lan743x_rate_adj = (u32)u64_delta;
    else
        lan743x_rate_adj = (u32)u64_delta | PTP_CLOCK_RATE_ADJ_DIR_;
    
    /* Hardware-Register schreiben (via PCIe, aber Effekt ist sofort in HW) */
    lan743x_csr_write(adapter, PTP_CLOCK_RATE_ADJ, lan743x_rate_adj);
    
    return 0;
}
```

**Wichtig**: Obwohl der Register-Zugriff ~100-200ns via PCIe dauert, erfolgt die **tatsächliche Clock-Adjustierung sofort in Hardware**. Die PTP-Engine wendet das Adjustment kontinuierlich auf den 125MHz-Takt an, unabhängig von weiteren CPU/PCIe-Operationen.

**Mathematische Herleitung:**
```
Scaled PPM = (Frequency_Offset / Base_Frequency) × 2^16 × 10^6
Hardware_Value = (2^35 × Scaled_PPM) / (2^16 × 10^6)
               = (2^35 × Scaled_PPM) / 65536000000
```

### 3. Time Adjustment (adjtime)

Schrittweise Zeitanpassung ohne Clock-Sprünge:

```c
static void lan743x_ptp_clock_step(struct lan743x_adapter *adapter, s64 time_step_ns)
{
    struct lan743x_ptp *ptp = &adapter->ptp;
    u32 nano_seconds = 0, unsigned_seconds = 0;
    s32 seconds = 0;
    u32 remainder = 0;
    
    /* Große Zeitsprünge als Clock-Set behandeln */
    if (time_step_ns > 15000000000LL) {  // > 15 Sekunden
        lan743x_ptp_clock_get(adapter, &unsigned_seconds, &nano_seconds, NULL);
        unsigned_seconds += div_u64_rem(time_step_ns, 1000000000LL, &remainder);
        nano_seconds += remainder;
        
        if (nano_seconds >= 1000000000) {
            unsigned_seconds++;
            nano_seconds -= 1000000000;
        }
        lan743x_ptp_clock_set(adapter, unsigned_seconds, nano_seconds, 0);
    }
    /* Kleine Anpassungen als Step */
    else {
        if (time_step_ns >= 0) {
            seconds = (s32)div_u64_rem(time_step_ns, 1000000000, &remainder);
            nano_seconds = (u32)remainder;
        } else {
            seconds = -((s32)div_u64_rem(-time_step_ns, 1000000000, &remainder));
            nano_seconds = (u32)remainder;
            if (nano_seconds > 0) {
                seconds--;
                nano_seconds = 1000000000 - nano_seconds;
            }
        }
        
        /* Hardware Step-Befehl */
        mutex_lock(&ptp->command_lock);
        lan743x_csr_write(adapter, PTP_CLOCK_STEP_ADJ,
                          PTP_CLOCK_STEP_ADJ_DIR_ |
                          ((u32)seconds));
        lan743x_csr_write(adapter, PTP_CLOCK_STEP_ADJ,
                          PTP_CLOCK_STEP_ADJ_DIR_ |
                          nano_seconds);
        mutex_unlock(&ptp->command_lock);
    }
}
```

### 4. TX Timestamp Handling

Hardware-Zeitstempel für ausgehende PTP-Pakete:

```c
static void lan743x_ptp_tx_ts_complete(struct lan743x_adapter *adapter)
{
    struct lan743x_ptp *ptp = &adapter->ptp;
    struct skb_shared_hwtstamps tstamps;
    u32 header, nseconds, seconds;
    bool ignore_sync = false;
    struct sk_buff *skb;
    int c, i;
    
    spin_lock_bh(&ptp->tx_ts_lock);
    
    /* Queue-Größe bestimmen */
    c = ptp->tx_ts_skb_queue_size;
    if (c > ptp->tx_ts_queue_size)
        c = ptp->tx_ts_queue_size;
    
    /* Timestamps zu SKBs zuordnen */
    for (i = 0; i < c; i++) {
        skb = ptp->tx_ts_skb_queue[i];
        seconds = ptp->tx_ts_seconds_queue[i];
        nseconds = ptp->tx_ts_nseconds_queue[i];
        header = ptp->tx_ts_header_queue[i];
        
        /* Hardware-Timestamp in SKB einsetzen */
        memset(&tstamps, 0, sizeof(tstamps));
        tstamps.hwtstamp = ktime_set(seconds, nseconds);
        
        /* Timestamp an Upper Layer weitergeben */
        skb_tstamp_tx(skb, &tstamps);
        dev_kfree_skb(skb);
        
        /* Queue-Einträge löschen */
        ptp->tx_ts_skb_queue[i] = NULL;
        ptp->tx_ts_seconds_queue[i] = 0;
        ptp->tx_ts_nseconds_queue[i] = 0;
        ptp->tx_ts_header_queue[i] = 0;
    }
    
    /* Queue verschieben */
    ptp->tx_ts_skb_queue_size -= c;
    ptp->tx_ts_queue_size -= c;
    ptp->pending_tx_timestamps -= c;
    
    spin_unlock_bh(&ptp->tx_ts_lock);
}
```

### 5. GPIO und Event Channel Management

Konfiguration von GPIO-Pins für PTP-Events:

```c
static int lan743x_gpio_rsrv_ptp_out(struct lan743x_adapter *adapter,
                                     int pin, int event_channel)
{
    struct lan743x_gpio *gpio = &adapter->gpio;
    unsigned long irq_flags = 0;
    int bit_mask = BIT(pin);
    int ret = -EBUSY;
    
    spin_lock_irqsave(&gpio->gpio_lock, irq_flags);
    
    if (!(gpio->used_bits & bit_mask)) {
        /* Pin reservieren */
        gpio->used_bits |= bit_mask;
        gpio->output_bits |= bit_mask;
        gpio->ptp_bits |= bit_mask;
        
        /* LED-Multiplexing deaktivieren */
        lan743x_led_mux_enable(adapter, pin, false);
        
        /* GPIO als Output konfigurieren */
        gpio->gpio_cfg0 |= GPIO_CFG0_GPIO_DIR_BIT_(pin);
        gpio->gpio_cfg0 &= ~GPIO_CFG0_GPIO_DATA_BIT_(pin);
        lan743x_csr_write(adapter, GPIO_CFG0, gpio->gpio_cfg0);
        
        /* Push-Pull-Modus aktivieren */
        gpio->gpio_cfg1 &= ~GPIO_CFG1_GPIOEN_BIT_(pin);
        gpio->gpio_cfg1 |= GPIO_CFG1_GPIOBUF_BIT_(pin);
        lan743x_csr_write(adapter, GPIO_CFG1, gpio->gpio_cfg1);
        
        /* PTP-Polarität setzen */
        gpio->gpio_cfg2 |= GPIO_CFG2_1588_POL_BIT_(pin);
        lan743x_csr_write(adapter, GPIO_CFG2, gpio->gpio_cfg2);
        
        /* Event Channel zuweisen */
        if (event_channel == 0) {
            gpio->gpio_cfg3 &= ~GPIO_CFG3_1588_CH_SEL_BIT_(pin); // Channel A
        } else {
            gpio->gpio_cfg3 |= GPIO_CFG3_1588_CH_SEL_BIT_(pin);  // Channel B
        }
        gpio->gpio_cfg3 |= GPIO_CFG3_1588_OE_BIT_(pin);         // Output Enable
        lan743x_csr_write(adapter, GPIO_CFG3, gpio->gpio_cfg3);
        
        ret = pin;
    }
    
    spin_unlock_irqrestore(&gpio->gpio_lock, irq_flags);
    return ret;
}
```

### 6. Periodic Output (PPS) Generation

Hardware-generierte periodische Pulse:

```c
static int lan743x_ptp_perout(struct lan743x_adapter *adapter, int on,
                              struct ptp_perout_request *perout_request)
{
    struct lan743x_ptp *ptp = &adapter->ptp;
    u32 period_sec, period_nsec;
    u32 start_sec, start_nsec;
    int event_ch, gpio_pin;
    u32 general_config;
    int pulse_width = 0;
    
    if (!on) {
        lan743x_ptp_perout_off(adapter, perout_request->index);
        return 0;
    }
    
    /* Event Channel reservieren */
    event_ch = lan743x_ptp_reserve_event_ch(adapter, index + 1);
    if (event_ch < 0) {
        netif_warn(adapter, drv, adapter->netdev,
                   "Failed to reserve event channel %d for PEROUT\n", event_ch);
        return -EBUSY;
    }
    
    /* GPIO Pin reservieren */
    gpio_pin = lan743x_gpio_rsrv_ptp_out(adapter, index, event_ch);
    if (gpio_pin < 0) {
        lan743x_ptp_release_event_ch(adapter, event_ch);
        return -EBUSY;
    }
    
    /* Periode berechnen */
    period_sec = perout_request->period.sec;
    period_nsec = perout_request->period.nsec;
    
    /* Start-Zeit setzen */
    start_sec = perout_request->start.sec;
    start_nsec = perout_request->start.nsec;
    
    /* Pulse Width bestimmen (10% der Periode) */
    if (period_sec == 0) {
        if (period_nsec >= 400000000)
            pulse_width = PTP_GENERAL_CONFIG_CLOCK_EVENT_200MS_;
        else if (period_nsec >= 20000000)
            pulse_width = PTP_GENERAL_CONFIG_CLOCK_EVENT_10MS_;
        else if (period_nsec >= 2000000)
            pulse_width = PTP_GENERAL_CONFIG_CLOCK_EVENT_1MS_;
        else
            pulse_width = PTP_GENERAL_CONFIG_CLOCK_EVENT_100US_;
    } else {
        pulse_width = PTP_GENERAL_CONFIG_CLOCK_EVENT_200MS_;
    }
    
    /* Hardware konfigurieren */
    general_config = lan743x_csr_read(adapter, PTP_GENERAL_CONFIG);
    general_config &= ~PTP_GENERAL_CONFIG_CLOCK_EVENT_X_MASK_(event_ch);
    general_config |= PTP_GENERAL_CONFIG_CLOCK_EVENT_X_SET_(event_ch, pulse_width);
    general_config &= ~PTP_GENERAL_CONFIG_RELOAD_ADD_X_(event_ch);
    lan743x_csr_write(adapter, PTP_GENERAL_CONFIG, general_config);
    
    /* Reload-Periode setzen */
    lan743x_csr_write(adapter, PTP_CLOCK_TARGET_RELOAD_SEC_X(event_ch), period_sec);
    lan743x_csr_write(adapter, PTP_CLOCK_TARGET_RELOAD_NS_X(event_ch), period_nsec);
    
    /* Start-Zeit setzen (aktiviert den Output) */
    lan743x_csr_write(adapter, PTP_CLOCK_TARGET_SEC_X(event_ch), start_sec);
    lan743x_csr_write(adapter, PTP_CLOCK_TARGET_NS_X(event_ch), start_nsec);
    
    return 0;
}
```

---

## Konfiguration und Verwendung

### 1. Kernel-Konfiguration

```bash
# .config Optionen aktivieren
CONFIG_PTP_1588_CLOCK=y
CONFIG_NETWORK_PHY_TIMESTAMPING=y
CONFIG_LAN743X=y
```

### 2. Hardware-Erkennung

```bash
# PTP Hardware Clock Devices anzeigen
ls -la /dev/ptp*
# drwxr-xr-x 2 root root     80 Feb 21 10:00 .
# crw------- 1 root root 248, 0 Feb 21 10:00 ptp0
# crw------- 1 root root 248, 1 Feb 21 10:00 ptp1

# Hardware Capabilities prüfen  
ethtool -T eth0
# Time stamping parameters for eth0:
# Capabilities:
#     hardware-transmit     (SOF_TIMESTAMPING_TX_HARDWARE)
#     software-transmit     (SOF_TIMESTAMPING_TX_SOFTWARE)
#     hardware-receive      (SOF_TIMESTAMPING_RX_HARDWARE)
#     software-receive      (SOF_TIMESTAMPING_RX_SOFTWARE)
#     software-system-clock (SOF_TIMESTAMPING_SOFTWARE)
#     hardware-raw-clock    (SOF_TIMESTAMPING_RAW_HARDWARE)
# PTP Hardware Clock: 0
# Hardware Transmit Timestamp Modes:
#     off                   (HWTSTAMP_TX_OFF)
#     on                    (HWTSTAMP_TX_ON)
# Hardware Receive Filter Modes:
#     none                  (HWTSTAMP_FILTER_NONE)
#     ptpv1-l4-event        (HWTSTAMP_FILTER_PTP_V1_L4_EVENT)
#     ptpv2-l4-event        (HWTSTAMP_FILTER_PTP_V2_L4_EVENT)
#     ptpv2-l2-event        (HWTSTAMP_FILTER_PTP_V2_L2_EVENT)
#     ptpv2-event           (HWTSTAMP_FILTER_PTP_V2_EVENT)
```

### 3. LinuxPTP Konfiguration

#### ptp4l.conf für LAN743x
```ini
[global]
# Dataset comparison
dataset_comparison     ieee1588
GM.capable             1
priority1              128
priority2              128
domainNumber           0
clockClass             248
clockAccuracy          0xFE
offsetScaledLogVariance 0xFFFF

# Network configuration  
network_transport      UDPv4
delay_mechanism        E2E

# Hardware timestamping
time_stamping          hardware
tx_timestamp_timeout   1
check_fup_sync         0

# Clock servo
pi_proportional_const  0.0
pi_integral_const      0.0
pi_proportional_scale  1.0
pi_proportional_exponent -0.3
pi_proportional_norm_max 0.7
pi_integral_scale      1.0
pi_integral_exponent   0.4
pi_integral_norm_max   0.3
step_threshold         0.0
first_step_threshold   0.00002
max_frequency          900000000

# Message rates (log2 values)
logAnnounceInterval    1      # 2 seconds
logSyncInterval        0      # 1 second
logMinDelayReqInterval 0      # 1 second

# Filters
freq_est_interval      1
delay_filter           moving_median
delay_filter_length   10

# GPIO und PPS (falls verwendet)
[eth0]
network_transport      UDPv4 
delay_mechanism        E2E
```

#### Starten der PTP Services
```bash
# PTP Master/Slave starten
sudo ptp4l -i eth0 -m -f ptp4l.conf

# System Clock synchronisieren
sudo phc2sys -s eth0 -m -w

# Als Systemd Services
sudo systemctl enable ptp4l@eth0
sudo systemctl enable phc2sys@eth0
sudo systemctl start ptp4l@eth0 
sudo systemctl start phc2sys@eth0
```

### 4. GPIO und Event Configuration

#### PPS Output konfigurieren
```bash
# testptp Tool verwenden
sudo testptp -d /dev/ptp0 -p 1000000000  # 1 PPS
sudo testptp -d /dev/ptp0 -P 1           # Enable PPS auf Pin 0

# Über ptp4l.conf
echo "ts2phc.pulsewidth 100000000" >> ptp4l.conf  # 100ms Pulses
echo "ts2phc.perout_phase 0" >> ptp4l.conf        # Phase Offset 
```

#### External Timestamping
```bash  
# External Events erfassen
sudo testptp -d /dev/ptp0 -e 1   # Enable External TS auf Index 1
sudo testptp -d /dev/ptp0 -w 10  # Warte auf Events (10s timeout)

# Event-Konfiguration in ptp4l.conf  
[eth0]
extts_polarity   rising
extts_correction 0
```

### 5. Real-world Deployment Beispiel

#### Industrielle Automatisierung Setup
```bash
#!/bin/bash
# PTP Master Konfiguration für Industrienetzwerk

# Network Interface konfigurieren
ip link set eth0 up
ip addr add 192.168.1.100/24 dev eth0

# Hardware Timestamping aktivieren
ethtool -s eth0 speed 1000 duplex full autoneg off

# PTP Master starten mit Industrial Profile
ptp4l -i eth0 -m -s -f industrial.conf &
PTP_PID=$!

# PHC zu System Clock synchronisieren
phc2sys -s eth0 -m -w &
PHC_PID=$!  

# PPS auf GPIO Pin 0 ausgeben (für Legacy-Geräte)
testptp -d /dev/ptp0 -p 1000000000 &
PPS_PID=$!

# Status monitoring
while true; do
    echo "PTP Status: $(pmc -u -b 0 'GET CURRENT_DATA_SET')"
    echo "Clock Offset: $(phc_ctl /dev/ptp0 get | grep offset)"
    sleep 30
done
```

---

## Debugging und Monitoring

### 1. Hardware-Status prüfen

#### Register-Dumps (nur Debug-Builds)
```bash  
# LAN743x Register über debugfs (falls verfügbar)
sudo cat /sys/kernel/debug/lan743x/eth0/registers | grep PTP
# PTP_CLOCK_SEC:     0x12345678
# PTP_CLOCK_NS:      0x87654321  
# PTP_CLOCK_SUBNS:   0x12345678
# PTP_CLOCK_RATE_ADJ: 0x00000000

# Über Kernel-Module Parameters
echo 2 > /sys/module/lan743x/parameters/msg_enable  # Enable debug messages
dmesg | grep lan743x | grep PTP
```

#### PHC Tools
```bash
# PHC Clock Status
phc_ctl /dev/ptp0 get
# phc: 1645454427.123456789
# sys: 1645454427.123457123  
# offset: -334

# Frequency Adjustment anzeigen
phc_ctl /dev/ptp0 freq
# 125000000.842

# Clock vergleichen
phc2sys -s eth0 -c CLOCK_REALTIME -n 1 -u 1
```

### 2. PTP Message Analysis

#### tcpdump für PTP Traffic  
```bash
# PTP Messages capturen (Port 319/320)
sudo tcpdump -i eth0 -v port 319 or port 320
# 10:30:45.123456 IP 192.168.1.100.319 > 224.0.1.129.319: PTPv2, Sync Message, length 44
# 10:30:45.123457 IP 192.168.1.100.319 > 224.0.1.129.319: PTPv2, Follow_Up Message, length 44

# Mit Hardware Timestamps
sudo tcpdump -i eth0 -j adapter_unsynced -ttt port 319
```

#### Wireshark Analysis
```bash  
# Wireshark mit PTP Decoder
wireshark -i eth0 -f "port 319 or port 320" &
# Analysiere:
# - Timestamp Correlation
# - Message Sequencing  
# - Delay Measurements
# - Clock Quality
```

### 3. Performance Monitoring

#### PTP Servo Statistics
```bash
# ptp4l Statistiken
pmc -u -b 0 'GET CURRENT_DATA_SET' 
# CURRENT_DATA_SET
#     stepsRemoved     1
#     offsetFromMaster -334.0
#     meanPathDelay    2847.0

pmc -u -b 0 'GET PARENT_DATA_SET'
# PARENT_DATA_SET  
#     parentPortIdentity            001122.fffe.334455-1
#     parentStats                   0
#     observedParentOffsetScaledLogVariance 0x436a
#     observedParentClockPhaseChangeRate 0x80000000

# Timing Statistics sammeln
while true; do
    offset=$(pmc -u -b 0 'GET CURRENT_DATA_SET' | grep offsetFromMaster | awk '{print $2}')
    echo "$(date +%s.%N), $offset" >> ptp_offset.log
    sleep 1  
done
```

#### System Performance Impact
```bash
# CPU Usage monitoring  
top -p $(pgrep ptp4l) -p $(pgrep phc2sys)
#   PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
#  1234 root      20   0   12345   1234   1234 S   2.3  0.1   0:12.34 ptp4l
#  1235 root      20   0    5678    567    567 S   1.2  0.1   0:05.67 phc2sys

# Interrupt Statistics
cat /proc/interrupts | grep eth0
#   24:    1234567          0   PCI-MSI-edge      eth0-0

# Network Interface Statistics  
ethtool -S eth0 | grep -i ptp
# ptp_tx_timestamp_count: 12345
# ptp_rx_timestamp_count: 67890
# ptp_timestamp_errors: 0
```

### 4. Common Issues und Solutions

#### Problem: Hoher Jitter
```bash
# Mögliche Ursachen prüfen:
# 1. Network Congestion
iftop -i eth0
ethtool -S eth0 | grep -E "(drop|error|collision)"

# 2. IRQ Affinity
cat /proc/irq/24/smp_affinity  
echo 2 > /proc/irq/24/smp_affinity  # Isoliere auf CPU 1

# 3. Real-time Kernel  
uname -r | grep rt
chrt -f 50 ptp4l -i eth0 -m -f ptp4l.conf  # Real-time Priority
```

#### Problem: Timestamp Errors
```bash
# Hardware Timestamp Capabilities prüfen
ethtool -T eth0
dmesg | grep -i timestamp

# Driver Debug aktivieren
echo 'module lan743x +p' > /sys/kernel/debug/dynamic_debug/control
dmesg -w | grep lan743x

# Hardware Reset durchführen
ip link set eth0 down && sleep 1 && ip link set eth0 up
```

#### Problem: Clock Drift
```bash
# PHC vs System Clock Vergleich
for i in {1..10}; do
    PHC=$(phc_ctl /dev/ptp0 get | awk '{print $2}')
    SYS=$(date +%s.%N)
    echo "PHC: $PHC, SYS: $SYS, DIFF: $(echo "$PHC - $SYS" | bc -l)"
    sleep 1
done

# Temperature Compensation (falls verfügbar)
# Manche Hardware hat Temperature-Sensoren für bessere Stabilität
sensors | grep -i temp
```

### 5. Automatisierte Tests

#### PTP Conformance Test
```bash
#!/bin/bash
# Automated PTP Performance Test

TEST_DURATION=3600  # 1 hour
SAMPLES=3600       # 1 sample per second
LOG_FILE="ptp_test_$(date +%Y%m%d_%H%M%S).log"

echo "Starting PTP Performance Test - Duration: ${TEST_DURATION}s" | tee $LOG_FILE

# Start PTP services
ptp4l -i eth0 -m -s &
PTP_PID=$!
sleep 5

phc2sys -s eth0 -m -w &  
PHC_PID=$!
sleep 5

# Collect statistics
for i in $(seq 1 $SAMPLES); do
    TIMESTAMP=$(date +%s.%N)
    OFFSET=$(pmc -u -b 0 'GET CURRENT_DATA_SET' 2>/dev/null | grep offsetFromMaster | awk '{print $2}')
    DELAY=$(pmc -u -b 0 'GET CURRENT_DATA_SET' 2>/dev/null | grep meanPathDelay | awk '{print $2}')
    
    if [[ -n "$OFFSET" ]] && [[ -n "$DELAY" ]]; then
        echo "$TIMESTAMP,$OFFSET,$DELAY" >> $LOG_FILE
    fi
    
    sleep 1
done

# Cleanup
kill $PTP_PID $PHC_PID 2>/dev/null

# Analysis
echo "Test Results:" | tee -a $LOG_FILE
awk -F, 'NR>1 {sum+=$2; sumsq+=$2*$2; n++} END {
    mean=sum/n; 
    variance=sumsq/n - mean*mean; 
    stddev=sqrt(variance);
    print "Samples: " n;
    print "Mean Offset: " mean " ns";  
    print "Std Deviation: " stddev " ns";
    print "RMS: " sqrt(sumsq/n) " ns";
}' $LOG_FILE | tee -a $LOG_FILE
```

---

## 9. Strategie: PTP-Support im LAN865x Treiber implementieren

### Überblick der Implementierungsstrategie

**Wichtige Erkenntnis**: Die LAN865x-Hardware ist bereits **PTP-ready**! Der bestehende Treiber enthält konkrete Hinweise auf eine funktionsfähige PTP-Hardware-Implementierung.

#### **🔍 Konkrete PTP-Hardware-Evidenz im bestehenden LAN865x-Treiber**

##### **1. TSU (Time Sync Unit) Register bereits definiert**

Im aktuellen `lan865x.c` Code finden wir:

```c
/* MAC TSU Timer Increment Register */
#define LAN865X_REG_MAC_TSU_TIMER_INCR      0x00010077
#define MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS 0x0028
```

**TSU = Time Sync Unit** → Das ist offizielle PTP-Hardware-Terminologie!

##### **2. Expliziter PTP-Hardware-Kommentar in `lan865x_probe()`**

```c
/* LAN865x Rev.B0/B1 configuration parameters from AN1760
 * As per the Configuration Application Note AN1760 published in the
 * link, https://www.microchip.com/en-us/application-notes/an1760  
 * Revision F (DS60001760G - June 2024), configure the MAC to set time
 * stamping at the end of the Start of Frame Delimiter (SFD) and set the
 * Timer Increment reg to 40 ns to be used as a 25 MHz internal clock.
 */
ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
                            MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
```

**Schlüssel-Evidenz:**
- ✅ **"time stamping"** → PTP-Timestamping explizit erwähnt!
- ✅ **"end of Start of Frame Delimiter (SFD)"** → Hardware-Timestamping am Wire!
- ✅ **"40 ns"** → PTP-Clock-Auflösung (0x0028 = 40 Nanosekunden)!
- ✅ **"25 MHz internal clock"** → 1/40ns = 25MHz PTP-Referenz-Takt!
- ✅ **"AN1760"** → Microchip Application Note explizit über PTP-Timing!

##### **3. Hardware wird bereits konfiguriert**

Der Treiber aktiviert die PTP-Hardware bereits beim Startup:

```c
ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
                            MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
if (ret) {
    dev_err(&spi->dev, "Failed to config TSU Timer Incr reg: %d\n", ret);
    goto oa_tc6_exit;
}
```

**Bedeutung:**
- **Register 0x00010077**: Dedicated Time-Sync-Hardware-Register
- **0x0028 (40ns)**: Optimale PTP-Clock-Auflösung  
- **25 MHz**: Standard-PTP-Frequenz für Nanosekunden-Genauigkeit
- **SFD-Timestamping**: Hardware-Timestamping bereits konfiguriert

##### **4. Status Quo: Hardware läuft, Software fehlt**

| Komponente | Status | Details |
|-----------|--------|---------|
| **PTP-Clock** | ✅ **Läuft bereits** | 25MHz/40ns Auflösung aktiv |
| **SFD-Timestamping** | ✅ **Konfiguriert** | Hardware-Timestamping am Wire |  
| **TSU-Register** | ✅ **Implementiert** | Timer Increment Register aktiv |
| **Register-Map** | ⚠️ **Teilweise** | TSU vorhanden, PTP-Interface fehlt |
| **PTP-Framework** | ❌ **Fehlt komplett** | Keine `ptp_clock_register()` |
| **Clock-Interface** | ❌ **Fehlt** | `adjfine`, `gettime` etc. nicht implementiert |
| **Timestamp-Handling** | ❌ **Fehlt** | TX/RX-Timestamp-Management fehlt |

**Fazit**: Die **Hardware ist PTP-ready** - nur die **Software-Schicht muss ergänzt** werden!

#### **🔗 LAN865x PTP im Kontext von PLCA (Physical Layer Collision Avoidance)**

Der LAN865x als **10BASE-T1S MAC-PHY** unterstützt **PLCA**, was die PTP-Implementierung besonders wertvoll macht:

##### **PLCA + PTP Synergie-Effekte:**

```
10BASE-T1S Multi-Drop Topology mit PLCA:
┌─────────────────────────────────────────────────────────────┐
│                Single Pair Ethernet Bus                     │
│  Node 1      Node 2      Node 3      Node 4      Node 5    │
│ (Master)    (Slave)     (Slave)     (Slave)     (Slave)    │
│    │          │           │           │           │        │
│ LAN865x   LAN865x     LAN865x     LAN865x     LAN865x      │
│ PTP+PLCA  PTP+PLCA   PTP+PLCA   PTP+PLCA   PTP+PLCA       │
└─────────────────────────────────────────────────────────────┘
```

##### **1. Deterministische Latenz durch PLCA**

**PLCA-Mechanismus:**
- **Token-basiert**: Jeder Node erhält deterministischen Sendezugriff
- **Bekannte Zykluszeit**: Vorhersagbare Frame-Delays
- **Collision-frei**: Keine zufälligen Backoff-Zeiten

**PTP-Vorteil:**
```c
// Ohne PLCA (CSMA/CD): Variable Delays
Frame_Delay = Medium_Access_Time + Collision_Backoff + Transmission_Time
//            ↑ 0-51.2µs random  ↑ 0-∞ exponential

// Mit PLCA: Deterministische Delays  
Frame_Delay = PLCA_Cycle_Position × Beacon_Timer + Transmission_Time
//            ↑ vorhersagbar      ↑ konstant
```

**Resultat**: **Sub-Mikrosekunden PTP-Genauigkeit** auch in Multi-Node-Netzwerken!

##### **2. PLCA-Aware PTP-Implementierung**

Der LAN865x könnte **PLCA-optimierte PTP-Features** bieten:

```c
/* Hypothetische PLCA-PTP Register-Erweiterungen */
#define LAN865X_REG_PLCA_PTP_CONFIG        0x00010080
#define LAN865X_REG_PLCA_BEACON_SYNC       0x00010081  
#define LAN865X_REG_PLCA_TIMESTAMP_OFFSET  0x00010082

/* PLCA-spezifische PTP-Konfiguration */
#define PLCA_PTP_BEACON_SYNC_EN     BIT(0)  // Sync to PLCA Beacon
#define PLCA_PTP_COMP_DELAYS        BIT(1)  // Compensate PLCA Delays  
#define PLCA_PTP_NODE_PRIORITY      GENMASK(7,4) // PTP Priority per Node
```

##### **3. Industrial Automation Use Case**

**Typische Anwendung**: Industrie 4.0 Sensor-Netzwerk
```
Factory Floor 10BASE-T1S + PLCA + PTP:

PLC/Master ──┬── Sensor Node 1 (Temperatur)    
             ├── Sensor Node 2 (Vibration)     
             ├── Sensor Node 3 (Druck)         
             ├── Actuator Node 4 (Ventil)      
             └── HMI Node 5 (Display)          

• PLCA: Garantierte 1ms Zykluszeit
• PTP:  Nanosekunden-synchrone Samples
• LAN865x: Hardware-Timestamping für alle Nodes
```

**Vorteile der Kombination:**
- ⚡ **Echtzeit-fähig**: PLCA + PTP = deterministische Kommunikation
- 🎯 **Präzise Synchronisation**: Hardware-Timestamps trotz Multi-Drop
- 💡 **Energie-effizient**: Single-Pair-Verkabelung
- 🔧 **Skalierbar**: Bis zu 8 Nodes pro Segment

##### **4. PLCA-Timestamp-Kompensation**

**Challenge**: PLCA fügt deterministische, aber **node-spezifische Delays** hinzu:

```c
/* PLCA-Delay-Berechnung für PTP-Timestamps */
static u32 lan865x_plca_delay_compensation(struct lan865x_adapter *adapter, 
                                          u8 node_id, u32 beacon_period)
{
    /* Node-Position im PLCA-Zyklus */
    u32 plca_slot_delay = node_id * beacon_period / max_nodes;
    
    /* Medium-Propagation (Single-Pair-Cable) */  
    u32 cable_delay = adapter->cable_length_m * 5; // ~5ns/m für Copper
    
    /* PLCA-Protocol-Overhead */
    u32 plca_overhead = 2400; // 24 Bit @ 10 Mbps = 2.4µs
    
    return plca_slot_delay + cable_delay + plca_overhead;
}

/* PTP-Timestamp mit PLCA-Compensation */
static void lan865x_ptp_rx_timestamp_plca(struct lan865x_adapter *adapter,
                                          struct sk_buff *skb)  
{
    u32 hw_timestamp_ns;
    u32 plca_compensation;
    ktime_t corrected_timestamp;
    
    /* Hardware-Timestamp lesen */
    hw_timestamp_ns = lan865x_read_rx_timestamp(adapter);
    
    /* PLCA-Delay kompensieren */
    plca_compensation = lan865x_plca_delay_compensation(adapter, 
                                                       adapter->plca_node_id,
                                                       adapter->plca_beacon_timer);
    
    /* Korrigierter Timestamp */
    corrected_timestamp = ns_to_ktime(hw_timestamp_ns - plca_compensation);
    
    /* SKB-Timestamp setzen */
    skb_hwtstamps(skb)->hwtstamp = corrected_timestamp;
}
```

##### **5. Multi-Master PLCA-PTP-Topology** 

**Advanced Use Case**: Redundante Master mit PLCA:

```
Redundant PTP Masters in PLCA Network:

Primary Master ──┬── Node 1 ── Node 2 ── Node 3 ──┐
                 │                                 │
Backup Master ───┴─────────────────────────────────┘

• PLCA: Beide Master haben Sendeslots
• PTP:  Best-Master-Clock-Algorithm (BMCA) 
• Failover: <1 PLCA-Cycle (typ. <1ms)
```

**Implementation:**
```c
/* PLCA-aware BMCA */
static int lan865x_ptp_plca_bmca(struct lan865x_adapter *adapter)
{
    if (adapter->plca_node_id == 0) {
        /* Node 0 = Primary Master Slot */
        return PTP_MASTER_PRIORITY_1;
    } else if (adapter->plca_node_id == 1) {
        /* Node 1 = Backup Master Slot */  
        return PTP_MASTER_PRIORITY_2;
    } else {
        /* Slave Nodes */
        return PTP_SLAVE_ONLY;
    }
}
```

##### **6. Performance-Expectations: PLCA + PTP**

| Metrik | Standard Ethernet | 10BASE-T1S + PLCA + PTP |
|--------|------------------|--------------------------|
| **Jitter** | ±10-100µs | ±100-500ns |  
| **Latency** | Variable | Deterministic |
| **Sync-Time** | 30-60s | 5-15s |
| **Topology** | Point-to-Point | Multi-Drop (8 Nodes) |
| **Cable** | 4-Pair | Single-Pair |
| **Power** | Standard PoE | PoDL (Power-over-DataLine) |

**Fazit**: **PLCA + PTP auf LAN865x** ermöglicht **industrielle Echtzeit-Synchronisation** mit **Single-Pair-Ethernet** - ideal für Industrie 4.0, Automotive und IoT-Anwendungen!

Basierend auf dieser bestehenden Hardware-Infrastruktur und den bereits vorhandenen TSU-Hardware-Hints, hier eine konkrete Strategie zur Implementierung von Hardware PTP-Support:

### 9.1. Phasenplan

#### **Phase 1: Hardware-Analyse und Register-Mapping**
```
1. Hardware-Dokumentation beschaffen
   ├── LAN865x Datasheet (PTP/TSU Sektion)
   ├── Register-Map für PTP-Funktionen  
   ├── Application Note AN1760 (bereits teilweise implementiert)
   └── Hardware-Evaluierung mit Oszilloskop/Logic-Analyzer

2. Bestehende TSU-Funktionalität analysieren
   ├── TSU_TIMER_INCR Register (bereits in Gebrauch)
   ├── SFD-Timestamping Konfiguration
   └── 25MHz PTP-Clock-Verifikation
```

#### **Phase 2: Treiber-Struktur erweitern**
```
3. PTP-Framework Integration vorbereiten
   ├── Datenstrukturen erweitern
   ├── Register-Definitionen hinzufügen
   ├── Grundlegende PTP-Funktionen implementieren
   └── Build-System anpassen
```

#### **Phase 3: Core PTP-Funktionen**
```
4. PTP Clock Interface implementieren
   ├── adjtime/adjfine Operations
   ├── gettime/settime Operations
   ├── Hardware Clock-Management
   └── SPI-Access-Optimierung

5. Hardware Timestamping
   ├── TX-Timestamp Handling
   ├── RX-Timestamp Handling  
   ├── FIFO-Management
   └── Interrupt-Integration
```

#### **Phase 4: Advanced Features**
```
6. GPIO und Events (falls verfügbar)
   ├── Periodic Output (PPS)
   ├── External Timestamps
   ├── Event Channels
   └── Pin-Configuration

7. Testing und Optimierung
   ├── LinuxPTP Integration
   ├── Performance-Messungen
   ├── Stress-Testing
   └── Dokumentation
```

### 9.2. Detaillierte Implementierung

#### **Schritt 1: Header-Datei erweitern**

Neue Datei: `drivers/net/ethernet/microchip/lan865x/lan865x_ptp.h`

```c
/* SPDX-License-Identifier: GPL-2.0+ */
/* LAN865x PTP Hardware Support */

#ifndef _LAN865X_PTP_H
#define _LAN865X_PTP_H

#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>

/* PTP Register Definitions (basierend auf TSU-Hardware) */
#define LAN865X_REG_PTP_CMD_CTL            0x00010070
#define LAN865X_REG_PTP_CLOCK_SEC          0x00010071
#define LAN865X_REG_PTP_CLOCK_NS           0x00010072
#define LAN865X_REG_PTP_CLOCK_SUBNS        0x00010073
#define LAN865X_REG_PTP_RATE_ADJ           0x00010074
#define LAN865X_REG_PTP_STEP_ADJ_SEC       0x00010075
#define LAN865X_REG_PTP_STEP_ADJ_NS        0x00010076
/* 0x00010077 = TSU_TIMER_INCR bereits definiert */
#define LAN865X_REG_PTP_TX_TS_FIFO         0x00010078
#define LAN865X_REG_PTP_RX_TS_FIFO         0x00010079
#define LAN865X_REG_PTP_INT_STS            0x0001007A
#define LAN865X_REG_PTP_INT_EN             0x0001007B

/* Command Control Bits */
#define LAN865X_PTP_CMD_ENABLE             BIT(0)
#define LAN865X_PTP_CMD_DISABLE            BIT(1)
#define LAN865X_PTP_CMD_CLOCK_READ         BIT(2)
#define LAN865X_PTP_CMD_CLOCK_LOAD         BIT(3)
#define LAN865X_PTP_CMD_STEP_SEC           BIT(4)
#define LAN865X_PTP_CMD_STEP_NS            BIT(5)

/* Rate Adjustment */
#define LAN865X_PTP_RATE_ADJ_DIR           BIT(31)
#define LAN865X_PTP_MAX_ADJ_PPB            31250000  // ±31.25 ppm

/* Interrupt Bits */
#define LAN865X_PTP_INT_TX_TS              BIT(0)
#define LAN865X_PTP_INT_RX_TS              BIT(1)
#define LAN865X_PTP_INT_TS_OVERFLOW        BIT(2)

/* Timestamp Queue Size */
#define LAN865X_PTP_TX_TS_QUEUE_SIZE       4
#define LAN865X_PTP_RX_TS_QUEUE_SIZE       8

/* PTP State Flags */
#define LAN865X_PTP_FLAG_ENABLED           BIT(0)
#define LAN865X_PTP_FLAG_TX_TS_ENABLED     BIT(1)
#define LAN865X_PTP_FLAG_RX_TS_ENABLED     BIT(2)

struct lan865x_adapter;

/* PTP Timestamp Entry */
struct lan865x_ptp_ts {
    u32 sec;
    u32 nsec;
    u16 seq_id;
    u8 msg_type;
    u8 domain;
};

/* PTP Private Data */
struct lan865x_ptp {
    struct ptp_clock_info ptp_clock_info;
    struct ptp_clock *ptp_clock;
    struct mutex cmd_lock;
    spinlock_t ts_lock;
    
    /* TX Timestamp Queue */
    struct sk_buff *tx_ts_skb_queue[LAN865X_PTP_TX_TS_QUEUE_SIZE];
    struct lan865x_ptp_ts tx_ts_queue[LAN865X_PTP_TX_TS_QUEUE_SIZE];
    u32 tx_ts_head;
    u32 tx_ts_tail;
    u32 tx_ts_count;
    
    /* RX Timestamp Queue */
    struct lan865x_ptp_ts rx_ts_queue[LAN865X_PTP_RX_TS_QUEUE_SIZE];
    u32 rx_ts_head;
    u32 rx_ts_tail;
    u32 rx_ts_count;
    
    /* Statistics */
    u64 tx_ts_processed;
    u64 rx_ts_processed;
    u64 ts_errors;
    
    u32 flags;
};

/* Function Prototypes */
int lan865x_ptp_init(struct lan865x_adapter *adapter);
int lan865x_ptp_open(struct lan865x_adapter *adapter);
void lan865x_ptp_close(struct lan865x_adapter *adapter);
void lan865x_ptp_remove(struct lan865x_adapter *adapter);

bool lan865x_ptp_is_ptp_frame(struct sk_buff *skb);
int lan865x_ptp_tx_timestamp_enable(struct lan865x_adapter *adapter, struct sk_buff *skb);
void lan865x_ptp_tx_timestamp_process(struct lan865x_adapter *adapter);
void lan865x_ptp_rx_timestamp_process(struct lan865x_adapter *adapter, struct sk_buff *skb);

#endif /* _LAN865X_PTP_H */
```

#### **Schritt 2: Basis-Datenstruktur erweitern**

In `lan865x.c` die `lan865x_priv` Struktur erweitern:

```c
#include "lan865x_ptp.h"

struct lan865x_priv {
    struct work_struct multicast_work;
    struct net_device *netdev;
    struct spi_device *spi;
    struct oa_tc6 *tc6;
    
    /* PTP Support hinzufügen */
    struct lan865x_ptp ptp;
    bool ptp_enabled;
};
```

#### **Schritt 3: PTP Clock Interface implementieren**

Neue Datei: `drivers/net/ethernet/microchip/lan865x/lan865x_ptp.c`

```c
#include <linux/module.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/oa_tc6.h>
#include "lan865x_ptp.h"

/* Helper function: Wait for PTP command completion */
static int lan865x_ptp_wait_cmd_complete(struct lan865x_adapter *adapter, u32 cmd)
{
    int timeout = 1000; /* 1ms timeout */
    u32 status;
    int ret;
    
    while (timeout--) {
        ret = oa_tc6_read_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, &status);
        if (ret)
            return ret;
            
        if (!(status & cmd))
            return 0; /* Command completed */
            
        usleep_range(1, 10);
    }
    
    return -ETIMEDOUT;
}

/* PTP Clock adjfine implementation */
static int lan865x_ptp_adjfine(struct ptp_clock_info *ptp_info, long scaled_ppm)
{
    struct lan865x_ptp *ptp = container_of(ptp_info, struct lan865x_ptp, ptp_clock_info);
    struct lan865x_adapter *adapter = container_of(ptp, struct lan865x_adapter, ptp);
    u32 adj_val = 0;
    u64 adj_abs;
    bool negative;
    int ret;
    
    if (scaled_ppm > LAN865X_PTP_MAX_ADJ_PPB || scaled_ppm < -LAN865X_PTP_MAX_ADJ_PPB)
        return -ERANGE;
    
    negative = scaled_ppm < 0;
    adj_abs = negative ? -scaled_ppm : scaled_ppm;
    
    /* Convert scaled_ppm to hardware register value
     * Hardware expects: (2^32 * adj_ppm) / 1e9
     * For 25MHz base clock: adj_val = (adj_abs * 2^32) / 25e6
     */
    adj_val = (u32)div_u64(adj_abs * (1ULL << 32), 25000000ULL);
    
    if (negative)
        adj_val |= LAN865X_PTP_RATE_ADJ_DIR;
    
    mutex_lock(&ptp->cmd_lock);
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_RATE_ADJ, adj_val);
    mutex_unlock(&ptp->cmd_lock);
    
    return ret;
}

/* PTP Clock adjtime implementation */
static int lan865x_ptp_adjtime(struct ptp_clock_info *ptp_info, s64 delta)
{
    struct lan865x_ptp *ptp = container_of(ptp_info, struct lan865x_ptp, ptp_clock_info);
    struct lan865x_adapter *adapter = container_of(ptp, struct lan865x_adapter, ptp);
    u32 sec_delta, nsec_delta;
    bool negative = delta < 0;
    int ret;
    
    if (negative)
        delta = -delta;
    
    sec_delta = div_u64_rem(delta, NSEC_PER_SEC, &nsec_delta);
    
    mutex_lock(&ptp->cmd_lock);
    
    /* Set step values */
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_STEP_ADJ_SEC, sec_delta);
    if (ret)
        goto exit;
        
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_STEP_ADJ_NS, nsec_delta);
    if (ret)
        goto exit;
    
    /* Execute step command */
    if (negative) {
        ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                                   LAN865X_PTP_CMD_STEP_SEC | LAN865X_PTP_RATE_ADJ_DIR);
    } else {
        ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                                   LAN865X_PTP_CMD_STEP_SEC);
    }
    
    if (!ret)
        ret = lan865x_ptp_wait_cmd_complete(adapter, LAN865X_PTP_CMD_STEP_SEC);

exit:
    mutex_unlock(&ptp->cmd_lock);
    return ret;
}

/* PTP Clock gettime64 implementation */
static int lan865x_ptp_gettime64(struct ptp_clock_info *ptp_info, struct timespec64 *ts)
{
    struct lan865x_ptp *ptp = container_of(ptp_info, struct lan865x_ptp, ptp_clock_info);
    struct lan865x_adapter *adapter = container_of(ptp, struct lan865x_adapter, ptp);
    u32 sec, nsec;
    int ret;
    
    mutex_lock(&ptp->cmd_lock);
    
    /* Trigger clock read */
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                               LAN865X_PTP_CMD_CLOCK_READ);
    if (ret)
        goto exit;
        
    ret = lan865x_ptp_wait_cmd_complete(adapter, LAN865X_PTP_CMD_CLOCK_READ);
    if (ret)
        goto exit;
    
    /* Read timestamp */
    ret = oa_tc6_read_register(adapter->tc6, LAN865X_REG_PTP_CLOCK_SEC, &sec);
    if (ret)
        goto exit;
        
    ret = oa_tc6_read_register(adapter->tc6, LAN865X_REG_PTP_CLOCK_NS, &nsec);
    if (!ret) {
        ts->tv_sec = sec;
        ts->tv_nsec = nsec;
    }

exit:
    mutex_unlock(&ptp->cmd_lock);
    return ret;
}

/* PTP Clock settime64 implementation */
static int lan865x_ptp_settime64(struct ptp_clock_info *ptp_info, const struct timespec64 *ts)
{
    struct lan865x_ptp *ptp = container_of(ptp_info, struct lan865x_ptp, ptp_clock_info);
    struct lan865x_adapter *adapter = container_of(ptp, struct lan865x_adapter, ptp);
    int ret;
    
    mutex_lock(&ptp->cmd_lock);
    
    /* Set clock values */
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CLOCK_SEC, (u32)ts->tv_sec);
    if (ret)
        goto exit;
        
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CLOCK_NS, (u32)ts->tv_nsec);
    if (ret)
        goto exit;
    
    /* Load clock */
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                               LAN865X_PTP_CMD_CLOCK_LOAD);
    if (!ret)
        ret = lan865x_ptp_wait_cmd_complete(adapter, LAN865X_PTP_CMD_CLOCK_LOAD);

exit:
    mutex_unlock(&ptp->cmd_lock);
    return ret;
}

/* PTP Clock enable (placeholder for future extensions) */
static int lan865x_ptp_enable(struct ptp_clock_info *ptp_info, struct ptp_clock_request *req, int on)
{
    /* Future: GPIO/PPS support */
    return -EOPNOTSUPP;
}

/* Initialize PTP subsystem */
int lan865x_ptp_init(struct lan865x_adapter *adapter)
{
    struct lan865x_ptp *ptp = &adapter->ptp;
    
    mutex_init(&ptp->cmd_lock);
    spin_lock_init(&ptp->ts_lock);
    
    /* Initialize PTP clock info */
    ptp->ptp_clock_info.owner = THIS_MODULE;
    snprintf(ptp->ptp_clock_info.name, sizeof(ptp->ptp_clock_info.name),
             "lan865x_%s", adapter->netdev->name);
    
    ptp->ptp_clock_info.max_adj = LAN865X_PTP_MAX_ADJ_PPB;
    ptp->ptp_clock_info.n_alarm = 0;
    ptp->ptp_clock_info.n_ext_ts = 0;  /* Future: External timestamping */
    ptp->ptp_clock_info.n_per_out = 0; /* Future: Periodic output */
    ptp->ptp_clock_info.n_pins = 0;    /* Future: GPIO pins */
    ptp->ptp_clock_info.pps = 0;       /* Future: PPS support */
    
    ptp->ptp_clock_info.adjfine = lan865x_ptp_adjfine;
    ptp->ptp_clock_info.adjtime = lan865x_ptp_adjtime;
    ptp->ptp_clock_info.gettime64 = lan865x_ptp_gettime64;
    ptp->ptp_clock_info.settime64 = lan865x_ptp_settime64;
    ptp->ptp_clock_info.enable = lan865x_ptp_enable;
    
    return 0;
}

/* Open PTP (register with PTP subsystem) */
int lan865x_ptp_open(struct lan865x_adapter *adapter)
{
    struct lan865x_ptp *ptp = &adapter->ptp;
    int ret;
    
    /* Enable PTP hardware */
    ret = oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                               LAN865X_PTP_CMD_ENABLE);
    if (ret) {
        netdev_err(adapter->netdev, "Failed to enable PTP hardware: %d\n", ret);
        return ret;
    }
    
    /* Register PTP clock */
    ptp->ptp_clock = ptp_clock_register(&ptp->ptp_clock_info, &adapter->spi->dev);
    if (IS_ERR(ptp->ptp_clock)) {
        ret = PTR_ERR(ptp->ptp_clock);
        netdev_err(adapter->netdev, "Failed to register PTP clock: %d\n", ret);
        return ret;
    }
    
    ptp->flags |= LAN865X_PTP_FLAG_ENABLED;
    netdev_info(adapter->netdev, "PTP clock registered as ptp%d\n", 
                ptp->ptp_clock->index);
    
    return 0;
}

/* Close PTP */
void lan865x_ptp_close(struct lan865x_adapter *adapter)
{
    struct lan865x_ptp *ptp = &adapter->ptp;
    
    if (ptp->flags & LAN865X_PTP_FLAG_ENABLED) {
        if (ptp->ptp_clock) {
            ptp_clock_unregister(ptp->ptp_clock);
            ptp->ptp_clock = NULL;
        }
        
        /* Disable PTP hardware */
        oa_tc6_write_register(adapter->tc6, LAN865X_REG_PTP_CMD_CTL, 
                             LAN865X_PTP_CMD_DISABLE);
        
        ptp->flags &= ~LAN865X_PTP_FLAG_ENABLED;
    }
}

/* Remove PTP */
void lan865x_ptp_remove(struct lan865x_adapter *adapter)
{
    lan865x_ptp_close(adapter);
}
```

#### **Schritt 4: Integration in den Haupttreiber**

Modifikationen in `lan865x.c`:

```c
/* Am Anfang der Datei */
#include "lan865x_ptp.h"

/* In lan865x_probe() nach der TSU-Konfiguration hinzufügen: */
static int lan865x_probe(struct spi_device *spi)
{
    /* ... bestehender Code bis TSU-Konfiguration ... */
    
    ret = oa_tc6_write_register(priv->tc6, LAN865X_REG_MAC_TSU_TIMER_INCR,
                                MAC_TSU_TIMER_INCR_COUNT_NANOSECONDS);
    if (ret) {
        dev_err(&spi->dev, "Failed to config TSU Timer Incr reg: %d\n", ret);
        goto oa_tc6_exit;
    }

    /* PTP-Initialisierung hinzufügen */
    ret = lan865x_ptp_init(priv);
    if (ret) {
        dev_err(&spi->dev, "Failed to initialize PTP: %d\n", ret);
        goto oa_tc6_exit;
    }
    
    /* ... restlicher bestehender Code ... */
    
    ret = register_netdev(netdev);
    if (ret) {
        dev_err(&spi->dev, "Register netdev failed (ret = %d)", ret);
        goto oa_tc6_exit;
    }
    
    /* PTP nach netdev-Registrierung aktivieren */
    ret = lan865x_ptp_open(priv);
    if (ret) {
        dev_warn(&spi->dev, "PTP initialization failed: %d (continuing without PTP)\n", ret);
        /* Non-fatal: Network funktioniert ohne PTP */
    }

    return 0;

oa_tc6_exit:
    /* PTP cleanup hinzufügen bei Fehlern */
    lan865x_ptp_remove(priv);
    oa_tc6_exit(priv->tc6);
free_netdev:
    free_netdev(priv->netdev);
    return ret;
}

/* In lan865x_remove() hinzufügen: */
static void lan865x_remove(struct spi_device *spi)
{
    struct lan865x_priv *priv = spi_get_drvdata(spi);

    cancel_work_sync(&priv->multicast_work);
    
    /* PTP cleanup hinzufügen */
    lan865x_ptp_close(priv);
    
    unregister_netdev(priv->netdev);
    
    lan865x_ptp_remove(priv);  /* Final cleanup */
    
    oa_tc6_exit(priv->tc6);
    free_netdev(priv->netdev);
}
```

#### **Schritt 5: Timestamping Integration (Zukunft)**

Für vollständige PTP-Funktionalität müssen TX/RX-Timestamping-Hooks hinzugefügt werden:

```c
/* In TX-Path (lan865x_start_xmit() oder entsprechend): */
static netdev_tx_t lan865x_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
    struct lan865x_priv *priv = netdev_priv(netdev);
    
    /* PTP TX Timestamp Request */
    if (priv->ptp.flags & LAN865X_PTP_FLAG_TX_TS_ENABLED) {
        if (lan865x_ptp_is_ptp_frame(skb)) {
            lan865x_ptp_tx_timestamp_enable(priv, skb);
        }
    }
    
    /* ... bestehender TX-Code ... */
}

/* In RX-Path (wo Frames empfangen werden): */
static void lan865x_handle_rx_frame(struct lan865x_priv *priv, struct sk_buff *skb)
{
    /* PTP RX Timestamp Processing */
    if (priv->ptp.flags & LAN865X_PTP_FLAG_RX_TS_ENABLED) {
        if (lan865x_ptp_is_ptp_frame(skb)) {
            lan865x_ptp_rx_timestamp_process(priv, skb);
        }
    }
    
    /* ... bestehender RX-Code ... */
}
```

### 9.3. Build-System Integration

#### **Makefile erweitern:**

In `drivers/net/ethernet/microchip/lan865x/Makefile`:

```makefile
# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_LAN865X) += lan865x.o

lan865x-objs := lan865x.o
lan865x-$(CONFIG_PTP_1588_CLOCK) += lan865x_ptp.o
```

#### **Kconfig erweitern:**

In `drivers/net/ethernet/microchip/Kconfig`:

```kconfig
config LAN865X
    tristate "LAN865x support"
    depends on SPI
    select OA_TC6
    select PHYLIB
    help
      Support for the Microchip LAN865x 10Base-T1S Ethernet controllers.
      
      This driver supports the LAN8650/LAN8651 family of T1S MAC-PHY 
      controllers with integrated PTP timestamping capabilities.
      
      To compile this driver as a module, choose M here. The module
      will be called lan865x.
```

### 9.4. Testing-Strategie

#### **Unit Tests:**
```bash
# 1. Hardware-Erkennung
dmesg | grep lan865x
ls -la /dev/ptp*

# 2. Basis PTP-Funktionen
phc_ctl /dev/ptp1 get    # Clock lesen
phc_ctl /dev/ptp1 set $(date +%s.%N)  # Clock setzen
phc_ctl /dev/ptp1 adj 1000  # Frequency adjustment

# 3. LinuxPTP Integration
ptp4l -i eth1 -m -s     # Als Master
ptp4l -i eth1 -m        # Als Slave
```

#### **Integration Tests:**
```bash
# Multi-Node PTP-Netzwerk
# Node 1 (LAN743x Master):
ptp4l -i eth0 -m -s -f master.conf

# Node 2 (LAN865x Slave):  
ptp4l -i eth1 -m -f slave.conf

# Performance-Monitoring:
while true; do
    echo "$(date): $(pmc -u -b 0 'GET CURRENT_DATA_SET' | grep offset)"
    sleep 1
done
```

### 9.5. Troubleshooting-Checkliste

#### **Häufige Probleme:**
1. **Register-Zugriff fehlschlägt**: SPI-Timing, Power-Management
2. **PTP Clock nicht registriert**: CONFIG_PTP_1588_CLOCK=y fehlt
3. **Hoher Jitter**: SPI-Interrupt-Latenz, CPU-Load
4. **Timestamp-Fehler**: Hardware-FIFO-Overflow, falsche Register-Map

#### **Debug-Kommandos:**
```bash
# Kernel-Messages
dmesg | grep -E "(lan865x|ptp|1588)"

# PTP-Status
cat /sys/class/ptp/ptp1/clock_name
ethtool -T eth1

# Register-Dumps (Development)
echo 'module lan865x +p' > /sys/kernel/debug/dynamic_debug/control
```

### 9.6. Performance-Erwartungen

#### **Realistische Ziele:**
- **Genauigkeit**: ±1-5µs (SPI-Overhead vs. PCIe LAN743x ±100ns)
- **Jitter**: <10µs (abhängig von SPI-Clock und System-Load)
- **CPU-Overhead**: Minimal (Hardware-Timestamping)
- **Sync-Zeit**: <30s (Standard PTP-Konvergenz)

#### **Optimierungen:**
- **SPI-Clock maximieren**: Reduziert Register-Access-Latency
- **DMA-basierter SPI**: Verringert CPU-Interrupt-Load
- **Real-time Kernel**: Verbessert Interrupt-Latency
- **IRQ-Affinity**: Isoliert PTP-Processing auf dedizierte CPU

---

### A. Register-Referenz LAN743x

#### PTP Clock Registers
```c
/* Base PTP Registers */
#define PTP_CLOCK_SEC               0x0A00  /* PTP Clock Seconds */
#define PTP_CLOCK_NS                0x0A04  /* PTP Clock Nanoseconds */  
#define PTP_CLOCK_SUBNS             0x0A08  /* PTP Clock Sub-Nanoseconds */
#define PTP_CLOCK_RATE_ADJ          0x0A0C  /* PTP Clock Rate Adjustment */

/* PTP Command Control */
#define PTP_CMD_CTL                 0x0A10  /* PTP Command Control */
#define PTP_CMD_CTL_PTP_ENABLE_     BIT(0)
#define PTP_CMD_CTL_PTP_DISABLE_    BIT(1) 
#define PTP_CMD_CTL_PTP_CLOCK_READ_ BIT(3)
#define PTP_CMD_CTL_PTP_CLOCK_LOAD_ BIT(4)
#define PTP_CMD_CTL_PTP_CLOCK_STEP_SEC_ BIT(5)
#define PTP_CMD_CTL_PTP_CLOCK_STEP_NS_  BIT(6)

/* Event Channels */
#define PTP_CLOCK_TARGET_SEC_X(ch)  (0x0A20 + ((ch) * 0x10))
#define PTP_CLOCK_TARGET_NS_X(ch)   (0x0A24 + ((ch) * 0x10))
#define PTP_CLOCK_TARGET_RELOAD_SEC_X(ch) (0x0A28 + ((ch) * 0x10))
#define PTP_CLOCK_TARGET_RELOAD_NS_X(ch)  (0x0A2C + ((ch) * 0x10))

/* GPIO Configuration */ 
#define GPIO_CFG0                   0x0B00  /* GPIO Configuration 0 */
#define GPIO_CFG1                   0x0B04  /* GPIO Configuration 1 */
#define GPIO_CFG2                   0x0B08  /* GPIO Configuration 2 */  
#define GPIO_CFG3                   0x0B0C  /* GPIO Configuration 3 */

/* TX/RX Timestamp FIFOs */
#define PTP_TX_TIMESTAMP_FIFO       0x0A70  /* TX Timestamp FIFO */
#define PTP_RX_TIMESTAMP_FIFO       0x0A74  /* RX Timestamp FIFO */
```

### B. Nützliche Links

- **IEEE 1588 Standard**: [IEEE Std 1588-2019](https://standards.ieee.org/standard/1588-2019.html)
- **Linux PTP Project**: [linuxptp.sourceforge.net](http://linuxptp.sourceforge.net/)
- **Kernel Documentation**: [kernel.org/doc/Documentation/ptp/](https://www.kernel.org/doc/Documentation/ptp/)
- **Microchip LAN743x**: [Microchip Technology](https://www.microchip.com/)

### C. Glossar

| Begriff | Beschreibung |
|---------|--------------|
| **Boundary Clock** | PTP-Node, der sowohl Master als auch Slave sein kann |
| **Event Messages** | PTP-Nachrichten mit präzisen Timestamps (Sync, Delay_Req) |
| **General Messages** | PTP-Nachrichten ohne Timestamps (Announce, Follow_Up) |
| **Grandmaster** | Referenz-Clock im PTP-Netzwerk |
| **One-Step Clock** | Setzt Timestamp direkt in Event Messages |  
| **Ordinary Clock** | Einfache PTP-Node (nur Master oder Slave) |
| **PHC** | PTP Hardware Clock - Hardware-Clock in Netzwerk-Interface |
| **PPS** | Pulse Per Second - 1Hz Reference Signal |
| **Transparent Clock** | Korrigiert Delay durch Netzwerk-Equipment |
| **Two-Step Clock** | Sendet Timestamp in separater Follow_Up Message |

---

**Fazit**: 

Die Hardware PTP-Unterstützung in Linux bietet durch die enge Integration zwischen Kernel-Framework, Netzwerk-Treibern und spezialisierter Hardware eine hochpräzise Zeitsynchronisation. Am Beispiel des Microchip LAN743x wird deutlich, wie ein vollständiges PTP-Hardware-System implementiert wird - von der Register-Ebene bis zur Anwendungsschnittstelle. 

Die Kombination aus dedizierter Hardware, optimierten Treibern und dem robusten Linux PTP-Framework ermöglicht Genauigkeiten im Nanosekunden-Bereich, die für moderne industrielle und wissenschaftliche Anwendungen essentiell sind.