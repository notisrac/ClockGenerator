/*
http://www.digole.com/tools/PicturetoC_Hex_converter.php
http://www.piskelapp.com
*/

#include <EEPROM.h>
#include <TimerOne.h>
#include <Encoder.h>
#include <Bounce2.h>
#include <U8glib.h>
#include <si5351.h>
#include <Wire.h>
#include <stdarg.h>

#define NUMBEROFCLOCKS 4
#define NUMBEROFSECTIONS 3
#define PIN_BUTTON_SAVE 6
#define PIN_BUTTON_CLOCKSELECT 5
#define PIN_BUTTON_SECTIONSELECT 4
#define UI_HEADER_TEXT_POS_Y 6
#define UI_BODY_START_Y 6
#define UI_BODY_SECTION_START_X 41
#define UI_BODY_SECTION_WIDTH 23
#define UI_BODY_DOT_WIDTH 8
#define PIN_CLOCK 9 // TIMER1_A_PIN on arduino uno
#define BITMAPHEIGHT 9
#define FONTHEIGHT 14 // px - capital A
#define FONTWIDTH 8 // px - capital A
#define ROTARYENCODER_TICKPERSTEP 4
#define BUTTON_LONGPRESS 500 // ms
#define CLOCK_SET_DEBOUNCE 100 // ms
#define CLOCK_PWM_DUTYCYCLE 512 // 50%
#define EEPROM_POS_CLOCKDATA 0
#define SCREENSAVER_TIMEOUT 600000 // 10min
#define PIN_ENCODER_1 2 // TIMER1_A_PIN on arduino uno
#define PIN_ENCODER_2 3 // TIMER1_A_PIN on arduino uno

typedef void(*DrawableFunction)();

struct Clock // 15 bytes
{
	bool enabled; // 1 byte
	long value; // 4 bytes
	long valueMin; // 4 bytes
	long valueMax; // 4 bytes
	bool isInternal; // 1 byte
	byte externalId; // 1 byte
};

Clock clocks[NUMBEROFCLOCKS]; // 4 clocks = 4 * 15 bytes = 60 bytes

byte clockSelected = 0;
byte sectionSelected = 0;

bool clockStatusChanged = false;
bool clockSectionChanged = false;

Bounce btnClock = Bounce();
Bounce btnSection = Bounce();
Bounce btnSave = Bounce();
unsigned long btnClockPressTimeStamp;
unsigned long btnSectionPressTimeStamp;
unsigned long lastButtonPress = 0;

Encoder encValueSelect(PIN_ENCODER_1, PIN_ENCODER_2);
long encoderPosition = 0;

byte screenSaverX = 1;
byte screenSaverY = 1;
bool screenSaverXDirection = true;
bool screenSaverYDirection = true;
unsigned long lastScreenSaverUpdate = 0;

unsigned long clockLastChanged = 0;

Si5351 clockGenerator;

bool shouldUpdateScreen = false;

U8GLIB_SH1106_128X64_2X u8g(U8G_I2C_OPT_NO_ACK);
//byte fontHeight = 14; // px - capital A
//byte fontWidth = 8; // px - capital A

const uint8_t bitmap_triangle[] U8G_PROGMEM = {
	B01000000,
	B01100000,
	B01110000,
	B01111000,
	B01111100,
	B01111000,
	B01110000,
	B01100000,
	B01000000
};

const uint8_t bitmap_on[] U8G_PROGMEM = {
	B00000000,
	B00000000,
	B00000000,
	B01011100,
	B10110010,
	B10110010,
	B01010010,
	B00000000,
	B00000000
};

const uint8_t bitmap_off[] U8G_PROGMEM = {
	B00000000,
	B00001001,
	B00010010,
	B01011111,
	B10110010,
	B10110010,
	B01010010,
	B00000000,
	B00000000
};

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

// 40x40
const uint8_t bitmap_saved[] U8G_PROGMEM = {
	 0x1f,0xff,0xff,0xff,0xc0
	,0x1f,0x80,0x00,0x03,0xe0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x03,0xe3,0xf0
	,0x1f,0x80,0x00,0x03,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x3f,0xff,0xf9,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x3f,0xff,0xf9,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x3f,0xff,0xf9,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0x00,0x00,0x01,0xf0
	,0x1f,0xff,0xff,0xff,0xf0
	,0x00,0x00,0x00,0x00,0x00
	,0x00,0x00,0x00,0x00,0x00
	,0x07,0xdf,0x45,0xf7,0x80
	,0x04,0x11,0x45,0x04,0x40
	,0x04,0x11,0x45,0x04,0x40
	,0x07,0xdf,0x45,0xe4,0x40
	,0x00,0x51,0x29,0x04,0x40
	,0x07,0xd1,0x11,0xf7,0x80
	,0x00,0x00,0x00,0x00,0x00
};

