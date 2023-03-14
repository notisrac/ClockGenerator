#include <Arduino.h>
#include <U8glib.h>

const uint8_t bitmap_logo[] U8G_PROGMEM = {
	 0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe0,0x0f,0xff,0xf0,0x00,0x7f,0xff,0x80,0x03,0xf1,0xe0
	,0x01,0xe0,0x0f,0xff,0xf0,0x00,0x7f,0xff,0x80,0x03,0xf1,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe0,0x0c,0x00,0x30,0x00,0x60,0x01,0x80,0x03,0x01,0xe0
	,0x01,0xe3,0xfc,0x00,0x3f,0xff,0xe0,0x01,0xff,0xff,0x01,0xe0
	,0x01,0xe3,0xfc,0x00,0x3f,0xff,0xe0,0x01,0xff,0xff,0x01,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf9,0xe0
	,0x01,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf9,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe3,0xff,0x30,0x03,0x03,0x3f,0xf3,0xff,0x30,0x31,0xe0
	,0x01,0xe3,0xff,0x30,0x03,0x03,0x3f,0xf3,0xff,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x03,0x00,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x03,0x00,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x03,0x00,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x03,0x00,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x03,0x00,0x30,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0xfc,0x33,0xf3,0xfc,0x3e,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0xfc,0x33,0xf3,0xfc,0x3e,0x31,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x33,0x00,0x31,0xf1,0xe0
	,0x01,0xe3,0x00,0x30,0x03,0x03,0x30,0x33,0x00,0x31,0xf1,0xe0
	,0x01,0xe3,0xff,0x3f,0xf3,0x03,0x3f,0xf3,0xff,0x30,0x31,0xe0
	,0x01,0xe3,0xff,0x3f,0xf3,0x03,0x3f,0xf3,0xff,0x30,0x31,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x01,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xe0
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
	,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x00
};