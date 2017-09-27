#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>


/*
* #include "../../../rpi_ws281x/clk.h"
* #include "../../../rpi_ws281x/gpio.h"
* #include "../../../rpi_ws281x/dma.h"
* #include "../../../rpi_ws281x/pwm.h"
* #include "../../../rpi_ws281x/version.h"
*/
#include "../../../rpi_ws281x/ws2811.h"
#include "../pivumeter.h"



// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10
#define DMA                     6
#define STRIP_TYPE              WS2811_STRIP_GRB		// WS2812/SK6812RGB integrated chip+leds
#define WIDTH                   16
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)


ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

static void ws281x_render(void){
	ws2811_return_t ret;
	if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
	{
		fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
	}
}

static void ws281x_clear_display(void){
    int x;
    for (x = 0; x < WIDTH; x++)
    {
		ledstring.channel[0].leds[x] = 0x0;
	}
	ws281x_render();
}

static int ws281x_init(void){
    ws2811_return_t ret;

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }
    
    ws281x_clear_display();
    atexit(ws281x_clear_display);
    return 0;
}
static int ws281_getrgb (int r, int g, int b, int brightness){
	int pixel=0;
    r=(r*brightness)/256;
    g=(g*brightness)/256;
    b=(b*brightness)/256;
    pixel <<= 8;
    pixel |= r;
    pixel <<= 8;
    pixel |= g;
    pixel <<= 8;
    pixel |= b;
    return pixel;
}

static void ws281x_update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    snd_pcm_scope_ameter_channel_t *l0;
    snd_pcm_scope_ameter_channel_t *l1;
    int peak;
    
    int meter_level = meter_level_l;
    if(meter_level_r > meter_level){meter_level = meter_level_r;}

    int bar = (meter_level / 32767.0f) * WIDTH;
	
	l0 = &level->channels[0];
	l1 = &level->channels[0];
	if (l0->levelchan > l1->levelchan) {
		peak = (l0->levelchan / 32767.0f) * WIDTH;
	} else {
		peak = (l1->levelchan / 32767.0f) * WIDTH;
	}	
    if(bar < 0) {bar = 0;}
    if(bar > WIDTH) {bar = WIDTH;}

    int led;
    int value;
    for(led = 0; led < WIDTH; led++){
		int percent=(led*100)/WIDTH;
        if (led == peak) {
			value=ws281_getrgb(0,0,255,level->led_brightness);
        } else if (led > bar) {
			value=0;
		} else {
			if (percent > 70) {
			value=ws281_getrgb(255,0,0,level->led_brightness);
			} else if (percent > 30) {
			value=ws281_getrgb(244,122,0,level->led_brightness);
			} else {
			value=ws281_getrgb(0,255,0,level->led_brightness);
			}
		}
        ledstring.channel[0].leds[led] = value;
    }
    ws281x_render();
}

device ws281x(){
    struct device _ws281x;
    _ws281x.init = &ws281x_init;
    _ws281x.update = &ws281x_update;
    return _ws281x;
}
