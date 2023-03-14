#include <Arduino.h>

/*
https://notisrac.github.io/FileToCArray/
http://www.piskelapp.com
*/

#include <EEPROM.h>
#include <TimerOne.h>
#include <Encoder.h>
#include <Bounce2.h>
#include <U8glib.h>
#include <SI5351.h>
#include <Wire.h>
#include <stdarg.h>

#include "bitmap_logo.h"
#include "bitmap_off.h"
#include "bitmap_on.h"
#include "bitmap_saved.h"
#include "bitmap_triangle.h"

#define NUMBEROFCLOCKS 4
#define NUMBEROFSECTIONS 3
#define PIN_BUTTON_SAVE 8
#define PIN_BUTTON_CLOCKSELECT 7
#define PIN_BUTTON_SECTIONSELECT 6
#define UI_HEADER_TEXT_POS_Y 6
#define UI_BODY_START_Y 6
#define UI_BODY_SECTION_START_X 41
#define UI_BODY_SECTION_WIDTH 23
#define UI_BODY_DOT_WIDTH 8
#define PIN_CLOCK 9 // TIMER1_A_PIN on arduino uno
#define BITMAPHEIGHT 9
#define FONTHEIGHT 14 // py - capital A
#define FONTWIDTH 8 // px - capital A
#define ROTARYENCODER_TICKPERSTEP 4
#define BUTTON_LONGPRESS 500 // ms
#define CLOCK_SET_DEBOUNCE 100 // ms
#define CLOCK_PWM_DUTYCYCLE 512 // 50%
#define EEPROM_POS_CLOCKDATA 0
#define SCREENSAVER_TIMEOUT 600000 // 10min
#define PIN_ENCODER_1 2 // 
#define PIN_ENCODER_2 3 // 

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

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NO_ACK);
//byte fontHeight = 14; // px - capital A
//byte fontWidth = 8; // px - capital A


const char formatPadSpace[] = "%3d";
const char formatPadZero[] = "%03d";

void draw() {
	// draw the header
	u8g.setFont(u8g_font_5x8r);
	u8g.drawStr(10, UI_HEADER_TEXT_POS_Y, F("#"));
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
		//Serial.println((clocks[i].enabled)? "on" : "off");
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
		clockGenerator.set_freq(clk.value * 100, (si5351_clock)clk.externalId);
	}
}

void toggleClockEnable(byte id) {
	Serial.print(F("Clock "));
	Serial.print(id);
	if (clocks[id].enabled) {
		clocks[id].enabled = false;
	}
	else {
		clocks[id].enabled = true;
	}
	Serial.print(F(" enabled: "));
	Serial.println(clocks[id].enabled);

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
	Serial.println(F("Clock Generator v1.0"));
	Serial.println("");

	// set up the clock generator
	// clockGenerator.init(SI5351_CRYSTAL_LOAD_10PF, 25000000, 0);
	Serial.print(F("Initializing SI5351..."));
	clockGenerator.init(SI5351_CRYSTAL_LOAD_10PF, 0, 0);
	Serial.println(F(" done"));
	// set a fixed PLL freq
	//clockGenerator.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
	// set output strength
	//clockGenerator.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
	// set the disabled mode to low on the clocks
	clockGenerator.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
	clockGenerator.set_clock_disable(SI5351_CLK1, SI5351_CLK_DISABLE_LOW);
	clockGenerator.set_clock_disable(SI5351_CLK2, SI5351_CLK_DISABLE_LOW);

	Serial.print(F("Initializing internal timer..."));
	Timer1.initialize(0); // init to 0Hz // 40 us = 25 kHz -> 1000000us(=1Hz) / 25000Hz
	Timer1.pwm(PIN_CLOCK, CLOCK_PWM_DUTYCYCLE); // this is a clock generator, so an 50% duty cycle is needed
	Serial.println(F(" done"));

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

	Serial.println(F("Startup done"));
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
		// Serial.println("Button pressed");
		btnClockPressTimeStamp = millis();
		clockStatusChanged = false;
	}
	if (btnClock.fell())
	{
		// Serial.println("Button released");
		if (!clockStatusChanged)
		{
			Serial.print(F("Active clock changed to: "));
			clockSelected++;
			if (clockSelected >= NUMBEROFCLOCKS)
			{
				clockSelected = 0;
			}
			Serial.println(clockSelected);
			shouldUpdateScreen = true;
		}
		clockStatusChanged = false;
	}
	if (HIGH == btnClock.read() && !clockStatusChanged)
	{
		if (millis() - btnClockPressTimeStamp > BUTTON_LONGPRESS)
		{
			// Serial.println("clock status changed");
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

		clocks[clockSelected].value = newValue;
		checkClockValue();
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
		updateScreen(1, draw);
		//delay(1000);
		shouldUpdateScreen = false;
	}
}