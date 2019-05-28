// Filename		: hello_world.c
// Programmer	: Gabriel Stewart
// Version Date	: May 28th, 2019
// Description	: This file contains the source code for a binary counter and an accompanying 7-Segment display.
//				  It demonstrates the configuration and usage of two SN74HC595 8-Bit shift registers, one dual
//				  7-Segment display, and one 10 LED light strip.

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include <string.h>
#include <math.h>

//*****************************************************************************
// Insert compiler version at compile time.
//*****************************************************************************
#define STRINGIZE_VAL(n)                    STRINGIZE_VAL2(n)
#define STRINGIZE_VAL2(n)                   #n

#ifdef __GNUC__
#define COMPILER_VERSION                    ("GCC " __VERSION__)
#elif defined(__ARMCC_VERSION)
#define COMPILER_VERSION                    ("ARMCC " STRINGIZE_VAL(__ARMCC_VERSION))
#elif defined(__KEIL__)
#define COMPILER_VERSION                    "KEIL_CARM " STRINGIZE_VAL(__CA__)
#elif defined(__IAR_SYSTEMS_ICC__)
#define COMPILER_VERSION                    __VERSION__
#else
#define COMPILER_VERSION                    "Compiler unknown"
#endif

//*****************************************************************************
//
// UART configuration settings.
//
//*****************************************************************************
am_hal_uart_config_t g_sUartConfig =
{
    .ui32BaudRate = 115200,
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
    .bTwoStopBits = false,
    .ui32Parity   = AM_HAL_UART_PARITY_NONE,
    .ui32FlowCtrl = AM_HAL_UART_FLOW_CTRL_NONE,
};


// Prototypes
void clearLED();
void count4Bit();
void displayInt(int number, int place);
void shiftOut(uint8_t value, int reg);

// Global variables
static int delay = 100;

// GPIO Pin Definitions
#define GPIO_SER_BIN		25
#define GPIO_OE_BIN			26
#define GPIO_CLK_BIN		28
#define GPIO_CLR_BIN		33

#define GPIO_SER_SEG		32
#define GPIO_OE_SEG			31
#define GPIO_CLK_SEG		30
#define GPIO_CLR_SEG		29

#define BINARY_REG		0
#define SEGMENT_REG		1

#define ONES			8
#define TENS			9
#define HUNDREDS1		40
#define HUNDREDS2		39


//*****************************************************************************
//
// Initialize the UART
//
//*****************************************************************************
void
uart_init(uint32_t ui32Module)
{
    //
    // Make sure the UART RX and TX pins are enabled.
    //
    am_bsp_pin_enable(COM_UART_TX);
    am_bsp_pin_enable(COM_UART_RX);

    //
    // Power on the selected UART
    //
    am_hal_uart_pwrctrl_enable(ui32Module);

    //
    // Start the UART interface, apply the desired configuration settings, and
    // enable the FIFOs.
    //
    am_hal_uart_clock_enable(ui32Module);

    //
    // Disable the UART before configuring it.
    //
    am_hal_uart_disable(ui32Module);

    //
    // Configure the UART.
    //
    am_hal_uart_config(ui32Module, &g_sUartConfig);

    //
    // Enable the UART FIFO.
    //
    am_hal_uart_fifo_config(ui32Module, AM_HAL_UART_TX_FIFO_1_2 | AM_HAL_UART_RX_FIFO_1_2);

    //
    // Enable the UART.
    //
    am_hal_uart_enable(ui32Module);
}

//*****************************************************************************
//
// Disable the UART
//
//*****************************************************************************
void
uart_disable(uint32_t ui32Module)
{
      //
      // Clear all interrupts before sleeping as having a pending UART interrupt
      // burns power.
      //
      am_hal_uart_int_clear(ui32Module, 0xFFFFFFFF);

      //
      // Disable the UART.
      //
      am_hal_uart_disable(ui32Module);

      //
      // Disable the UART pins.
      //
      am_bsp_pin_disable(COM_UART_TX);
      am_bsp_pin_disable(COM_UART_RX);

      //
      // Disable the UART clock.
      //
      am_hal_uart_clock_disable(ui32Module);
}