const char formatPadSpace[] = "%3d";
const char formatPadZero[] = "%03d";

void draw() {
	// draw the header
	u8g.setFont(u8g_font_5x8r);
	u8g.drawStr(0, UI_HEADER_TEXT_POS_Y, F("out"));
	u8g.drawStr(19, UI_HEADER_TEXT_POS_Y, F("stat"));
	u8g.drawStr(49, UI_HEADER_TEXT_POS_Y, F("MHz"));
	u8g.drawStr(81, UI_HEADER_TEXT_POS_Y, F("KHz"));
	u8g.drawStr(117, UI_HEADER_TEXT_POS_Y, F("Hz"));
	u8g.drawLine(0, UI_HEADER_TEXT_POS_Y + 1, 160, UI_HEADER_TEXT_POS_Y + 1);
	// draw the body - 1234567890123456
	u8g.setFont(u8g_font_unifontr);
	for (byte i = 0; i < NUMBEROFCLOCKS; i++)
	{
		char text[17] = "               ";
		byte yPos = UI_BODY_START_Y + FONTHEIGHT * (1 + i);
		// clock id
		sprintf(text + 1, "%i", i + 1);
		text[2] = ' ';
		// the separated values of the current clock
		int valHz = clocks[i].value % 1000;
		int valKHz = (clocks[i].value / 1000) % 1000;
		int valMHz = clocks[i].value / 1000000;
		// print the value in 000.000.000 format
		if (clocks[i].value >= 1000000)
		{ // mhz range
			sprintf(text + 5, formatPadSpace, valMHz);
			text[8] = '.';
		}
		if (clocks[i].value >= 1000)
		{ // khz range
			sprintf(text + 9, (valMHz > 0) ? formatPadZero : formatPadSpace, valKHz);
			text[12] = '.';
		}
		sprintf(text + 13, (valKHz > 0 || valMHz > 0)? formatPadZero : formatPadSpace, valHz);
		// print out the text for this line
		u8g.drawStr(0, yPos, text);
		// display the status icon for this line
		u8g.drawBitmapP(FONTWIDTH * 3, yPos - BITMAPHEIGHT, 1, BITMAPHEIGHT, (clocks[i].enabled)? bitmap_on : bitmap_off);
	}
	// clock select triangle
	u8g.drawBitmapP(0, UI_BODY_START_Y +  FONTHEIGHT * (clockSelected + 1) - BITMAPHEIGHT - 1, 1, BITMAPHEIGHT, bitmap_triangle);
	// section select line
	u8g.drawLine(UI_BODY_SECTION_START_X + sectionSelected * UI_BODY_SECTION_WIDTH + UI_BODY_DOT_WIDTH * sectionSelected, UI_BODY_START_Y + FONTHEIGHT * (clockSelected + 1) + 1, UI_BODY_SECTION_START_X + sectionSelected * UI_BODY_SECTION_WIDTH + UI_BODY_SECTION_WIDTH + UI_BODY_DOT_WIDTH * sectionSelected, UI_BODY_START_Y + FONTHEIGHT * (clockSelected + 1) + 1);
}

void drawSplashScreen() {
	u8g.drawBitmapP(16, 7, 12, 41, bitmap_logo);
	u8g.setFont(u8g_font_5x8r);
	u8g.drawStr(3, 63, F("v1.0"));
	u8g.drawStr(75, 63, F("noti, 2015"));
}

void drawSavedIcon() {
	u8g.setColorIndex(0);
	u8g.drawBox(42, 10, 43, 43);
	u8g.setColorIndex(1);
	u8g.drawBitmapP(44, 12, 5, 40, bitmap_saved);
}

