/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>

#define     KEY_UP      (0x01)
#define     KEY_DOWN    (0x02)
#define     MAX_RANGE   (7)
#define     MAX_POWER   (10)
#define     CPU_FREQ    (80e6)

// Interrupt handler
CYBIT int_Capture_flag = 0;

CY_ISR(int_Capture_isr) {
	int_Capture_flag = 1;
}

// Decimal number generation
CYCODE const uint32 power10[MAX_POWER+1] = {
1UL,
1UL,
10UL,
100UL,
1000UL,
10000UL,
100000UL,
1000000UL,
10000000UL,
100000000UL,
1000000000UL,
};

void LCD_PrintDecUint32(uint32 d, uint8 digits) {
	uint8  m;
	uint8  v;
    static char numbuf[32];
    
    for (m = MAX_POWER; m > 0; m--) {
	    for (v = 0; d >= power10[m]; ) {
	  	  d -= power10[m];
		  v++;
	    }
	    if (m <= digits) {
            numbuf[digits-m] = '0' + v;
	    }
	}
    numbuf[digits] = 0;
    LCD_PrintString(numbuf);
}

// Parameters for frequency range
CYCODE const struct {
    double  divisor;
    uint8   mux;
} params[MAX_RANGE+1] = {
    {1e2,  0x0},    // x100 prescaler
    {1e3,  0x1},    // x1k prescaler
    {1e4,  0x2},    // x10k prescaler
    {1e5,  0x3},    // x100k prescaler
    {1e6,  0x4},    // x1M prescaler
    {1e7,  0x5},    // x10M prescaler
    {1e8,  0x6},    // x100M prescaler
    {1e9,  0x7},    // x1G prescaler
};

// Frequency range control
uint8   range;
double  resolution;
double  cpks;  // cycles per kilo-second
uint8   required;

void setRange(uint8 p_range) {
    range = p_range;
    int_Capture_Disable();
    CR1_Write(params[range].mux);
    resolution = 1e0 / (BCLK__BUS_CLK__HZ * params[range].divisor);
    cpks = CPU_FREQ * 1e3;
    required = 2;
    LCD_ClearDisplay();
    LCD_Position(0,15);
    LCD_PutChar('0' + range);
	LCD_Position(0,10);
    LCD_PrintString("Hz");
	LCD_Position(1,10);
    LCD_PrintString("mc");
    Timer_ClearFIFO();
    int_Capture_ClearPending();
    int_Capture_Enable();
}

// Frequency and Period calculation
uint32 capture_last = 0;
uint32 capture_now;
uint32 capture_period;
double period;
uint32 freq;
uint32 cycles;
uint32 pico_period;

int main()
{
    uint8  key;
    
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
	int_Capture_StartEx(int_Capture_isr);
	LCD_Init();
	Timer_Start();
    setRange(0);

    for(;;) {
        /* Place your application code here. */
		if (int_Capture_flag) {
			int_Capture_flag = 0;
			capture_now = Timer_ReadCapture();
			capture_period = capture_last - capture_now;
			capture_last = capture_now;
            if (required == 0) {
    			period = (double)capture_period * resolution;
    			freq = (uint32)(1e0 / period);
//                pico_period = (uint32)(period * 1e12);
                cycles = (uint32)(period * cpks);
    			LCD_Position(0,0);
    			LCD_PrintDecUint32(freq, 10);
    			LCD_Position(1,0);
    			LCD_PrintDecUint32(cycles, 10);
            } else if (required == 1) {
                LCD_Position(1,15);
                LCD_PutChar('*');
                required = 0;
            } else {
                LCD_Position(1,14);
                LCD_PutChar('*');
                required = 1;
            }
		}
        key = SR1_Read();
        if (key == KEY_UP) {
            if (range < MAX_RANGE) {
                setRange(range + 1);
            }
            while (SR1_Read());
        }
        if (key == KEY_DOWN) {
            if (range > 0) {
                setRange(range - 1);
            }
            while (SR1_Read());
        }
    }
}

/* [] END OF FILE */
