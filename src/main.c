/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
#include "img/tela1_v1.h"
#include "img/tela1.h"
#include "img/tela2.h"
#include "img/tela3e4_v1.h"
#include "img/tela5_v1.h"
#include "img/tela6_v1.h"
#include "img/tela6_v2.h"
#include "img/tela7_v1.h"
#include "img/roda.h"
#include "arm_math.h"

// PINO DE RECEBIMENTO PULSO
#define RECEBIMENTO_PIO PIOA
#define RECEBIMENTO_PIO_ID ID_PIOA
#define RECEBIMENTO_PIO_IDX 19
#define RECEBIMENTO_PIO_IDX_MASK (1 << RECEBIMENTO_PIO_IDX)

#define VEL_MAX_KMH 5.0f
#define VEL_MIN_KMH 0.5f
#define RAMP
#define PI 3.14
#define RAIO_RODA 0.29 // 29 centímetros
#define TAMANHO_BUFFER_VELOCIDADE 10

LV_FONT_DECLARE(montserrat_65);
LV_FONT_DECLARE(montserrat_32);
LV_FONT_DECLARE(montserrat_18);
LV_FONT_DECLARE(montserrat_16);
LV_FONT_DECLARE(acceleration_symbols);
LV_FONT_DECLARE(wheel_symbol);
LV_FONT_DECLARE(font_awesome_test);

#define MY_CLOCK_SYMBOL "\xEF\x80\x97"
#define MY_UP_SYMBOL "\xEF\x84\x82"
#define MY_DOWN_SYMBOL "\xEF\x84\x83"
#define MY_WHEEL_SYMBOL "\xEF\x88\x86"

/************************************************************************/
/* STRUCTs                                                              */
/************************************************************************/

