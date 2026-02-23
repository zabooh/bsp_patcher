/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Microchip LAN865x PTP Hardware Clock Support
 *
 * Author: BSP Patcher Project <info@example.com>
 */

#ifndef _LAN865X_PTP_H
#define _LAN865X_PTP_H

#include <linux/ptp_clock_kernel.h>
#include <linux/ptp_clock.h>
#include <linux/net_tstamp.h>
#include <linux/ktime.h>

/* Forward declaration */
struct lan865x_priv;
struct oa_tc6;

/* ==================== PTP REGISTER DEFINITIONS ==================== */

/* TSU (Time Sync Unit) Base Registers */
#define LAN865X_REG_MAC_TSU_TIMER_INCR          0x00010077  /* Already defined in main driver */

/* PTP Clock Registers */
#define LAN865X_REG_PTP_CLK_CMD                 0x00010070  /* PTP Clock Command */
#define LAN865X_REG_PTP_CLK_SEC_HIGH            0x00010071  /* Seconds High (32-bit) */
#define LAN865X_REG_PTP_CLK_SEC_LOW             0x00010072  /* Seconds Low (32-bit) */
#define LAN865X_REG_PTP_CLK_NS                  0x00010073  /* Nanoseconds (30-bit) */
#define LAN865X_REG_PTP_CLK_SUB_NS              0x00010074  /* Sub-nanoseconds (8-bit) */

/* PTP Rate Adjustment */
#define LAN865X_REG_PTP_RATE_ADJ_CMD            0x00010075  /* Rate Adjustment Command */
#define LAN865X_REG_PTP_RATE_ADJ_VALUE          0x00010076  /* Rate Adjustment Value */

/* PTP General Purpose Timer (for PPS) */
#define LAN865X_REG_PTP_GPT_CMD                 0x00010078  /* General Purpose Timer Command */
#define LAN865X_REG_PTP_GPT_SEC_HIGH            0x00010079  /* GPT Target Seconds High */
#define LAN865X_REG_PTP_GPT_SEC_LOW             0x0001007A  /* GPT Target Seconds Low */
#define LAN865X_REG_PTP_GPT_NS                  0x0001007B  /* GPT Target Nanoseconds */
#define LAN865X_REG_PTP_GPT_RELOAD              0x0001007C  /* GPT Reload Value (for periodic) */

/* PTP Interrupt and Status */
#define LAN865X_REG_PTP_INT_STS                 0x0001007D  /* PTP Interrupt Status */
#define LAN865X_REG_PTP_INT_EN                  0x0001007E  /* PTP Interrupt Enable */

/* GPIO and PPS Control */
#define LAN865X_REG_PTP_GPIO_CFG                0x0001007F  /* GPIO Configuration for PTP */
#define LAN865X_REG_PTP_PPS_CFG                 0x00010080  /* PPS Output Configuration */

/* ==================== PTP REGISTER BIT DEFINITIONS ==================== */

/* PTP Clock Command Register (0x00010070) */
#define LAN865X_PTP_CLK_CMD_CLOCK_READ          BIT(0)      /* Read current time */
#define LAN865X_PTP_CLK_CMD_CLOCK_LOAD          BIT(1)      /* Load time */
#define LAN865X_PTP_CLK_CMD_CLOCK_STEP          BIT(2)      /* Step time adjustment */
#define LAN865X_PTP_CLK_CMD_CLOCK_INC_DEC       BIT(3)      /* Inc/Dec step direction */
#define LAN865X_PTP_CLK_CMD_CLOCK_ENABLE        BIT(4)      /* Enable PTP clock */ 

/* PTP Rate Adjustment Command Register (0x00010075) */
#define LAN865X_PTP_RATE_CMD_RATE_TEMP_LOAD     BIT(0)      /* Load temporary rate */
#define LAN865X_PTP_RATE_CMD_RATE_PERM_LOAD     BIT(1)      /* Load permanent rate */
#define LAN865X_PTP_RATE_CMD_RATE_DIR           BIT(31)     /* Rate direction (0=slow, 1=fast) */

/* PTP General Purpose Timer Command Register (0x00010078) */
#define LAN865X_PTP_GPT_CMD_GPT_LOAD            BIT(0)      /* Load GPT target time */
#define LAN865X_PTP_GPT_CMD_GPT_ENABLE          BIT(1)      /* Enable GPT */
#define LAN865X_PTP_GPT_CMD_GPT_RELOAD_ENABLE   BIT(2)      /* Enable auto-reload (periodic) */
#define LAN865X_PTP_GPT_CMD_GPT_MODE_PULSE      BIT(3)      /* Pulse mode (vs level) */

/* PTP Interrupt Status/Enable Registers (0x0001007D/0x0001007E) */
#define LAN865X_PTP_INT_TX_TIMESTAMP_RDY        BIT(0)      /* TX Timestamp ready */
#define LAN865X_PTP_INT_RX_TIMESTAMP_RDY        BIT(1)      /* RX Timestamp ready */
#define LAN865X_PTP_INT_GPT_TARGET_REACHED      BIT(2)      /* GPT target reached (PPS event) */
#define LAN865X_PTP_INT_TIMESTAMP_OVERFLOW      BIT(3)      /* Timestamp FIFO overflow */

/* PTP GPIO Configuration Register (0x0001007F) */
#define LAN865X_PTP_GPIO_PPS_OUTPUT_EN          BIT(0)      /* Enable PPS output on GPIO */
#define LAN865X_PTP_GPIO_PPS_POLARITY           BIT(1)      /* PPS polarity (0=active_low, 1=active_high) */
#define LAN865X_PTP_GPIO_PPS_GPIO_SELECT        GENMASK(7,4) /* GPIO pin selection for PPS */