void drawScreenSaver() {
	u8g.drawBitmapP(screenSaverX - 7, screenSaverY, 12, 41, bitmap_logo);
	if (lastScreenSaverUpdate + 100 < millis())
	{
		lastScreenSaverUpdate = millis();
		if (screenSaverXDirection)
		{
			screenSaverX++;
		}
		else
		{
			screenSaverX--;
		}
		if (screenSaverYDirection)
		{
			screenSaverY++;
		}
		else
		{
			screenSaverY--;
		}
		if (screenSaverX <= 1)
		{
			screenSaverXDirection = true;
		}
		if (screenSaverX >= 127 - 83)
		{
			screenSaverXDirection = false;
		}
		if (screenSaverY <= 1)
		{
			screenSaverYDirection = true;
		}
		if (screenSaverY >= 63 - 40)
		{
			screenSaverYDirection = false;
		}
	}
}

void updateScreen(byte num, ...);

void updateScreen(byte num, ...) {
	va_list arguments;

	u8g.firstPage();
	do {
		// Initializing arguments to store all values after num
		va_start(arguments, num);
		for (byte i = 0; i < num; i++)
		{
			DrawableFunction drawFunction = va_arg(arguments, DrawableFunction);
			drawFunction();
		}
		// clean up the list
		va_end(arguments);
	} while (u8g.nextPage());
}

void setClocks() {
	Clock clk = clocks[clockSelected];
	if (!clk.enabled)
	{
		return;
	}
	if (clk.isInternal)
	{
		Timer1.setPeriod(1000000 / clk.value);
		Timer1.pwm(PIN_CLOCK, CLOCK_PWM_DUTYCYCLE);
	}
	else
	{
		//Serial.print("external clock (");
		//Serial.print(clk.externalId);
		//Serial.print("): ");
		//Serial.println(clk.value);
		clockGenerator.set_freq(clk.value * 100, /*SI5351_PLL_FIXED*/0, (si5351_clock)clk.externalId);
	}
}

void toggleClockEnable(byte id) {
	clocks[id].enabled = !clocks[id].enabled;
	if (clocks[id].isInternal)
	{
		if (!clocks[id].enabled)
		{
			Timer1.stop();
			digitalWrite(PIN_CLOCK, LOW);
			//Timer1.disablePwm(PIN_CLOCK);
		}
		else
		{
			Timer1.start();
			setClocks();
			//setInternalClock(clocks[id].value);
		}
	}
	else
	{
		clockGenerator.output_enable((si5351_clock)clocks[id].externalId, clocks[id].enabled);
	}
}

void checkClockValue() {
	if (clocks[clockSelected].value >= clocks[clockSelected].valueMax)
	{
		clocks[clockSelected].value = clocks[clockSelected].valueMax;
	}
	if (clocks[clockSelected].value <= clocks[clockSelected].valueMin)
	{
		clocks[clockSelected].value = clocks[clockSelected].valueMin;
	}
}

void saveClocks() {
	EEPROM.put(EEPROM_POS_CLOCKDATA, clocks);
}

void loadClocks() {
	EEPROM.get(EEPROM_POS_CLOCKDATA, clocks);

	// just in case
	clocks[0].externalId = SI5351_CLK0;
	clocks[0].isInternal = false;
	clocks[0].valueMax = 160000000;
	clocks[0].valueMin = 8000;
	clocks[1].externalId = SI5351_CLK1;
	clocks[1].isInternal = false;
	clocks[1].valueMax = 160000000;
	clocks[1].valueMin = 8000;
	clocks[2].externalId = SI5351_CLK2;
	clocks[2].isInternal = false;
	clocks[2].valueMax = 160000000;
	clocks[2].valueMin = 8000;
	clocks[3].externalId = 0;
	clocks[3].isInternal = true;
	clocks[3].valueMax = 10000;
	clocks[3].valueMin = 1;
}