//*****************************************************************************
//
// Transmit delay waits for busy bit to clear to allow
// for a transmission to fully complete before proceeding.
//
//*****************************************************************************
void
uart_transmit_delay(uint32_t ui32Module)
{
  //
  // Wait until busy bit clears to make sure UART fully transmitted last byte
  //
  while ( am_hal_uart_flags_get(ui32Module) & AM_HAL_UART_FR_BUSY );
}


// Function		: main
// Description	: Main completes configuration of the board and begins running the counter loop
// Parameters	: None
// Returns		: Int - Contains return status
int
main(void)
{
    // Set the clock frequency.
    am_hal_clkgen_sysclk_select(AM_HAL_CLKGEN_SYSCLK_MAX);

    // Set the default cache configuration
    am_hal_cachectrl_enable(&am_hal_cachectrl_defaults);

    // Configure the board for low power operation.
    am_bsp_low_power_init();

    // Initialize the SWO GPIO pin
    am_bsp_pin_enable(ITM_SWO);

    // Enable the ITM.
    am_hal_itm_enable();

    // Select a UART module to use.
    uint32_t ui32Module = AM_BSP_UART_PRINT_INST;

    // Initialize the printf interface for UART output.
    am_util_stdio_printf_init((am_util_stdio_print_char_t)am_bsp_uart_string_print);

    // Configure and enable the UART.
    uart_init(ui32Module);

    // Disable the UART and interrupts
//    uart_disable(ui32Module);


    // Configure Binary LED Counter GPIO Pins
    am_hal_gpio_pin_config(GPIO_SER_BIN, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_OE_BIN, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_CLK_BIN, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_CLR_BIN, AM_HAL_PIN_OUTPUT);
	am_hal_gpio_out_bit_clear(GPIO_SER_BIN);
	am_hal_gpio_out_bit_clear(GPIO_OE_BIN);
	am_hal_gpio_out_bit_clear(GPIO_CLK_BIN);
	am_hal_gpio_out_bit_clear(GPIO_CLR_BIN);
    am_hal_gpio_out_bit_set(GPIO_OE_BIN);
    am_hal_gpio_out_bit_set(GPIO_CLR_BIN);

    // Configure 7-Segment Display GPIO Pins
    am_hal_gpio_pin_config(GPIO_SER_SEG, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_OE_SEG, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_CLK_SEG, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(GPIO_CLR_SEG, AM_HAL_PIN_OUTPUT);
	am_hal_gpio_out_bit_clear(GPIO_SER_SEG);
	am_hal_gpio_out_bit_clear(GPIO_OE_SEG);
	am_hal_gpio_out_bit_clear(GPIO_CLK_SEG);
	am_hal_gpio_out_bit_clear(GPIO_CLR_SEG);
    am_hal_gpio_out_bit_set(GPIO_OE_SEG);
    am_hal_gpio_out_bit_set(GPIO_CLR_SEG);

    // Configure Additional pins used for control
    am_hal_gpio_pin_config(ONES, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(TENS, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(HUNDREDS1, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_pin_config(HUNDREDS2, AM_HAL_PIN_OUTPUT);
	am_hal_gpio_out_bit_clear(ONES);
	am_hal_gpio_out_bit_clear(TENS);
	am_hal_gpio_out_bit_clear(HUNDREDS1);
	am_hal_gpio_out_bit_clear(HUNDREDS2);

    // Configure buttons
    am_hal_gpio_pin_config(AM_BSP_GPIO_BUTTON0, AM_HAL_PIN_INPUT);

    // Enable interrupt for button 0
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_BUTTON0));
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);

    // Global Variable Declaration
    uint8_t integer = 0;

	// Clear output
	am_hal_gpio_out_bit_clear(GPIO_CLR_BIN);
	am_hal_gpio_out_bit_clear(GPIO_CLK_BIN);
	am_hal_gpio_out_bit_set(GPIO_CLK_BIN);
	am_hal_gpio_out_bit_set(GPIO_CLR_BIN);

	am_hal_gpio_out_bit_clear(GPIO_CLR_SEG);
	am_hal_gpio_out_bit_clear(GPIO_CLK_SEG);
	am_hal_gpio_out_bit_set(GPIO_CLK_SEG);
	am_hal_gpio_out_bit_set(GPIO_CLR_SEG);

	// Loop Forever
    while (1)
    {
    	// Load register
    	shiftOut(integer, BINARY_REG);

    	// Turn on binary LEDs
    	am_hal_gpio_out_bit_clear(GPIO_OE_BIN);

    	// Multiplex display of integer
    	for (int x = 0; x < delay; x++)
    	{
    		// Toggle ONES anode
    	    am_hal_gpio_out_bit_set(ONES);
    	    am_hal_gpio_out_bit_clear(TENS);

    	    // Shift out ones value
        	displayInt(integer, 0);

    		// Toggle display
    		am_hal_gpio_out_bit_clear(GPIO_OE_SEG);
    		am_util_delay_ms(1);
    		am_hal_gpio_out_bit_set(GPIO_OE_SEG);

    		// Toggle TENS anode
    	    am_hal_gpio_out_bit_set(TENS);
    	    am_hal_gpio_out_bit_clear(ONES);

    	    // Shift out tens value
        	displayInt(integer, 1);

    		// Toggle display
    		am_hal_gpio_out_bit_clear(GPIO_OE_SEG);
    		am_util_delay_ms(1);
    		am_hal_gpio_out_bit_set(GPIO_OE_SEG);
    	}

		// Turn off binary LEDs
		am_hal_gpio_out_bit_set(GPIO_OE_BIN);

		// Display current integer
		am_util_stdio_printf("%3.0u \r", integer);

		integer++;
    }
}


// Function		: shiftOut
// Description	: This function shifts a value passed to it into the shift register
// Parameters	: uint8_t value - Integer to be passed to register
//				  int reg - Shift register being shifted to
// Returns		: None
void shiftOut(uint8_t value, int reg)
{
	// Variables for register addressing
	int SER, CLK, CLR;

	switch (reg)
	{
	case BINARY_REG:
		SER = GPIO_SER_BIN;
		CLK = GPIO_CLK_BIN;
		CLR = GPIO_CLR_BIN;
		break;
	case SEGMENT_REG:
		SER = GPIO_SER_SEG;
		CLK = GPIO_CLK_SEG;
		CLR = GPIO_CLR_SEG;
		break;
	}

	// Clear Register
	am_hal_gpio_out_bit_clear(CLR);
	am_hal_gpio_out_bit_set(CLR);

	// Shift out to register
	for (int x = 0; x <= 8; x++)
	{
    	// Set SER low or high based on current binary value
		if (value & (1<<x)) {
			am_hal_gpio_out_bit_set(SER);
		}
		else
			am_hal_gpio_out_bit_clear(SER);

    	// Cycle clock
    	am_hal_gpio_out_bit_set(CLK);
    	am_hal_gpio_out_bit_clear(CLK);
	}
}


// Function		: am_gpio_isr
// Description	: This handler is called when the button is pushed
// Parameters	: void
// Returns		: None
void am_gpio_isr(void)
{
    // Clear the GPIO Interrupt (write to clear).
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_BUTTON0));

    // Perform interrupt functionality here
    delay += 100;
}


// Function		: displayInt
// Description	: This Function displays the int passed to it on an external display
// Parameters	: int number - Value to represent on display
//				  int place - Display we are using
// Returns		: None
// Notes		:
void displayInt(int number, int place)
{
	// Variable declaration
	int multiple = 0;

	uint8_t array[10] = { 0x77, 0x11, 0x6B, 0x3B, 0x1D, 0x3E, 0x7E, 0x13, 0x7F, 0x3F };

	// Clear hundreds LEDs
	am_hal_gpio_out_bit_clear(HUNDREDS1);
	am_hal_gpio_out_bit_clear(HUNDREDS2);

	// If we are above one/two hundred, remove one hundred
	if (number >= 200) {
		number -= 100;
		am_hal_gpio_out_bit_set(HUNDREDS1);
	}
	if (number >= 100) {
		number -= 100;
		am_hal_gpio_out_bit_set(HUNDREDS2);
	}

	// Determine if we are displaying ones or tens place
	if (place == 0)
		multiple = number % 10;
	else
		multiple = number / 10;

	// Depending on number to display, clear particular bits
	shiftOut(~array[multiple], 1);
}
