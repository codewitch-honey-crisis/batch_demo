#include <Arduino.h>
#include <tft_io.hpp>
#include <ili9341.hpp>
#include <gfx_cpp14.hpp>
// the font
#include "DEFTONE.hpp"
using namespace arduino;
using namespace gfx;

#define LCD_HOST    VSPI
#ifdef ESP_WROVER_KIT // don't change these
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5
#define BCKL_HIGH false
#else // change these to your setup. below is fastest
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define PIN_NUM_DC   2
#define PIN_NUM_RST  4
#define PIN_NUM_BCKL 15
#define BCKL_HIGH true
#endif
// declare the bus for the driver
// must use tft_spi_ex in order to
// enable DMA for async operations
using bus_t = tft_spi_ex<VSPI,
                        PIN_NUM_CS,
                        PIN_NUM_MOSI,
                        PIN_NUM_MISO,
                        PIN_NUM_CLK,
                        SPI_MODE0,
                        false,
                        320*240*2+8,
                        2>;
// declare the driver
using lcd_t = ili9341<PIN_NUM_DC,
                      PIN_NUM_RST,
                      PIN_NUM_BCKL,
                      bus_t,
                      1,
                      BCKL_HIGH,
                      400, // 40MHz writes
                      200>; // 20MHz reads

// easy access to the color enum
using color_t = color<typename lcd_t::pixel_type>;

// app settings
const bool gradient = false;
const bool async = true;
const char* text = "hello world!";
const uint16_t text_height = 75;
const open_font& text_font = DEFTONE_ttf;

// globals
lcd_t lcd;
float hue;
srect16 text_rect;
float text_scale;

void setup() {
  Serial.begin(115200);
  // premeasure the text and center it
  // so we don't have to every time
  text_scale = text_font.scale(text_height);
  text_rect = text_font.measure_text(ssize16::max(),
                                    spoint16::zero(),
                                    text,
                                    text_scale).
                                      bounds().
                                      center((srect16)lcd.
                                                      bounds());
}

void loop() {
  // current background color
  hsv_pixel<24> px(true,hue,1,1);
  
  // use batching here
  auto ba = (async)?draw::batch_async(lcd,lcd.bounds()):
                  draw::batch(lcd,lcd.bounds());
  
  if(gradient) {
    // draw a gradient
    for(int y = 0;y<lcd.dimensions().height;++y) {
      px.template channelr<channel_name::S>(((double)y)/lcd.bounds().y2);
      for(int x = 0;x<lcd.dimensions().width;++x) {
        px.template channelr<channel_name::V>(((double)x)/lcd.bounds().x2);
        ba.write(px);
      }
    }
  } else {
    // draw a checkerboard pattern
    for(int y = 0;y<lcd.dimensions().height;y+=16) {
      for(int yy=0;yy<16;++yy) {
        for(int x = 0;x<lcd.dimensions().width;x+=16) {
          for(int xx=0;xx<16;++xx) {
            if (0 != ((x + y) % 32)) {
              ba.write(px);
            } else {
              ba.write(color_t::white);
            }
          }
        }
      }
    }
  }
  // commit what we wrote
  ba.commit();
  // offset the hue by half the total hue range
  float hue2 = hue+.5;
  if(hue2>1.0) {
    hue2-=1.0;
  }
  px=hsv_pixel<24>(true,hue2,1,1);
  // convert the HSV pixel to RGBA because HSV doesn't antialias
  // and so we can alpha blend
  rgba_pixel<32> px2,px3;
  convert(px,&px2);
  // set the alpha channel tp 75%
  px2.channelr<channel_name::A>(.75);
  // draw the text
  draw::text(lcd,text_rect,spoint16::zero(),text,text_font,text_scale,px2);
  // increment the hue
  hue+=.1;
  if(hue>1.0) {
    hue = 0.0;
  }
  
}