void setup() {
	Serial.begin(115200);

	// set up the clock generator
	clockGenerator.init(SI5351_CRYSTAL_LOAD_10PF, 25000000);
	// set a fixed PLL freq
	//clockGenerator.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
	// set output strength
	//clockGenerator.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
	// set the disabled mode to low on the clocks
	clockGenerator.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
	clockGenerator.set_clock_disable(SI5351_CLK1, SI5351_CLK_DISABLE_LOW);
	clockGenerator.set_clock_disable(SI5351_CLK2, SI5351_CLK_DISABLE_LOW);

	Timer1.initialize(0); // init to 0Hz // 40 us = 25 kHz -> 1000000us(=1Hz) / 25000Hz
	Timer1.pwm(PIN_CLOCK, CLOCK_PWM_DUTYCYCLE); // this is a clock generator, so an 50% duty cycle is needed

	// set up the debounced clock select button
	pinMode(PIN_BUTTON_CLOCKSELECT, INPUT);
	btnClock.attach(PIN_BUTTON_CLOCKSELECT);
	btnClock.interval(5);
	// set up the debounced section select button
	pinMode(PIN_BUTTON_SECTIONSELECT, INPUT);
	btnSection.attach(PIN_BUTTON_SECTIONSELECT);
	btnSection.interval(5);
	// set up the debounced save button
	pinMode(PIN_BUTTON_SAVE, INPUT);
	btnSave.attach(PIN_BUTTON_SAVE);
	btnSave.interval(5);

	shouldUpdateScreen = true;
	clockStatusChanged = false;
	clockSectionChanged = false;

	///* TEMP
	//clocks[0] = { true, 8000, 8000, 160000000, false, SI5351_CLK0 };
	//clocks[1] = { false, 8000, 8000, 160000000, false, SI5351_CLK1 };
	//clocks[2] = { false, 8000, 8000, 160000000, false, SI5351_CLK2 };
	//clocks[3] = { true, 8, 1, 8000, true, 0 };

	//clockGenerator.set_freq(25000000, SI5351_PLL_FIXED, SI5351_CLK0);
	//clockGenerator.clock_enable(SI5351_CLK0, 1);
	//clockGenerator.set_freq(25000000, 0, SI5351_CLK1);
	//clockGenerator.clock_enable(SI5351_CLK1, 0);
	//clockGenerator.set_freq(4000000, 0, SI5351_CLK2);
	//clockGenerator.clock_enable(SI5351_CLK2, 0);
	/// /* TEMP

	// display the splash screen
	//u8g.firstPage();
	//do {
	//	drawSplashScreen();
	//} while (u8g.nextPage());
	updateScreen(1, drawSplashScreen);
	delay(1500);

	// load the clock data from the eprom
	loadClocks();

	// set the initial value
	for (byte i = 0; i < NUMBEROFCLOCKS; i++)
	{
		clockSelected = i;
		checkClockValue();
		setClocks();
	}
	clockSelected = 0;
	lastButtonPress = millis();
}

