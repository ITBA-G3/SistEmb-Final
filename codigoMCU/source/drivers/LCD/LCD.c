/***************************************************************************//**
  @file     LCD.c
  @brief    Function definitions for LCD display manipulation
  @author   Grupo 3 - Jose Iván Hertter, Ezequiel Diaz Guzmán
 ******************************************************************************/

#include "LCD.h"

static volatile uint32_t TICKS_DELAY_MS = 0;

static void delay_cb(void);
static void delay_ms(uint32_t ms);

typedef struct {
    char* text[2];
    uint8_t size_text[2];
} display_t;

display_t LCD = {{NULL, NULL}, {0, 0}};

void init_LCD (void)
{
    gpioMode(PORTNUM2PIN(PE,24), OUTPUT); 

    init_I2C();
    PIT_Init(PIT_3, PIT_TIME(1000)); // 1ms tick
    PIT_SetCallback(delay_cb, PIT_3);
    delay_ms(40);

    uint8_t payload = (uint8_t)(1<<3);
    write_I2C(PCF8574T_SLAVE_ADDR, &payload, 1);

    delay_ms(5);
    // Dummy bytes
    write_nibble(0b0011, 0, 0);
    delay_ms(5);
    write_nibble(0b0011, 0, 0);
    delay_ms(3);
    write_nibble(0b0011, 0, 0);
    delay_ms(3);
    // set interface to 4-bit
    write_nibble(0b0010, 0, 0);
    delay_ms(3);
    // specify display lines & char font
    write_byte(0b00101000, 0, 0);
    delay_ms(3);
    // display off
    write_byte(0b00001000, 0, 0);
    delay_ms(3);
    // display clear
    write_byte(0b00000001, 0, 0);
    delay_ms(6);
    // Entry mode set
    write_byte(0b00000110, 0, 0);
    delay_ms(3);
    // display on
    write_byte(0b00001100, 0, 0);
    delay_ms(3);

    // return_home();
    clear_LCD();
    delay_ms(3);  
}

/**
 * @brief         The user has the responsability to send a valid str ending on '\n' 
 *
 * @param str     message to display
 * @param line    LCD display has 2 lines. If 0 it will display on the upper line, if 1, on the bottom line.
**/
void write_LCD(char* str, uint8_t line)
{
    static uint8_t size = 0;

    if (line > 1)
        return;

    size = 0;
    while(str[size] != '\0' && size <= 255)
        size++;

    LCD.text[line] = str;
    LCD.size_text[line] = size;
    clear_line(line);

    set_cursor_line(line);
    for (int i=0; i<LCD.size_text[line]; i++)
        if(i<DISPLAY_WIDTH)
            write_byte(LCD.text[line][i], 0, 1);

    return_home();
    delay_ms(50);
}

void shift_LCD(uint8_t line)
{
    if (line>1 || line<0)
        return;

    if (LCD.size_text[line] < DISPLAY_WIDTH)
    {
        set_cursor_line(line);
        for (int i=0; i<LCD.size_text[line]; i++)
            write_byte(LCD.text[line][i], 0, 1);
        return;
    }

    for (int i=0; i<=(LCD.size_text[line]); i++)
    {
        set_cursor_line(line);
        // Print a line on screen
        for (int j=0; j<DISPLAY_WIDTH; j++)
        {
            if (i+j < LCD.size_text[line])
                write_byte(LCD.text[line][j+i], 0, 1); 
            else
                write_byte(' ', 0, 1);   // Empty space
        }
        delay_ms(600);
        clear_line(line);
    }

    for (int i=0; i<DISPLAY_WIDTH; i++)
    {
        set_cursor_line(line);
        for (int j=0; j<DISPLAY_WIDTH; j++)
        {
            if (j < DISPLAY_WIDTH - i - 1 )
                write_byte(' ', 0, 1);
            else 
                write_byte(LCD.text[line][j - DISPLAY_WIDTH + i + 1], 0, 1);
        }
        delay_ms(600);
        clear_line(line);
    }
}


/************************
 *   Internal functions
 ***********************/

void return_home()
{
    static uint8_t data_byte = 0;

    data_byte = 0b00000010;
    write_byte(data_byte, 0, 0);
}

void set_cursor_line(uint8_t line)
{
    uint8_t data_byte = 0b10000000;

    if (line != 0)
        data_byte |= 0x40;

    write_byte(data_byte, 0, 0);
}

void write_byte(uint8_t data, uint8_t RW, uint8_t RS)
{
    static uint8_t hi, lo, ctrl;

    hi = data & 0xF0;
    lo = data << 4;
    ctrl = COMMAND_NIBBLE(RW,RS);

    uint8_t payload[4]={hi | ctrl | EN_MASK,
                        hi | ctrl,
                        lo | ctrl | EN_MASK,
                        lo | ctrl};

    write_I2C(PCF8574T_SLAVE_ADDR, payload, 4);
}

void write_nibble(uint8_t nibble, uint8_t RW, uint8_t RS)
{
    uint8_t ctrl = COMMAND_NIBBLE(RW, RS), data = nibble << 4;
    
    uint8_t payload[2]={data | ctrl | EN_MASK,
                        data | ctrl};

    write_I2C(PCF8574T_SLAVE_ADDR, payload, 2);
}


/*********************************
 *    COMMANDS LCD
* *******************************/

void clear_LCD()
{
    const uint8_t data=0b00000001;
    write_byte(data, 0, 0);

    // delay_ms(4);    
}

void clear_line(uint8_t line)
{
    set_cursor_line(line);
    for(int i=0; i<DISPLAY_WIDTH; i++)
        write_byte(' ', 0, 1);
    set_cursor_line(line);
}

static void delay_cb(void)
{
    TICKS_DELAY_MS++;
    gpioToggle(PORTNUM2PIN(PE,24));
}

static void delay_ms(uint32_t ms)
{
    TICKS_DELAY_MS = 0;
    while (TICKS_DELAY_MS < ms);

}