/* PTP PPS Configuration Register (0x00010080) */
#define LAN865X_PTP_PPS_WIDTH_MASK              GENMASK(15,0) /* PPS pulse width in ns */
#define LAN865X_PTP_PPS_RELOAD_SEC_MASK         GENMASK(31,16) /* PPS reload seconds */

/* ==================== PTP CONSTANTS ==================== */

#define LAN865X_PTP_CLOCK_FREQUENCY             25000000    /* 25 MHz PTP clock */
#define LAN865X_PTP_NANOSEC_PER_SEC             1000000000ULL
#define LAN865X_PTP_MAX_ADJ_PPB                 31250000    /* ±31.25 ppm (1.25% of 25MHz) */
#define LAN865X_PTP_TIMER_INCREMENT_NS          40          /* 40 ns per tick (25MHz) */

/* Default PPS Configuration */
#define LAN865X_PTP_PPS_DEFAULT_WIDTH_NS        100000000   /* 100ms pulse width */
#define LAN865X_PTP_PPS_DEFAULT_PERIOD_SEC      1           /* 1 second period */

/* ==================== PTP DATA STRUCTURES ==================== */

struct lan865x_ptp_clock_time {
    u32 sec_high;       /* High 32 bits of seconds */
    u32 sec_low;        /* Low 32 bits of seconds */
    u32 nanoseconds;    /* Nanoseconds (0-999999999) */
    u8  sub_nanoseconds; /* Sub-nanoseconds (0-255) */
};

struct lan865x_ptp_config {
    bool clock_enabled;          /* PTP clock enabled */
    bool pps_enabled;            /* PPS output enabled */
    u32  pps_width_ns;          /* PPS pulse width in nanoseconds */
    u8   pps_gpio_pin;          /* GPIO pin for PPS output */
    bool pps_polarity;          /* PPS polarity (true=active_high) */
    s32  rate_adjustment_ppb;   /* Current rate adjustment in ppb */
};

struct lan865x_ptp_adapter {
    struct lan865x_priv *adapter;       /* Back-pointer to main adapter */
    struct ptp_clock_info ptp_caps;     /* PTP capabilities */
    struct ptp_clock *ptp_clock;        /* Linux PTP clock device */
    
    struct lan865x_ptp_config config;   /* Current PTP configuration */
    
    /* Debug interface */
    struct dentry *debugfs_root;        /* debugfs root directory */
    
    /* Hardware state */
    spinlock_t ptp_lock;               /* Protect PTP registers */
    bool hardware_available;           /* Hardware PTP functionality available */
    
    /* Statistics */
    u64 tx_timestamps_sent;
    u64 rx_timestamps_received;
    u64 pps_events_generated;
};

/* ==================== FUNCTION PROTOTYPES ==================== */

/* Core PTP Functions */
int lan865x_ptp_init(struct lan865x_priv *adapter);
void lan865x_ptp_remove(struct lan865x_priv *adapter);
int lan865x_ptp_reset(struct lan865x_priv *adapter);

/* PTP Hardware Interface */
int lan865x_ptp_clock_read(struct lan865x_priv *adapter, struct lan865x_ptp_clock_time *time);
int lan865x_ptp_clock_set(struct lan865x_priv *adapter, const struct lan865x_ptp_clock_time *time);
int lan865x_ptp_clock_adjust(struct lan865x_priv *adapter, s64 delta_ns);
int lan865x_ptp_rate_adjust(struct lan865x_priv *adapter, s32 ppb);

/* PPS Functions */
int lan865x_ptp_pps_enable(struct lan865x_priv *adapter, bool enable);
int lan865x_ptp_pps_configure(struct lan865x_priv *adapter, u32 width_ns, u8 gpio_pin, bool polarity);
int lan865x_ptp_pps_set_target(struct lan865x_priv *adapter, const struct lan865x_ptp_clock_time *target_time);

/* Linux PTP Framework Interface */
int lan865x_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm);
int lan865x_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta);
int lan865x_ptp_gettime64(struct ptp_clock_info *ptp, struct timespec64 *ts);
int lan865x_ptp_settime64(struct ptp_clock_info *ptp, const struct timespec64 *ts);
int lan865x_ptp_enable(struct ptp_clock_info *ptp, struct ptp_clock_request *req, int on);

/* Debug Interface */
int lan865x_ptp_debugfs_init(struct lan865x_ptp_adapter *ptp_adapter);
void lan865x_ptp_debugfs_remove(struct lan865x_ptp_adapter *ptp_adapter);

/* Register Access Helper Functions */
int lan865x_ptp_read_register(struct lan865x_priv *adapter, u32 address, u32 *value);
int lan865x_ptp_write_register(struct lan865x_priv *adapter, u32 address, u32 value);
int lan865x_ptp_modify_register(struct lan865x_priv *adapter, u32 address, u32 mask, u32 value);

/* Utility Functions */


void lan865x_ptp_clock_time_to_timespec64(const struct lan865x_ptp_clock_time *ptp_time, 
                                          struct timespec64 *ts);
void lan865x_timespec64_to_ptp_clock_time(const struct timespec64 *ts, 
                                          struct lan865x_ptp_clock_time *ptp_time);

/* Hardware Detection */
bool lan865x_ptp_hardware_detect(struct lan865x_priv *adapter);
int lan865x_ptp_hardware_validate(struct lan865x_priv *adapter);

#endif /* _LAN865X_PTP_H */