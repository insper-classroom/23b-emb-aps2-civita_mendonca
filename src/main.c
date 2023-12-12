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

LV_FONT_DECLARE(montserrat_65);
LV_FONT_DECLARE(montserrat_32);
LV_FONT_DECLARE(montserrat_18);
LV_FONT_DECLARE(acceleration_symbols);
LV_FONT_DECLARE(wheel_symbol);
LV_FONT_DECLARE(font_awesome_test);
// LV_FONT_DECLARE(up_symbol);

#define MY_CLOCK_SYMBOL "\xEF\x80\x97"
#define MY_UP_SYMBOL "\xEF\x84\x82"
#define MY_DOWN_SYMBOL "\xEF\x84\x83"
#define MY_WHEEL_SYMBOL "\xEF\x88\x86"

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (320)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

lv_obj_t *img1;
lv_obj_t *img2;
lv_obj_t *velocimeter;
lv_obj_t *kmporh;
lv_obj_t *play;
lv_obj_t *pause;
lv_obj_t *stop;
lv_obj_t *config;
lv_obj_t *home;
lv_obj_t *back;
lv_obj_t *wheel;
lv_obj_t *up;
lv_obj_t *down;
lv_obj_t *clock;
lv_obj_t *roller;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);
static void pause_handler(lv_event_t *e);
static void play2_handler(lv_event_t *e);

void lv_tela1(void);
void lv_tela2(void);
void lv_tela3(void);
void lv_tela4(void);
void lv_tela5(void);

