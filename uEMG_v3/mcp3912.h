#include "nrf.h"
#include "nrf_gpio.h"
#include <stdint.h>

#define MCP3912_CH0     0x00
#define MCP3912_CH1     0x01
#define MCP3912_CH2     0x02
#define MCP3912_CH3     0x03
#define MCP3912_MOD     0x08
#define MCP3912_PHASE   0x0A
#define MCP3912_GAIN    0x0B
#define MCP3912_STATCOM 0x0C
#define MCP3912_CONFIG0 0x0D
#define MCP3912_CONFIG1 0x0E
#define MCP3912_OFF0    0x0F
#define MCP3912_GC0     0x10
#define MCP3912_OFF1    0x11
#define MCP3912_GC1     0x12
#define MCP3912_OFF2    0x13
#define MCP3912_GC2     0x14
#define MCP3912_OFF3    0x15
#define MCP3912_GC3     0x16
#define MCP3912_LOCK    0x1F


/**
 * @brief PGA Gain Configuration Register
 *
 * bit 23-12
 *      Unimplemented: Read as 0
 * bit 11-0
 *      PGA_CHn<2:0>: PGA Setting for Channel n
 *          111 = Reserved (Gain = 1)
 *          110 = Reserved (Gain = 1)
 *          101 = Gain is 32
 *          100 = Gain is 16
 *          011 = Gain is 8
 *          010 = Gain is 4
 *          001 = Gain is 2
 *          000 = Gain is 1 (DEFAULT)
 *
 */

/**
 * @brief PGA Settings for the channels
 */

typedef struct {

    union {
        uint32_t wholeRegister;

        struct {
            unsigned PGA_CH0 : 3;
            unsigned PGA_CH1 : 3;
            unsigned PGA_CH2 : 3;
            unsigned PGA_CH3 : 3;
            unsigned : 12;
        };
    };
} MCP391x_GAIN_REG;


/**
 * @brief CONFIG0 Register Bit-Field
 *
 * bit 23
 *      EN_OFFCAL: Enables the 24-bit digital offset error calibration.
 *          1 = Enabled. This mode does not add any group delay to the ADC data.
 *          0 = Disabled (DEFAULT)
 * bit 22
 *      EN_GAINCAL: Enables or disables the 24-bit digital gain error
 *                  calibration on all channels
 *          1 = Enabled. This mode adds a group delay on all channels of 24
 *              DMCLK periods. All data ready pulses are delayed by 24 DMCLK
 *              lock periods, compared to the mode with EN_GAINCAL = 0.
 *          0 = Disabled (DEFAULT)
 * bit 21-20
 *      DITHER<1:0>: Control for dithering circuit for idle tone?s cancellation
 *                  and improved THD on all channels
 *          11 = Dithering ON, Strength = Maximum (DEFAULT)
 *          10 = Dithering ON, Strength = Medium
 *          01 = Dithering ON, Strength = Minimum
 *          00 = Dithering turned OFF
 * bit 19-18
 *      BOOST<1:0>: Bias Current Selection for all ADCs (impacts achievable
 *                  maximum sampling speed, see Table 5-2 of MCP3912 Datasheet)
 *          11 = All channels have current x 2
 *          10 = All channels have current x 1 (Default)
 *          01 = All channels have current x 0.66
 *          00 = All channels have current x 0.5
 * bit 17-16
 *      PRE<1:0>: Analog Master Clock (AMCLK) Prescaler Value
 *          11 = AMCLK = MCLK/8
 *          10 = AMCLK = MCLK/4
 *          01 = AMCLK = MCLK/2
 *          00 = AMCLK = MCLK (Default)
 * bit 15-13
 *      OSR<2:0>: Oversampling Ratio for delta sigma A/D Conversion
 *              (ALL CHANNELS, fD/fS)
 *          111 = 4096 (fd = 244 sps for MCLK = 4 MHz, fs = AMCLK = 1 MHz)
 *          110 = 2048 (fd = 488 sps for MCLK = 4 MHz, fs = AMCLK = 1 MHz)
 *          101 = 1024 (fd = 976 sps for MCLK = 4 MHz, fs = AMCLK = 1 MHz)
 *          100 = 512 (fd = 1.953 ksps for MCLK = 4 MHz, fs = AMCLK = 1MHz)
 *          011 = 256 (fd = 3.90625 ksps for MCLK = 4 MHz, fs = AMCLK = 1 MHz) D
 *          010 = 128 (fd = 7.8125 ksps for MCLK = 4 MHz, fs = AMCLK = 1 MHz)
 *          001 = 64 (fd = 15.625 ksps for MCLK = 4 MHz, fs = AMCLK = 1MHz)
 *          000 = 32 (fd = 31.25 ksps for MCLK = 4 MHz, fs = AMCLK = 1MHz)
 * bit 12-8
 *      Unimplemented: Read as 0
 * bit 7-0
 *      VREFCAL<7:0>: Internal Voltage Temperature coefficient VREFCAL<7:0>
 *                    value. (See Section 5.6.3 ?Temperature compensation
 *                    (VREFCAL<7:0>)? in MCP documentation for complete
 *                    description).
 *
 */
