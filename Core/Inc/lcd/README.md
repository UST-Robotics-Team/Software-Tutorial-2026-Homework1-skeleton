# Controller LCD library

This library drives an SPI TFT from the STM32F103 controller. The default
configuration is a 240 x 320 ST7789 using RGB666 transfers. A 128 x 160
ST7735 configuration is also available through `TFT_SCREEN_MODE` in `lcd.h`.

## Controller wiring

| Signal | Controller pin |
| --- | --- |
| SCK | PA5 / SPI1 |
| MOSI | PA7 / SPI1 |
| Reset | PB10 / TFT_RES |
| Data/command | PB11 / TFT_DC |
| Chip select | PB12 / TFT_CS |
| Backlight | PB13 / TFT_BL |

CubeMX generates the GPIO pin definitions and the SPI handle. SPI1 uses mode
0, 8-bit MSB-first transfers with software NSS. The current controller uses a
prescaler of 8, giving a 9 MHz SPI clock. Initialize GPIO and SPI before the
display:

```c
MX_GPIO_Init();
MX_SPI1_Init();
tft_init(PIN_ON_TOP, BLACK, WHITE, CYAN, DARK_GREY);
```

Text is buffered. Populate the complete desired text frame, then call
`tft_update()`:

```c
tft_printc(0, 0, "RDC CONTROLLER");
tft_prints(0, 1, "Value: %u", (unsigned)value);
tft_update(20U);
```

`tft_printc()` prints a fixed string. `tft_prints()` supports printf-style
formatting. A successful update swaps the text buffers and clears the next
frame, so the application must repopulate all desired text before subsequent
updates. Pixel, rectangle, image, and graphics functions draw immediately and
do not require `tft_update()`.

LCD transfers currently use blocking `HAL_SPI_Transmit()`. Internal names
containing `dma` are retained for compatibility but do not require SPI DMA.
The ADC uses DMA independently of the LCD.

## Verification

Build the firmware with:

```sh
cmake --build build/Debug
```

From the repository parent directory, run the host LCD checks with:

```sh
cc -std=c11 -Wall -Wextra -fsanitize=address,undefined -g \
  -Itests/lcd/stubs -IRDC_controller/Core/Inc \
  RDC_controller/Core/Src/lcd.c RDC_controller/Core/Src/lcd_graphics.c \
  tests/lcd/test_lcd.c -o /tmp/rdc-lcd-test-run
/tmp/rdc-lcd-test-run
```

The host checks validate initialization, text formatting, clipping, and pixel
conversion. Screen wiring, panel orientation, and electrical timing still
require verification on the physical controller.