typedef struct
{
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

enum screen
{
	TELA2,
	TELA3,
	TELA4,
	TELA5
};


/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX (240)
#define LV_VER_RES_MAX (320)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv; /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

lv_obj_t *img1;
lv_obj_t *img2;
lv_obj_t *velocimeter;
lv_obj_t *kmporh;
lv_obj_t *play;
lv_obj_t *pause;
lv_obj_t *btn_pause_play;
lv_obj_t *label_pause_play;
lv_obj_t *stop;
lv_obj_t *config;
lv_obj_t *home;
lv_obj_t *back;
lv_obj_t *wheel;
lv_obj_t *up;
lv_obj_t *down;
lv_obj_t *clock;
lv_obj_t *distance;
lv_obj_t *speed;
lv_obj_t *timer;
lv_obj_t *roller;
lv_obj_t *odometer;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE (1024 * 6 / sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_SIMULATOR_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_SIMULATOR_STACK_PRIORITY (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTT_Handler(void);
void TC_init(Tc *TC, int ID_TC, int TC_CHANNEL, int freq);

void lv_tela1(void);
void lv_tela2(void);
void lv_tela3(void);
void lv_tela4(void);
void lv_tela5(void);

static void tela1_handler(lv_event_t *e);
static void run_handler(lv_event_t *e);
static void pause_play_handler(lv_event_t *e);
static void stop_handler(lv_event_t *e);
static void config_handler(lv_event_t *e);
static void back_handler(lv_event_t *e);
static void roller_handler(lv_event_t *e);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;)
	{
	}
}

extern void vApplicationIdleHook(void) {}

extern void vApplicationTickHook(void) {}

extern void vApplicationMallocFailedHook(void)
{
	configASSERT((volatile void *)NULL);
}

/************************************************************************/
/* SEMÁFOROS                                                            */
/************************************************************************/

SemaphoreHandle_t xSemaphoreSeconds;
SemaphoreHandle_t xSemaphoreRunning;
SemaphoreHandle_t xSemaphoreWheel;
SemaphoreHandle_t xSemaphorePause;
QueueHandle_t xQueueScreens;
QueueHandle_t xQueuePulse;
// QueueHandle_t xQueueResults;

/************************************************************************/
/* CORES                                                                */
/************************************************************************/

static inline lv_color_t lv_color_red(void)
{
	return lv_color_make(0xf4, 0x21, 0x35);
}

static inline lv_color_t lv_color_cyan(void)
{
	return lv_color_make(0x21, 0xf4, 0xe0);
}

/************************************************************************/
/* TELAS                                                                */
/************************************************************************/

void lv_tela1(void)
{
	img1 = lv_imgbtn_create(lv_scr_act());
	lv_imgbtn_set_src(img1, LV_IMGBTN_STATE_RELEASED, NULL, NULL, &tela1);
	lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_size(img1, 240, 320);
	lv_obj_add_event_cb(img1, tela1_handler, LV_EVENT_ALL, NULL);
}

void lv_tela2(void)
{
	lv_obj_t *label;
	static lv_style_t style;

	lv_style_init(&style);
	// lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_RED)); # mostrar o bug pro professor dps
	lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_width(&style, 5);

	velocimeter = lv_label_create(lv_scr_act());
	lv_obj_align(velocimeter, LV_ALIGN_CENTER, 0, -75);
	lv_obj_set_style_text_font(velocimeter, &montserrat_65, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(velocimeter, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(velocimeter, "%d.%d", 0, 0);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_CENTER, 0, -15);
	lv_obj_set_style_text_font(label, &montserrat_32, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km/h");

	play = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(play, run_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play, LV_ALIGN_BOTTOM_MID, 0, -50);
	lv_obj_add_style(play, &style, 0);
	lv_obj_set_width(play, 80);
	lv_obj_set_height(play, 80);
	label = lv_label_create(play);
	lv_label_set_text(label, LV_SYMBOL_PLAY);
	lv_obj_set_style_text_color(play, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(clock, "%d:%02d:%02d", 12, 30, 0);
}

void lv_tela3(void)
{
	lv_obj_t *label;
	static lv_style_t style;

	lv_style_init(&style);
	// lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_RED)); # mostrar o bug pro professor dps
	lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_width(&style, 5);

	velocimeter = lv_label_create(lv_scr_act());
	lv_obj_align(velocimeter, LV_ALIGN_CENTER, 0, -105);
	lv_obj_set_style_text_font(velocimeter, &montserrat_65, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(velocimeter, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(velocimeter, "%d.%d", 0, 0);

	kmporh = lv_label_create(lv_scr_act());
	lv_obj_align(kmporh, LV_ALIGN_CENTER, 0, -50);
	lv_obj_set_style_text_font(kmporh, &montserrat_32, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(kmporh, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(kmporh, "km/h");

	up = lv_label_create(lv_scr_act());
	lv_obj_align(up, LV_ALIGN_CENTER, -90, -105);
	lv_obj_add_style(up, &style, 0);
	lv_obj_set_style_text_font(up, &acceleration_symbols, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(up, lv_color_cyan(), LV_STATE_DEFAULT);

	down = lv_label_create(lv_scr_act());
	lv_obj_align(down, LV_ALIGN_CENTER, 90, -105);
	lv_obj_add_style(down, &style, 0);
	lv_obj_set_style_text_font(down, &acceleration_symbols, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(down, lv_color_red(), LV_STATE_DEFAULT);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Seu Trajeto");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 15);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Distância:");

	distance = lv_label_create(lv_scr_act());
	lv_obj_align(distance, LV_ALIGN_CENTER, 0, 15);
	lv_obj_set_style_text_font(distance, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(distance, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(distance, "%2.1f", 0.0);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 15);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 50);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "V. Média:");

	speed = lv_label_create(lv_scr_act());
	lv_obj_align(speed, LV_ALIGN_CENTER, 0, 50);
	lv_obj_set_style_text_font(speed, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(speed, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(speed, "%2.1f", 0.0);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 50);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km/h ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 85);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Tempo:");

	timer = lv_label_create(lv_scr_act());
	lv_obj_align(timer, LV_ALIGN_CENTER, 0, 85);
	lv_obj_set_style_text_font(timer, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(timer, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(timer, "%02d:%02d:%02d", 0, 0, 0);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 85);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "h:m:s ");

	config = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(config, config_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(config, LV_ALIGN_BOTTOM_MID, -80, -10);
	lv_obj_add_style(config, &style, 0);
	lv_obj_set_width(config, 50);
	lv_obj_set_height(config, 50);
	label = lv_label_create(config);
	lv_label_set_text(label, LV_SYMBOL_SETTINGS);
	lv_obj_center(label);

	stop = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(stop, stop_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, 0, -10);
	lv_obj_add_style(stop, &style, 0);
	lv_obj_set_width(stop, 50);
	lv_obj_set_height(stop, 50);
	label = lv_label_create(stop);
	lv_label_set_text(label, LV_SYMBOL_STOP);
	lv_obj_set_style_text_color(stop, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	btn_pause_play = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn_pause_play, pause_play_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn_pause_play, LV_ALIGN_BOTTOM_MID, 80, -10);
	lv_obj_add_style(btn_pause_play, &style, 0);
	lv_obj_set_width(btn_pause_play, 50);
	lv_obj_set_height(btn_pause_play, 50);
	label_pause_play = lv_label_create(btn_pause_play);
	lv_label_set_text(label_pause_play, LV_SYMBOL_PAUSE);
	lv_obj_set_style_text_color(label_pause_play, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label_pause_play);

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
}

void lv_tela4(void)
{
	lv_obj_t *label;
	static lv_style_t style;

	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_width(&style, 5);

	lv_obj_t *labelTraj = lv_label_create(lv_scr_act());
	lv_obj_align(labelTraj, LV_ALIGN_TOP_MID, 0, 35);
	lv_obj_set_style_text_font(labelTraj, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelTraj, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelTraj, "Trajeto Encerrado");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 100);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Tempo\nGasto");
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);

	timer = lv_label_create(lv_scr_act());
	lv_obj_align(timer, LV_ALIGN_TOP_MID, 0, 100);
	lv_obj_set_style_text_font(timer, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(timer, lv_color_white(), LV_STATE_DEFAULT);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 100);
	lv_obj_set_style_text_font(label, &montserrat_16, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "h:m:s ");

	play = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(play, run_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play, LV_ALIGN_BOTTOM_MID, 0, -50);
	lv_obj_add_style(play, &style, 0);
	lv_obj_set_width(play, 80);
	lv_obj_set_height(play, 80);
	label = lv_label_create(play);
	lv_label_set_text(label, LV_SYMBOL_PLAY);
	lv_obj_set_style_text_color(play, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
}

void lv_tela5(void)
{
	lv_obj_t *label;
	static lv_style_t style, style_roller, style_roller_sel;

	// Estilos comuns
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_width(&style, 5);

	// Estilo padrão do roller
	lv_style_init(&style_roller);
	lv_style_set_bg_color(&style_roller, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style_roller, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_text_font(&style_roller, &montserrat_18);

	// Estilo para o item selecionado do roller
	lv_style_init(&style_roller_sel);
	lv_style_set_text_font(&style_roller_sel, &montserrat_32);
	lv_style_set_bg_color(&style_roller_sel, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style_roller_sel, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_transform_zoom(&style_roller_sel, 256); // 256 é a escala normal, aumentar para zoom

	// Criação e configuração do roller
	lv_obj_t *roller = lv_roller_create(lv_scr_act());
	lv_obj_align(roller, LV_ALIGN_CENTER, 50, -35);
	lv_roller_set_options(roller,
						  "21\n"
						  "22\n"
						  "23\n"
						  "24\n"
						  "25\n"
						  "26\n"
						  "27\n"
						  "28\n"
						  "29",
						  LV_ROLLER_MODE_NORMAL);
	lv_roller_set_visible_row_count(roller, 3);
	lv_obj_add_style(roller, &style_roller, LV_PART_MAIN);
	lv_obj_add_style(roller, &style_roller_sel, LV_PART_SELECTED);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 35);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Ajustar DIAMETRO");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_CENTER, -50, -35);
	lv_obj_set_style_text_font(label, &montserrat_32, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Aro:");

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(clock, "%d:%02d:%02d", 12, 30, 0);

	back = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(back, back_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(back, LV_ALIGN_BOTTOM_MID, -100, 0);
	lv_obj_add_style(back, &style, 0);
	label = lv_label_create(back);
	lv_label_set_text(label, LV_SYMBOL_NEW_LINE);
	lv_obj_center(label);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 80);
	lv_obj_add_style(label, &style, 0);
	lv_obj_set_style_text_font(label, &wheel_symbol, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text(label, MY_WHEEL_SYMBOL);
}

/************************************************************************/
/* HANDLERS / CALLBACKS                                                 */
/************************************************************************/

void sensor_callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreWheel, &xHigherPriorityTaskWoken);
}

static void tela1_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		enum screen scr = TELA2;
		xQueueSendFromISR(xQueueScreens, &scr, 0);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void run_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		xSemaphoreGive(xSemaphoreRunning);
		enum screen scr = TELA3;
		xQueueSendFromISR(xQueueScreens, &scr, 0);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void pause_play_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED)
	{
		xSemaphoreGiveFromISR(xSemaphorePause, 0);
	}
}

static void stop_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		enum screen scr = TELA4;
		xQueueSendFromISR(xQueueScreens, &scr, 0);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void config_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		enum screen scr = TELA5;
		xQueueSendFromISR(xQueueScreens, &scr, 0);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void back_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		enum screen scr = TELA3;
		xQueueSendFromISR(xQueueScreens, &scr, 0);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void roller_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_target(e);
	if (code == LV_EVENT_VALUE_CHANGED)
	{
		char buf[32];
		lv_roller_get_selected_str(obj, buf, sizeof(buf));
		LV_LOG_USER("Tire: %s\n", buf);
	}
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC)
	{
		// o código para irq de segundo vem aqui
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphoreSeconds, &xHigherPriorityTaskWoken);
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}

	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM)
	{
		// o código para irq de alame vem aqui
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void RTT_Handler(void)
{
	uint32_t ul_status;
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS)
	{
		// RTT_init(1000, 4000, RTT_MR_ALMIEN);
		// BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		// xSemaphoreGiveFromISR(xSemaphoreRTT, &xHigherPriorityTaskWoken);
	}
}

/************************************************************************/
/* FUNCOES                                                              */
/************************************************************************/

calendar get_difference(calendar start, calendar end)
{
	calendar diff;

	int end_second = end.second;
	int start_second = start.second;
	int end_minute = end.minute;
	int start_minute = start.minute;
	int end_hour = end.hour;
	int start_hour = start.hour;
	int end_day = end.day;
	int start_day = start.day;
	int end_week = end.week;
	int start_week = start.week;
	int end_month = end.month;
	int start_month = start.month;
	int end_year = end.year;
	int start_year = start.year;

	// Subtrai os segundos
	int second = end_second - start_second;
	if (second < 0)
	{
		second += 60;
		end_minute--;
	}

	// Subtrai os minutos
	int minute = end_minute - start_minute;
	if (minute < 0)
	{
		minute += 60;
		end_hour--;
	}

	// Subtrai as horas
	int hour = end_hour - start_hour;
	if (hour < 0)
	{
		hour += 24;
		end_day--;
	}

	// Subtrai os dias
	int day = end_day - start_day;
	if (day < 0)
	{
		day += 30; // Ajuste adicional pode ser necessário dependendo do mês/ano
		end_month--;
	}

	// Subtrai as semanas
	int week = end_week - start_week;
	if (week < 0)
	{
		week += 52;
		end_year--;
	}

	// Subtrai os meses
	int month = end_month - start_month;
	if (month < 0)
	{
		month += 12;
		end_year--;
	}

	// Subtrai os anos
	int year = end_year - start_year;

	diff.second = second;
	diff.minute = minute;
	diff.hour = hour;
	diff.day = day;
	diff.week = week;
	diff.month = month;
	diff.year = year;
	
	return diff;
}

calendar sum_time(calendar start, calendar end)
{
	calendar sum;

	// Soma os segundos
	sum.second = start.second + end.second;
	if (sum.second >= 60)
	{
		sum.second -= 60;
		start.minute++;
	}

	// Soma os minutos
	sum.minute = start.minute + end.minute;
	if (sum.minute >= 60)
	{
		sum.minute -= 60;
		start.hour++;
	}

	// Soma as horas
	sum.hour = start.hour + end.hour;
	if (sum.hour >= 24)
	{
		sum.hour -= 24;
		start.day++;
	}

	// Soma os dias
	sum.day = start.day + end.day; // Ajuste adicional pode ser necessário dependendo do mês/ano

	// Meses e anos não são alterados
	sum.month = start.month;
	sum.year = start.year;

	return sum;
}

/************************************************************************/
/* INIT                                                                 */
/************************************************************************/

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type)
{
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc, irq_type);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource)
{

	uint16_t pllPreScale = (int)(((float)32768) / freqPrescale);

	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);

	if (rttIRQSource & RTT_MR_ALMIEN)
	{
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT))
			;
		rtt_write_alarm_time(RTT, IrqNPulses + ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
		rtt_enable_interrupt(RTT, rttIRQSource);
	else
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

float kmh_to_hz(float vel, float raio)
{
	float f = vel / (2 * PI * raio * 3.6);
	return (f);
}

static void task_simulador(void *pvParameters)
{

	pmc_enable_periph_clk(ID_PIOC);
	pio_set_output(PIOC, PIO_PC31, 1, 0, 0);

	float vel = VEL_MAX_KMH;
	float f;
	int ramp_up = 1;

	while (1)
	{
		pio_clear(PIOC, PIO_PC31);
		delay_ms(1);
		pio_set(PIOC, PIO_PC31);

#ifdef RAMP

		if (ramp_up)
		{
			// printf("[SIMU] ACELERANDO: %d \n", (int)(10 * vel));
			vel += 0.5;
		}
		else
		{
			// printf("[SIMU] DESACELERANDO: %d \n", (int)(10 * vel));
			vel -= 0.5;
		}

		if (vel >= VEL_MAX_KMH)
			ramp_up = 0;
		else if (vel <= VEL_MIN_KMH)
			ramp_up = 1;

#else

		vel = 5;
		printf("[SIMU] CONSTANTE: %d \n", (int)(10 * vel));

#endif

		f = kmh_to_hz(vel * 10, RAIO_RODA);
		int t = 720 * (1.0 / f); // UTILIZADO 965 como multiplicador ao invés de 1000
								 // para compensar o atraso gerado pelo Escalonador do freeRTOS
		delay_ms(t);
	}
}

static void task_lcd(void *pvParameters)
{
	enum screen screen;
	int wheel_counter = 0;
	char *wheel_radius_s = "";
	double wheel_radius = 0.29;
	static int lastTick = 0;
	static float distanciaTotal = 0;
	static float velocidadeMediaKmh = 0;
	static float bufferVelocidades[TAMANHO_BUFFER_VELOCIDADE] = {0};
	static int indiceBuffer = 0;
	static float somaVelocidades = 0;
	static int contadorVelocidades = 0;
	static float velocidadeInstantaneaAnterior = 0;

	calendar run_start = {0, 0, 0, 0, 0, 0, 0};
	calendar pause_start = {0, 0, 0, 0, 0, 0, 0};
	calendar pause_duration = {0, 0, 0, 0, 0, 0, 0};
	calendar run = {0, 0, 0, 0, 0, 0, 0};
	calendar rtc_initial = {2024, 12, 12, 50, 12, 30, 0};

	int pause = 1;
	int run_time = 0;
	int restart = 0;
	int freq = 100;

	int current_hour, current_min, current_sec;

	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	RTT_init(100, 0, 0);

	lv_tela1();

	for (;;)
	{
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);

		if (xSemaphoreTake(xSemaphoreSeconds, 0))
		{
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			calendar current_time = {0, 0, 0, 0, current_hour, current_min, current_sec};

			if (xQueueReceiveFromISR(xQueueScreens, &screen, 1))
			{
				lv_obj_clean(lv_scr_act());
				printf("TELA%d\n", screen+2);
				switch (screen)
				{
				case TELA2:
					lv_tela2();
					break;
				case TELA3:
					lv_tela3();
					break;
				case TELA4:
					lv_tela4();
					break;
				case TELA5:
					lv_tela5();
					break;
				default:
					break;
				}
			}

			lv_label_set_text_fmt(clock, "%02d:%02d:%02d", current_hour, current_min, current_sec);

			if (screen == TELA3 && !pause && (run_start.hour != 0 || run_start.minute != 0 || run_start.second != 0))
			{
				run = get_difference(run_start, current_time);
				calendar teste = get_difference(pause_duration, run);
				lv_label_set_text_fmt(timer, "%d:%02d:%02d", teste.hour, teste.minute, teste.second);

				printf("teste = %02d:%02d:%02d\n", teste.hour, teste.minute, teste.second);
				printf("current_time = %02d:%02d:%02d\n", current_time.hour, current_time.minute, current_time.second);
			}
		}

		if (xSemaphoreTake(xSemaphoreRunning, 0))
		{
			pause = 0;
			int start_hour, start_min, start_sec;
			rtc_get_time(RTC, &start_hour, &start_min, &start_sec);
			run_start = (calendar){0, 0, 0, 0, start_hour, start_min, start_sec};
			// printf("run_start = %02d:%02d:%02d\n", run_start.hour, run_start.minute, run_start.second);

			distanciaTotal = 0;
			velocidadeMediaKmh = 0;
			indiceBuffer = 0;
			somaVelocidades = 0;
			contadorVelocidades = 0;
			velocidadeInstantaneaAnterior = 0;

			for (int i = 0; i < TAMANHO_BUFFER_VELOCIDADE; i++)
				bufferVelocidades[i] = 0;

			lv_label_set_text_fmt(distance, "%.1f", 0.0);
			lv_label_set_text_fmt(speed, "%.1f", 0.0);
		}

		if (xSemaphoreTake(xSemaphorePause, 0))
		{
			if (pause == 0)
			{
				lv_label_set_text(label_pause_play, LV_SYMBOL_PLAY);
				lv_obj_set_style_text_color(label_pause_play, lv_color_cyan(), LV_STATE_DEFAULT);
				int start_pause_hour, start_pause_min, start_pause_sec;
				rtc_get_time(RTC, &start_pause_hour, &start_pause_min, &start_pause_sec);

				pause_start = (calendar){0, 0, 0, 0, start_pause_hour, start_pause_min, start_pause_sec};
				printf("pause_start = %02d:%02d:%02d\n", start_pause_hour, start_pause_min, start_pause_sec);
			}
			else
			{
				lv_label_set_text(label_pause_play, LV_SYMBOL_PAUSE);
				lv_obj_set_style_text_color(label_pause_play, lv_color_red(), LV_STATE_DEFAULT);
				rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
				calendar current_time = {0, 0, 0, 0, current_hour, current_min, current_sec};
				pause_duration = sum_time(pause_duration, get_difference(pause_start, current_time));
				printf("pause_duration = %02d:%02d:%02d\n", pause_duration.hour, pause_duration.minute, pause_duration.second);
			}
			pause = !pause;
		}

		if (pause == 1)
			run_time = 0;
		else
			run_time = 1;

		if (xSemaphoreTake(xSemaphoreWheel, portMAX_DELAY))
		{
			int currentTick = rtt_read_timer_value(RTT);
			int deltaTicks = currentTick - lastTick;
			lastTick = currentTick;

			float deltaT = deltaTicks * 0.01;
			if (deltaT == 0)
				continue;

			float velocidadeAngular = 2 * PI / deltaT;
			float velocidadeLinearInstantanea = velocidadeAngular * wheel_radius;
			float velocidadeInstantaneaKmh = velocidadeLinearInstantanea * 3.6;

			if (screen == TELA2 || screen == TELA3)
			{
				char buffer[20];
				sprintf(buffer, "%.1f", velocidadeInstantaneaKmh);
				lv_label_set_text(velocimeter, buffer);

				if (velocidadeInstantaneaKmh > velocidadeInstantaneaAnterior)
				{
					lv_label_set_text(up, MY_UP_SYMBOL);
					lv_label_set_text(down, ' ');
				}
				else if (velocidadeInstantaneaKmh < velocidadeInstantaneaAnterior)
				{
					lv_label_set_text(up, ' ');
					lv_label_set_text(down, MY_DOWN_SYMBOL);
				}
				velocidadeInstantaneaAnterior = velocidadeInstantaneaKmh;
			}

			if (run_time == 1)
			{
				if (screen == TELA3)
				{
					float distanciaPulso = 2 * PI * wheel_radius / 1000;
					distanciaTotal += distanciaPulso;
					somaVelocidades -= bufferVelocidades[indiceBuffer];
					bufferVelocidades[indiceBuffer] = velocidadeInstantaneaKmh;
					somaVelocidades += velocidadeInstantaneaKmh;
					indiceBuffer = (indiceBuffer + 1) % TAMANHO_BUFFER_VELOCIDADE;
					contadorVelocidades = (contadorVelocidades < TAMANHO_BUFFER_VELOCIDADE) ? contadorVelocidades + 1 : TAMANHO_BUFFER_VELOCIDADE;
					velocidadeMediaKmh = somaVelocidades / contadorVelocidades;

					char buffer[20];
					sprintf(buffer, "%.1f", velocidadeMediaKmh);
					lv_label_set_text(speed, buffer);
					sprintf(buffer, "%.1f", distanciaTotal);
					lv_label_set_text(distance, buffer);
				}
			}

			if (screen == TELA4)
			{
				calendar total_time = sum_time(run, pause_duration);
				lv_label_set_text_fmt(timer, "%d:%02d:%02d", total_time.hour, total_time.minute, total_time.second);
			}
		}
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void)
{
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS); //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);

	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

void init(void)
{
	sysclk_init();

	// Desativa WatchDog Timer
	WDT->WDT_MR = WDT_MR_WDDIS;

	pmc_enable_periph_clk(RECEBIMENTO_PIO_ID);
	pio_configure(RECEBIMENTO_PIO, PIO_INPUT, RECEBIMENTO_PIO_IDX_MASK, PIO_DEBOUNCE);
	pio_handler_set(RECEBIMENTO_PIO, RECEBIMENTO_PIO_ID, RECEBIMENTO_PIO_IDX_MASK, PIO_IT_FALL_EDGE, sensor_callback);

	pio_enable_interrupt(RECEBIMENTO_PIO, RECEBIMENTO_PIO_IDX_MASK);
	pio_get_interrupt_status(RECEBIMENTO_PIO);

	NVIC_EnableIRQ(RECEBIMENTO_PIO_ID);
	NVIC_SetPriority(RECEBIMENTO_PIO_ID, 4); // Prioridade 4
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	ili9341_set_top_left_limit(area->x1, area->y1);
	ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

	/* IMPORTANT!!!
	 * Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	int px, py, pressed;

	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED;

	data->point.x = py;
	data->point.y = 320 - px;
}

void configure_lvgl(void)
{
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);

	lv_disp_drv_init(&disp_drv);	   /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;	   /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;   /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX; /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX; /*Set the vertical resolution in pixels*/

	lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/

	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void)
{
	/* board and sys init */
	board_init();
	init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	ili9341_set_orientation(ILI9341_FLIP_Y | ILI9341_SWITCH_XY);

	xSemaphoreSeconds = xSemaphoreCreateBinary();
	if (xSemaphoreSeconds == NULL)
		printf("falha em criar o semaforo seconds\n");

	xSemaphoreRunning = xSemaphoreCreateBinary();
	if (xSemaphoreRunning == NULL)
		printf("falha em criar o semaforo running\n");

	xSemaphoreWheel = xSemaphoreCreateBinary();
	if (xSemaphoreWheel == NULL)
		printf("falha em criar o semaforo wheel\n");

	xSemaphorePause = xSemaphoreCreateBinary();
	if (xSemaphorePause == NULL)
		printf("falha em criar o semaforo pause\n");

	xQueueScreens = xQueueCreate(32, sizeof(enum screen));
	if (xQueueScreens == NULL)
		printf("falha em criar a fila screens\n");

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS)
	{
		printf("Failed to create lcd task\r\n");
	}

	if (xTaskCreate(task_simulador, "SIMUL", TASK_SIMULATOR_STACK_SIZE, NULL, TASK_SIMULATOR_STACK_PRIORITY, NULL) != pdPASS)
	{
		printf("Failed to create lcd task\r\n");
	}

	/* Start the scheduler. */
	vTaskStartScheduler();

	while (1)
	{
	}
}