typedef struct {

    union {
        uint32_t wholeRegister;

        struct {
            unsigned VREFCAL : 8;
            unsigned : 5;
            unsigned OSR : 3;
            unsigned PRE : 2;
            unsigned BOOST : 2;
            unsigned DITHER : 2;
            unsigned EN_GAINCAL : 1;
            unsigned EN_OFFCAL : 1;
            unsigned : 8;
        };
    };
} MCP391x_CONFIG0_REG;


/**
 * @brief Status and Communication Register Bit-Field
 *
 * bit 23-22
 *      READ<1:0>: Address counter increment setting for Read Communication
 *          11 = Address counter auto-increments, and loops on the entire register map
 *          10 = Address counter auto-increments, and loops on register TYPES (DEFAULT)
 *          01 = Address counter auto-increments, and loops on register GROUPS
 *          00 = Address is not incremented, and continually reads the same
 *               single-register address
 * bit 21
 *      WRITE: Address counter increment setting for Write Communication
 *          1 = Address counter auto-increments and loops on writable part of
 *              the register map (DEFAULT)
 *          0 = Address is not incremented, and continually writes to the same
 *              single register address
 * bit 20
 *      DR_HIZ: Data Ready Pin Inactive State Control
 *          1 = The DR pin state is a logic high when data is NOT ready
 *          0 = The DR pin state is high-impedance when data is NOT ready (DEFAULT)
 * bit 19
 *      DR_LINK: Data Ready Link Control
 *          1 = Data Ready link enabled. Only one pulse is generated on the DR
 *              pin for all ADC channels corresponding to the data ready pulse
 *              of the most lagging ADC. (DEFAULT)
 *          0 = Data Ready link disabled. Each ADC produces its own data ready
 *              pulse on the DR pin.
 * bit 18
 *      WIDTH_CRC: Format for CRC-16 on communications
 *          1 = 32-bit (CRC-16 code is followed by zeros). This coding is
 *              compatible with CRC implementation in most 32-bit MCUs
 *              (including PIC32 MCUs).
 *          0 = 16 bit (default)
 * bit 17-16
 *      WIDTH_DATA<1:0>: ADC Data Format Settings for all ADCs
 *          11 = 32-bit with sign extension
 *          10 = 32-bit with zeros padding
 *          01 = 24-bit (default)
 *          00 = 16-bit (with rounding)
 * bit 15
 *      EN_CRCCOM: Enable CRC CRC-16 Checksum on Serial communications
 *          1 = CRC-16 Checksum is provided at the end of each communication
 *              sequence (therefore each communication is longer). The CRC-16
 *              Message is the complete communication sequence (see section
 *              Section 6.9).
 *          0 = Disabled
 * bit 14
 *      EN_INT: Enable for the CRCREG interrupt function
 *          1 = The interrupt flag for the CRCREG checksum verification is
 *              enabled. The Data Ready pin (DR) will become logic low and stays
 *               logic low if a CRCREG checksum error happens. This interrupt is
 *              cleared if the LOCK<7:0> value is made equal to the PASSWORD
 *              value (0xA5).
 *          0 = The interrupt flag for the CRCREG checksum verification is
 *              disabled. The CRCREG<15:0> bits are still calculated properly
 *              and can still be read in this mode. No interrupt is generated
 *              even when a CRCREG checksum error happens. (Default)
 * bit 13-12
 *      Reserved: Should be kept equal to 0 at all times
 * bit 11-4
 *      Unimplemented: Read as 0
 * bit 3-0
 *      DRSTATUS<3:0>: Data ready status bit for each individual ADC channel
 *          DRSTATUS<n> = 1 - Channel CHn data is not ready (DEFAULT)
 *          DRSTATUS<n> = 0 - Channel CHn data is ready. The status bit is set back
 *                    to '1' after reading the STATUSCOM register. The status
 *                    bit is not set back to '1' by the read of the corresponding
 *                    channel ADC data.
 *
 */
 