static void tela1_handler(lv_event_t * e);
static void play1_handler(lv_event_t *e);
static void play2_handler(lv_event_t *e);
static void pause_handler(lv_event_t *e);
static void stop_handler(lv_event_t *e);
static void config_handler(lv_event_t *e);
static void back_handler(lv_event_t *e);
static void roller_handler(lv_event_t *e);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

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
	lv_obj_add_event_cb(play, play1_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play, LV_ALIGN_BOTTOM_MID, 0, -50);
	lv_obj_add_style(play, &style, 0);
	lv_obj_set_width(play, 80);
	lv_obj_set_height(play, 80);
	label = lv_label_create(play);
	lv_label_set_text(label, LV_SYMBOL_PLAY);
	lv_obj_set_style_text_color(play, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	config = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(config, config_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(config, LV_ALIGN_BOTTOM_MID, -100, 0);
	lv_obj_add_style(config, &style, 0);
	label = lv_label_create(config);
	lv_label_set_text(label, LV_SYMBOL_SETTINGS);
	lv_obj_center(label);

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(clock, "%02d:%02d", 12, 30);
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
	lv_label_set_text(up, MY_UP_SYMBOL);

	down = lv_label_create(lv_scr_act());
	lv_obj_align(down, LV_ALIGN_CENTER, 90, -105);
	lv_obj_add_style(down, &style, 0);
	lv_obj_set_style_text_font(down, &acceleration_symbols, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(down, lv_color_red(), LV_STATE_DEFAULT);
	lv_label_set_text(down, MY_DOWN_SYMBOL);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_CENTER, 0, -10);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Seu Trajeto");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 25);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Distancia:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 25);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 65);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Vel. Media:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 65);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km/h ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 105);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Tempo:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 105);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "min ");

	config = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(config, config_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(config, LV_ALIGN_BOTTOM_MID, -100, 0);
	lv_obj_add_style(config, &style, 0);
	label = lv_label_create(config);
	lv_label_set_text(label, LV_SYMBOL_SETTINGS);
	lv_obj_center(label);

	stop = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(stop, stop_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(stop, LV_ALIGN_BOTTOM_MID, 60, 0);
	lv_obj_add_style(stop, &style, 0);
	label = lv_label_create(stop);
	lv_label_set_text(label, LV_SYMBOL_STOP);
	lv_obj_set_style_text_color(stop, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	pause = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(pause, pause_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(pause, LV_ALIGN_BOTTOM_MID, 95, 0);
	lv_obj_add_style(pause, &style, 0);
	label = lv_label_create(pause);
	lv_label_set_text(label, LV_SYMBOL_PAUSE);
	lv_obj_set_style_text_color(pause, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);

	clock = lv_label_create(lv_scr_act());
	lv_obj_align(clock, LV_ALIGN_TOP_RIGHT, -20, 5);
	lv_obj_set_style_text_font(clock, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(clock, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(clock, "%02d:%02d", 12, 30);
}

void lv_tela4(void)
{
	lv_obj_t *label;
	static lv_style_t style;

	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_border_width(&style, 5);

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 35);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Trajeto Encerrado");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 80);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Distancia:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 80);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 120);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Vel. Media:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 120);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "km/h ");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 160);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "Tempo:");

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 160);
	lv_obj_set_style_text_font(label, &montserrat_18, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "min ");

	play = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(play, play1_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(play, LV_ALIGN_BOTTOM_MID, 0, -50);
	lv_obj_add_style(play, &style, 0);
	lv_obj_set_width(play, 80);
	lv_obj_set_height(play, 80);
	label = lv_label_create(play);
	lv_label_set_text(label, LV_SYMBOL_PLAY);
	lv_obj_set_style_text_color(play, lv_color_red(), LV_STATE_DEFAULT);
	lv_obj_center(label);
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
	lv_label_set_text_fmt(clock, "%02d:%02d", 12, 30);

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
/* HANDLERS                                                             */
/************************************************************************/

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void tela1_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		lv_obj_clean(lv_scr_act());
		lv_tela2();
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void play1_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		lv_obj_clean(lv_scr_act());
		lv_tela3();
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void play2_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		// apagar o botao play e colocar o pause no lugar
		lv_obj_del(play);

		lv_obj_t *label;
		static lv_style_t style;

		lv_style_init(&style);
		lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
		lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
		lv_style_set_border_width(&style, 5);

		pause = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(pause, pause_handler, LV_EVENT_ALL, NULL);
		lv_obj_align(pause, LV_ALIGN_BOTTOM_MID, 95, 0);
		lv_obj_add_style(pause, &style, 0);
		label = lv_label_create(pause);
		lv_label_set_text(label, LV_SYMBOL_PAUSE);
		lv_obj_set_style_text_color(pause, lv_color_red(), LV_STATE_DEFAULT);
		lv_obj_center(label);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void pause_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		// apagar o botao play e colocar o pause no lugar
		lv_obj_del(pause);

		lv_obj_t *label;
		static lv_style_t style;

		lv_style_init(&style);
		// lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_RED)); # mostrar o bug pro professor dps
		lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_NONE));
		lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_NONE));
		lv_style_set_border_width(&style, 5);

		play = lv_btn_create(lv_scr_act());
		lv_obj_add_event_cb(play, play2_handler, LV_EVENT_ALL, NULL);
		lv_obj_align(play, LV_ALIGN_BOTTOM_MID, 95, 0);
		lv_obj_add_style(play, &style, 0);
		label = lv_label_create(play);
		lv_label_set_text(label, LV_SYMBOL_PAUSE);
		lv_obj_set_style_text_color(play, lv_color_cyan(), LV_STATE_DEFAULT);
		lv_obj_center(label);
	}
	else if (code == LV_EVENT_VALUE_CHANGED)
	{
		LV_LOG_USER("Toggled");
	}
}

static void stop_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		lv_obj_clean(lv_scr_act());
		lv_tela4();
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
		lv_obj_clean(lv_scr_act());
		lv_tela5();
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
		lv_obj_clean(lv_scr_act());
		lv_tela3();
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

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	lv_tela1();

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
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

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

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
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();
	ili9341_set_orientation(ILI9341_FLIP_Y | ILI9341_SWITCH_XY);

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
