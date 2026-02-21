# Hardware PTP-Unterstützung in Linux

Eine ausführliche Anleitung zum Verständnis der Hardware PTP (Precision Time Protocol) Implementierung in Linux, mit praktischen Beispielen anhand des Microchip LAN743x Treibers.

## Inhaltsverzeichnis

1. [Hintergründe und Motivation](#hintergründe-und-motivation)
2. [PTP Grundlagen](#ptp-grundlagen)
3. [Linux PTP Framework](#linux-ptp-framework)
4. [Hardware vs. Software Timestamping](#hardware-vs-software-timestamping)
5. [LAN743x PTP Implementierung](#lan743x-ptp-implementierung)
6. [Code-Beispiele und Analyse](#code-beispiele-und-analyse)
7. [Konfiguration und Verwendung](#konfiguration-und-verwendung)
8. [Debugging und Monitoring](#debugging-und-monitoring)

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

Der Microchip LAN743x (LAN7430/7431) bietet umfassende Hardware PTP-Unterstützung:

```
┌─────────────────────────────────────────────────────────┐
│                    LAN743x SoC                          │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────┐ │
│  │ PCIe        │    │ PTP Engine   │    │ Ethernet    │ │
│  │ Interface   │◄──►│ - 125MHz Clk │◄──►│ MAC/PHY     │ │
│  │             │    │ - TX/RX FIFO │    │             │ │
│  │             │    │ - GPIO Pins  │    │             │ │
│  └─────────────┘    └──────────────┘    └─────────────┘ │
└─────────────────────────────────────────────────────────┘
```

**Spezifikationen:**
- **Clock-Frequenz**: 125 MHz (8ns Auflösung)
- **Frequenz-Adjustment**: ±31.25 ppm
- **Event-Kanäle**: 2 (Timer A/B)
- **GPIO-Pins**: 4 (LAN7430) / 12 (LAN7431)
- **Timestamp-FIFO**: Hardware-gepuffert

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
    
    /* Hardware-Register schreiben */
    lan743x_csr_write(adapter, PTP_CLOCK_RATE_ADJ, lan743x_rate_adj);
    
    return 0;
}
```

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

## Anhang

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