typedef struct {

    union {
        uint32_t wholeRegister;

        struct {
            unsigned DRSTATUS : 4;
            unsigned : 10;
            unsigned EN_INT : 1;
            unsigned EN_CRCCOM : 1;
            unsigned WIDTH_DATA : 2;
            unsigned WIDTH_CRC : 1;
            unsigned DR_LINK : 1;
            unsigned DR_HIZ : 1;
            unsigned WRITE : 1;
            unsigned READ : 2;
            unsigned : 8;
        };
    };
} MCP391x_STATUSCOM_REG;

/**
 * @brief CONFIG1 Address Register Bit-Field
 *
 * bit 23-20
 *      Unimplemented: Read as 0.
 * bit 19-16
 *      RESET<3:0>: Soft Reset mode setting for each individual ADC
 *          RESET<n> = 1 : Channel CHn in soft reset mode
 *          RESET<n> = 0 : Channel CHn not in soft reset mode
 * bit 15-12
 *      Unimplemented: Read as 0
 * bit 11-8
 *      SHUTDOWN<3:0>: Shutdown Mode setting for each individual ADC
 *          SHUTDOWN<n> = 1 : ADC Channel CHn in Shutdown
 *          SHUTDOWN<n> = 0 : ADC Channel CHn not in Shutdown
 * bit 7
 *      VREFEXT: Internal Voltage Reference selection bit
 *          1 = Internal Voltage Reference Disabled. An external reference
 *              voltage needs to be applied across the REFIN+/- pins. The analog
 *              power consumption (AI_DD) is slightly diminished in this mode
 *              since the internal voltage reference is placed in Shutdown mode.
 *          0 = Internal Reference enabled. For optimal accuracy, the REFIN+/OUT
 *              pin needs proper decoupling capacitors. REFIN- pin should be
 *              connected to A_GND, when in this mode.
 * bit 6
 *      CLKEXT: Internal Clock selection bit
 *          1 = MCLK is generated externally and should be provided on the OSC1
 *          pin. The oscillator is disabled and uses no current (Default)
 *          0 = Crystal oscillator enabled. A crystal must be placed between
 *              OSC1 and OSC2 with proper decoupling capacitors. The digital
 *              power consumption (DI_DD) is increased in this mode due to the
 *              oscillator.
 * bit 5-0
 *      Unimplemented: Read as 0
 *
 */
typedef struct {

    union {
        uint32_t wholeRegister;

        struct {
            unsigned : 6;
            unsigned CLKEXT : 1;
            unsigned VREFEXT : 1;
            unsigned SHUTDOWN : 4;
            unsigned : 4;
            unsigned RESET : 4;
            unsigned : 12;
        };
    };
} MCP391x_CONFIG1_REG;

typedef struct sMCP3912
{
	uint32_t CS;
	int16_t cvalue[4];
}sMCP3912;

sMCP3912 mcp;

void init_mcp3912(uint8_t pin_CIPO, uint8_t pin_COPI, uint8_t pin_SCK, uint8_t pin_CS, uint8_t pin_INT);
uint8_t mcp3912_read();
int *mcp3912_get_data_buf();
void mcp3912_stop();
int mcp3912_is_ok();

void mcp_fft_mode(int onoff);
int mcp_fft_process();
float *mcp_fft_get_spectr();
float *mcp_fft_get_raw();
float sqrt_fast(float x);