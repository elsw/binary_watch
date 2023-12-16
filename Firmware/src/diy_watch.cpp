#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "multiplex_lcd.pio.h"
#include "diy_watch/multiplex_lcd_drv.h"
#include "diy_watch/RTClib.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
const uint RTC_1HZ_PIN = 18;

const uint LCD_BASE_PIN = 0;
const uint LCD_SM = 1; //LCD State Machine
const uint TARGET_LCD_CYCLE_HZ = 250; //4ms per cycle, 4 cycles per refresh

MultiplexLCDDriver::Ptr lcd;
unsigned count = 0;

void pio_irh() 
{
  lcd->FIFOEmpty();
  pio0_hw->irq = 1;

/*if (pio0_hw->irq & 1) {
    

  } else if (pio0_hw->irq & 2) {
    pio0_hw->irq = 2;

    // PIO0 IRQ1 fired
  }*/
}

void gpio_callback(uint gpio, uint32_t events) 
{
    if(gpio==RTC_1HZ_PIN)
    {
        //Increment time
        gpio_put(LED_PIN,!gpio_get(LED_PIN));
        count++;
        if(count > 9)
          count = 0;
        uint8_t digits[6];
        for(unsigned i = 0 ; i < 6 ; i++)
        {
          digits[i] = count;
        }
        lcd->UpdateOutput(digits);
    }
}

int main()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //Init LCD PIO Driver
    PIO pio = pio0;
    //Load program into memory
    uint offset = pio_add_program(pio, &multiplex_lcd_drv_program);
    //Claim state machine
    pio_sm_claim(pio, LCD_SM);
    //Initialise driver
    lcd = std::make_shared<MultiplexLCDDriver>(pio,LCD_SM,offset,LCD_BASE_PIN,TARGET_LCD_CYCLE_HZ);

    gpio_set_irq_enabled_with_callback(RTC_1HZ_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    // Enable IRQ0 on PIO0
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_irh);
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;

    //sm_config_set_mov_status (&c, STATUS_TX_LESSTHAN , 1 )

    uint8_t digits[6];
    for(unsigned i = 0 ; i < 6 ; i++)
    {
      digits[i] = count;
    }
    lcd->UpdateOutput(digits);

    //Initialize I2C port at 400 kHz
    const uint sda_pin = 16;
    const uint scl_pin = 17;
    i2c_inst_t *i2c = i2c0;
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    i2c_init(i2c, 400 * 1000);

    //Enable IRQ from LCD PIO code
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_sm_put(pio0,LCD_SM,0x00000000);

    RTC_DS3231 rtc;
    rtc.begin(i2c);
    rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_SquareWave1Hz);

    while (true) 
    {
    }
}