void loop() {
	// update the debouncers
	btnClock.update();
	btnSection.update();
	btnSave.update();

	if (btnClock.fell() || btnClock.rose() || btnSave.fell() || btnSave.rose() || btnSection.fell() || btnSection.rose())
	{
		lastButtonPress = millis();
	}

	// screensaver
	if (lastButtonPress + SCREENSAVER_TIMEOUT < millis())
	{ // activate screensaver
		lastScreenSaverUpdate = millis();
		bool screeSaverActive = true;
		while (screeSaverActive)
		{
			btnClock.update();
			btnSection.update();
			btnSave.update();

			if (btnClock.fell() || btnClock.rose() || btnSave.fell() || btnSave.rose() || btnSection.fell() || btnSection.rose())
			{
				screeSaverActive = false;
				lastButtonPress = millis();
			}

			updateScreen(1, drawScreenSaver);
		}
		btnClock.update();
		btnSection.update();
		btnSave.update();
	}

	// handle the clock select button actions
	if (btnClock.rose())
	{
		//Serial.println("Button pressed");
		btnClockPressTimeStamp = millis();
		clockStatusChanged = false;
	}
	if (btnClock.fell())
	{
		//Serial.println("Button released");
		if (!clockStatusChanged)
		{
			//Serial.print("clock changed to: ");
			clockSelected++;
			if (clockSelected >= NUMBEROFCLOCKS)
			{
				clockSelected = 0;
			}
			//Serial.println(clockSelected);
			shouldUpdateScreen = true;
		}
		clockStatusChanged = false;
	}
	if (HIGH == btnClock.read() && !clockStatusChanged)
	{
		if (millis() - btnClockPressTimeStamp > BUTTON_LONGPRESS)
		{
			//Serial.println("clock disabled");
			//clocks[clockSelected].enabled = !clocks[clockSelected].enabled;
			toggleClockEnable(clockSelected);
			shouldUpdateScreen = true;
			clockStatusChanged = true;
		}
	}

	// handle the section select button actions
	if (btnSection.rose())
	{
		//Serial.println("Button pressed");
		btnSectionPressTimeStamp = millis();
		clockSectionChanged = false;
	}
	if (btnSection.fell())
	{
		//Serial.println("Button released");
		if (!clockSectionChanged)
		{
			//Serial.print("clock changed to: ");
			sectionSelected++;
			if (sectionSelected >= NUMBEROFSECTIONS)
			{
				sectionSelected = 0;
			}
			//Serial.println(clockSelected);
			shouldUpdateScreen = true;
		}
		clockSectionChanged = false;
	}
	if (HIGH == btnSection.read() && !clockSectionChanged)
	{
		if (millis() - btnSectionPressTimeStamp > BUTTON_LONGPRESS)
		{
			//Serial.println("clock disabled");
			//Serial.println(clocks[clockSelected].value);
			//Serial.println(NUMBEROFSECTIONS - 1 - sectionSelected);
			//Serial.println(round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected)));
			//Serial.println(clocks[clockSelected].value / round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected)));
			//Serial.println(round(clocks[clockSelected].value / round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected))));
			//Serial.println(round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected)));
			//Serial.println(round(clocks[clockSelected].value / round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected))) * round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected)));
			//clocks[clockSelected].value -= round(clocks[clockSelected].value / round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected))) * round(pow(1000, NUMBEROFSECTIONS - 1 - sectionSelected));
			//int valHz = clocks[sectionSelected].value % 1000;
			//int valKHz = (clocks[sectionSelected].value / 1000) % 1000;
			//int valMHz = clocks[sectionSelected].value / 1000000;
			switch (sectionSelected)
			{
				case 0:
					clocks[clockSelected].value -= (clocks[clockSelected].value / 1000000) * 1000000;
					break;
				case 1:
					clocks[clockSelected].value -= ((clocks[clockSelected].value / 1000) % 1000) * 1000;
					break;
				case 2:
					clocks[clockSelected].value -= (clocks[clockSelected].value % 1000);
					break;
				default:
					break;
			}
			checkClockValue();
			shouldUpdateScreen = true;
			clockSectionChanged = true;
			clockLastChanged = millis();
		}
	}

	// handle the save button
	if (btnSave.rose())
	{
		saveClocks();
		updateScreen(2, draw, drawSavedIcon);
		delay(700);
		shouldUpdateScreen = true;
	}

	long encoderNewPosition = encValueSelect.read();
	//if (encoderNewPosition != encoderPosition)
	if (abs(encoderNewPosition - encoderPosition) >= ROTARYENCODER_TICKPERSTEP)
	{
		int posDifference = round((encoderNewPosition - encoderPosition) / ROTARYENCODER_TICKPERSTEP);
		//Serial.print(encoderNewPosition - encoderPosition);
		long newValue = clocks[clockSelected].value;
		long modValue = 0;
		switch (sectionSelected)
		{
			case 0:
				modValue = posDifference * 1000000;
				break;
			case 1:
				modValue = posDifference * 1000;
				break;
			case 2:
				modValue = posDifference * 1;
				break;
			default:
				break;
		}

		newValue -= modValue;

		//if (newValue >= clocks[clockSelected].valueMax)
		//{
		//	newValue = clocks[clockSelected].valueMax;
		//}
		//if (newValue <= clocks[clockSelected].valueMin)
		//{
		//	newValue = clocks[clockSelected].valueMin;
		//}
		clocks[clockSelected].value = newValue;
		checkClockValue();
		//Serial.print(encoderPosition);
		//Serial.print("-");
		//Serial.print(encoderNewPosition);
		//Serial.print("=");
		//Serial.print(encoderPosition - encoderNewPosition);
		//Serial.println("");
		encoderPosition = encoderNewPosition;
		clockLastChanged = millis();
		shouldUpdateScreen = true;
	}

	// do not adjust the clock generators on every click
	if (0!= clockLastChanged && CLOCK_SET_DEBOUNCE > millis() - clockLastChanged)
	{
		clockLastChanged = 0;
		setClocks();
	}

	if (shouldUpdateScreen)
	{
		//u8g.firstPage();
		//do {
		//	draw();
		//} while (u8g.nextPage());
		updateScreen(1, draw);
		//delay(1000);
		shouldUpdateScreen = false;
	}
}