#include <U8x8lib.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "Pitches.h"
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Software SPI setup:
#define OLED_MOSI  14
#define OLED_CLK   15
#define OLED_DC    16
#define OLED_CS    12
#define OLED_RESET 13

U8X8_SSD1306_128X64_NONAME_4W_SW_SPI screen(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET);

//Global defines
#define BLINKER_DELAY 10000
#define BLINKER_FLASH_ON 500
#define BLINKER_FLASH_OFF 150

//Pin assignments
//Shift register control
#define DS 4
#define STCP 2
#define SHCP 7
#define MR 8
//Shift register pin assignments.
#define LED_blinkerRight 3
#define LED_blinkerLeft 2
#define LED_lampFront 0
#define LED_lampBack 1
//Other pinouts.
#define BUZZER 3
#define LED_indicatorRed 9
#define LED_indicatorGreen 10
#define LED_indicatorBlue 11
//Input pins.
#define INPUT_DELAY 300
#define IN_mode 17
#define IN_left 18
#define IN_right 19
#define MODE_MAX 2

//Global variables
//Shift register buffer.
bool outputBuffer[8] = { 0/*Front lamp*/, 0/*Back lamp*/, 0/*Left blinker*/, 0/*Right blinker*/, 0, 0, 0, 0 };
//-Blinker Variables
bool blinkerRight = 0;
bool blinkerLeft = 0;
unsigned long blinkerRightStart = 0;
unsigned long blinkerLeftStart = 0;
unsigned long blinkerRightLastUpdate = 0;
unsigned long blinkerLeftLastUpdate = 0;
//Input control variables
//Mode control button.
char mode = 0;
bool modePressed, leftPressed, rightPressed = 0;
unsigned long modePressedTime, leftPressedTime, rightPressedTime = 0;
//Music variables.
short BPM, noteDuration;
short* currentSequence;
bool isPlaying = false;
int currentNoteIndex;
unsigned long noteEndTime;

short beep1[] = {
	3, 100,
	NOTE_A4, 2,
	NOTE_E6, 2,
	NOTE_C8, 2
};

short skyrim[] = {
	104, 120,
	NOTE_D5, 8,
	NOTE_E5, 8,
	NOTE_F5, 8 / 3,
	NOTE_F5, 8,
	NOTE_F5, 8,
	NOTE_G5, 8,
	NOTE_A5, 8 / 3,
	NOTE_A5, 8,
	NOTE_A5, 8,
	NOTE_C5, 8,//10
	NOTE_G5, 8 / 3,
	NOTE_G5, 8,
	NOTE_F5, 8,
	NOTE_E5, 8,
	NOTE_D5, 8 / 3,
	NOTE_D5, 8,
	NOTE_D5, 8,
	NOTE_E5, 8,
	NOTE_F5, 8 / 3,
	NOTE_F5, 8,//20
	NOTE_F5, 8,
	NOTE_G5, 8,
	NOTE_A5, 8 / 3,
	NOTE_A5, 8,
	NOTE_A5, 8,
	NOTE_C5, 8,
	NOTE_D5, 8 / 3,
	NOTE_D5, 8,
	NOTE_C5, 8,
	NOTE_E5, 8,//30
	NOTE_D5, 8 / 3,
	NOTE_D5, 8,
	NOTE_D5, 8,
	NOTE_E5, 8,
	NOTE_F5, 4,
	NOTE_E5, 8,
	NOTE_E5, 8,
	NOTE_D5, 4,
	NOTE_C5, 4,
	NOTE_B5, 8,//40
	NOTE_B5, 8,
	NOTE_A5, 4,
	NOTE_G5, 8 / 3,
	NOTE_G5, 8,
	NOTE_F5, 8,
	NOTE_A5, 8,
	NOTE_G5, 8 / 3,
	NOTE_G5, 4,
	NOTE_F5, 16,
	NOTE_E5, 16,//50
	NOTE_F5, 4,
	NOTE_F5, 16,
	NOTE_E5, 16,
	NOTE_F5, 4,
	NOTE_F5, 16,
	NOTE_E5, 16,
	NOTE_G5, 8,
	NOTE_F5, 8,
	NOTE_E5, 8,
	NOTE_D5, 4,//60
	NOTE_D5, 16,
	NOTE_C5, 16,
	NOTE_D5, 4,
	NOTE_D5, 16,
	NOTE_C5, 16,
	NOTE_D5, 4,
	NOTE_C5, 16,
	NOTE_D5, 16,
	NOTE_E5, 8,
	NOTE_F5, 8,//70
	NOTE_C5, 8,
	NOTE_D5, 4,
	NOTE_D5, 16,
	NOTE_E5, 16,
	NOTE_F5, 4,
	NOTE_F5, 16,
	NOTE_G5, 16,
	NOTE_A5, 4,
	NOTE_E5, 16,
	NOTE_F5, 16,//80
	NOTE_G5, 8,
	NOTE_F5, 8,
	NOTE_E5, 8,
	NOTE_D5, 4,
	NOTE_D5, 16,
	NOTE_C5, 16,
	NOTE_D5, 4,
	NOTE_D5, 16,
	NOTE_C5, 16,
	NOTE_D5, 4,//90
	NOTE_C5, 16,
	NOTE_D5, 16,
	NOTE_E5, 8,
	NOTE_F5, 8,
	NOTE_C5, 8,
	NOTE_D5, 8 / 3,
	NOTE_D5, 4,
	NOTE_REST, 8,
	NOTE_D5, 4,
	NOTE_REST, 8,//100
	NOTE_D5, 4,
	NOTE_REST, 8,
	NOTE_D5, 4,
	NOTE_REST, 8
};

//-------------------|
//GRAPHICS FUNCTIONS:|
//-------------------|

//Shortcut for writing text to the screen.
void writeText(const char* text, int x, int y) {
	screen.setCursor(x, y);
	screen.print(text);
}

//Draws the template of the main screen onto the display.
void drawMainScreen() {
	screen.clear();
	screen.setInverseFont(0);
	screen.setFont(u8x8_font_amstrad_cpc_extended_f);
	writeText("Bicycle Control", 0, 0);
	screen.setFont(u8x8_font_artossans8_r);
	writeText("+-+-+-+-+------+", 0, 1);
	writeText("|L|R|F|B|Mode:0|", 0, 2);
	writeText("+-+-+-+-+------+", 0, 3);
	writeText("|Song:         |", 0, 4);
	writeText("+--------------+", 0, 5);
}

//Draws the blinker indicators to the screen.
void drawBlinkerStatus() {
	screen.setFont(u8x8_font_artossans8_r);
	screen.setInverseFont(blinkerLeft);
	writeText("L", 1, 2);
	screen.setInverseFont(blinkerRight);
	writeText("R", 3, 2);
}

//Draws the front lamp indicator.
void drawFrontLampStatus() {
	screen.setFont(u8x8_font_artossans8_r);
	screen.setInverseFont(outputBuffer[LED_lampFront]);
	writeText("F", 5, 2);
}

//Draws the back lamp indicator.
void drawBackLampStatus() {
	screen.setFont(u8x8_font_artossans8_r);
	screen.setInverseFont(outputBuffer[LED_lampBack]);
	writeText("B", 7, 2);
}

//Draws the current mode number on the screen.
void drawModeStatus() {
	screen.setFont(u8x8_font_artossans8_r);
	screen.setInverseFont(0);
	screen.setCursor(14, 2);
	screen.print((int)mode);
}

//-------------------|
//STARTUP FUNCTIONS: |
//-------------------|

//Initialize the OLED screen.
void startupScreen() {
	screen.begin();
	screen.setFont(u8x8_font_amstrad_cpc_extended_r);
}

//Initialize peripherals.
void startupPeripherals() {
	//Shift register outputs.
	pinMode(DS, OUTPUT);
	pinMode(STCP, OUTPUT);
	pinMode(SHCP, OUTPUT);
	pinMode(MR, OUTPUT);
	digitalWrite(MR, HIGH);
	//Other outputs.
	pinMode(BUZZER, OUTPUT);
	pinMode(LED_indicatorRed, OUTPUT);
	pinMode(LED_indicatorGreen, OUTPUT);
	pinMode(LED_indicatorBlue, OUTPUT);
	//Inputs.
	pinMode(IN_mode, INPUT);
	pinMode(IN_left, INPUT);
	pinMode(IN_right, INPUT);
}

//-------------------|
//UPDATE FUNCTIONS:  |
//-------------------|

//Updates the display with blinker status.
void updateBlinkerStatus() {
	bool needsRefresh = 0;
	unsigned long currentTime = millis();
	//Check if the blinkers must shut off.
	if (blinkerLeft) {
		//Check if we should try blinking the light.
		if (currentTime - blinkerLeftStart < BLINKER_DELAY) {
			if (outputBuffer[LED_blinkerLeft]) {
				//Test if the blinker needs to be flashed off.
				if (currentTime - blinkerLeftLastUpdate > BLINKER_FLASH_ON) {
					setPin(LED_blinkerLeft, 0);
					blinkerLeftLastUpdate = currentTime;
				}
			}
			else {
				//Test if the blinker needs to be flashed on.
				if (currentTime - blinkerLeftLastUpdate > BLINKER_FLASH_OFF) {
					setPin(LED_blinkerLeft, 1);
					blinkerLeftLastUpdate = currentTime;
				}
			}
		}
		else {
			//Try to shut down the blinker.
			setPin(LED_blinkerLeft, 0);
			blinkerLeft = 0;
			needsRefresh = 1;
		}
	}
	if (blinkerRight) {
		//Check if we should try blinking the light.
		if (currentTime - blinkerRightStart < BLINKER_DELAY) {
			if (outputBuffer[LED_blinkerRight]) {
				//Test if the blinker needs to be flashed off.
				if (currentTime - blinkerRightLastUpdate > BLINKER_FLASH_ON) {
					setPin(LED_blinkerRight, 0);
					blinkerRightLastUpdate = currentTime;
				}
			}
			else {
				//Test if the blinker needs to be flashed on.
				if (currentTime - blinkerRightLastUpdate > BLINKER_FLASH_OFF) {
					setPin(LED_blinkerRight, 1);
					blinkerRightLastUpdate = currentTime;
				}
			}
		}
		else {
			//Try to shut down the blinker.
			setPin(LED_blinkerRight, 0);
			blinkerRight = 0;
			needsRefresh = 1;
		}
	}
	//Only if a change has been made, update the display.
	if (needsRefresh) drawBlinkerStatus();
}

//Checks for user button presses, and processes input.
void updateInputs() {
	unsigned long currentTime = millis();
	if (!modePressed && digitalRead(IN_mode)) {
		//Mode button was pressed.
		modePressed = 1;
		modePressedTime = currentTime;
		if (mode == MODE_MAX) {
			mode = 0;
		}
		else {
			mode++;
		}
		drawModeStatus();
		//Serial.println("Mode pressed.");
	}
	if (!leftPressed && digitalRead(IN_left)) {
		//Left button was pressed.
		leftPressed = 1;
		leftPressedTime = currentTime;
		switch (mode) {
		case 0:
			setFrontLamp(!outputBuffer[LED_lampFront]);
			break;
		case 1:
			if (!blinkerLeft) activateBlinker(0);
			break;
		}
		//Serial.println("Left pressed.");
	}
	if (!rightPressed && digitalRead(IN_right)) {
		//Right button was pressed.
		rightPressed = 1;
		rightPressedTime = currentTime;
		switch (mode) {
		case 0:
			setBackLamp(!outputBuffer[LED_lampBack]);
			break;
		case 1:
			if (!blinkerRight) activateBlinker(1);
			break;
		}
		//Serial.println("Right pressed.");
	}
	//Remove restrictions on time delays.
	if (currentTime - modePressedTime > INPUT_DELAY) {
		modePressed = 0;
	}
	if (currentTime - leftPressedTime > INPUT_DELAY) {
		leftPressed = 0;
	}
	if (currentTime - rightPressedTime > INPUT_DELAY) {
		rightPressed = 0;
	}
}

//Updates the music player, and potentially quits the music.
void updateSoundPlay() {
	if (isPlaying && (noteEndTime < millis()) && currentNoteIndex < currentSequence[0]) {
		//It is necessary to play the next note.
		int realNoteIndex = currentNoteIndex * 2 + 2;
		int realDuration = noteDuration / currentSequence[currentNoteIndex * 2 + 3];
		tone(BUZZER, currentSequence[realNoteIndex], realDuration);
		noteEndTime = millis() + realDuration;
		currentNoteIndex++;
	}
	if (currentNoteIndex == currentSequence[0]) {
		isPlaying = false;
	}
}

//Updates all modules.
void updateAll() {
	updateBlinkerStatus();
	updateInputs();
	updateSoundPlay();
}

//-------------------|
//STATE CHANGE FUNC: |
//-------------------|

//Loads 8 bits into the register.
void loadRegister(bool values[8]) {
	for (int i = 7; i >= 0; --i) {
		digitalWrite(DS, values[i]);
		digitalWrite(SHCP, HIGH);
		delayMicroseconds(1);
		digitalWrite(SHCP, LOW);
	}
	digitalWrite(STCP, HIGH);
	delayMicroseconds(1);
	digitalWrite(STCP, LOW);
}

//Sets a pin on the register and reloads the bits.
void setPin(byte index, bool value) {
	outputBuffer[index] = value;
	loadRegister(outputBuffer);
}

//Activates a blinker for BLINK_DELAY milliseconds, until it is shut off by the updateBlinkStatus. 0 = Left, 1 = Right
void activateBlinker(bool side) {
	unsigned long currentTime = millis();
	if (side) {
		setPin(LED_blinkerRight, 1);
		blinkerRight = 1;
		blinkerRightStart = currentTime;
		blinkerRightLastUpdate = currentTime;
		Serial.println("Activated Right blinker.");
	}
	else {
		setPin(LED_blinkerLeft, 1);
		blinkerLeft = 1;
		blinkerLeftStart = currentTime;
		blinkerLeftLastUpdate = currentTime;
		Serial.println("Activated Left blinker.");
	}
	drawBlinkerStatus();
}

//Controls the front lamp.
void setFrontLamp(bool value) {
	setPin(LED_lampFront, value);
	drawFrontLampStatus();
}

//Controls the back lamp.
void setBackLamp(bool value) {
	setPin(LED_lampBack, value);
	drawBackLampStatus();
}

//Sets the color of the indicator LED.
void setIndicatorColor(byte red, byte green, byte blue) {
	analogWrite(LED_indicatorRed, red);
	analogWrite(LED_indicatorGreen, green);
	analogWrite(LED_indicatorBlue, blue);
}

//-------------------|
//MUSIC FUNCTIONS:   |
//-------------------|

void setBPM(short newBPM) {
	BPM = newBPM;
	noteDuration = 240000 / BPM;
}

//Begins playing a new sequence of the form: {Length, BPM, note 1, time 1, note 2, time 2, ...}
void playSequence(short* sequence) {
	currentSequence = sequence;
	setBPM(sequence[1]);
	currentNoteIndex = 0;
	noteEndTime = 0;
	isPlaying = true;
}

//-------------------|
//ERROR CHECKING:    |
//-------------------|

void testOutput(short pinNumber, const char* name) {
	screen.clear();
	writeText(name, 0, 0);
	digitalWrite(pinNumber, HIGH);
	delay(500);
	digitalWrite(pinNumber, LOW);
}

//Visually tests each output.
void testOutputs() {
	screen.clear();
	writeText("Outputs test...", 0, 0);
	delay(1000);
	testOutput(LED_blinkerLeft, "Left blinker");
	testOutput(LED_blinkerRight, "Right blinker");
	testOutput(BUZZER, "Sound");
	testOutput(LED_indicatorRed, "Indic. Red");
	testOutput(LED_indicatorGreen, "Indic. Green");
	testOutput(LED_indicatorBlue, "Indic. Blue");
	testOutput(LED_lampFront, "Front lamp");
	testOutput(LED_lampBack, "Back lamp");
}

//-------------------|
//ARDUINO RUNTIME:   |
//-------------------|

void setup() {
	Serial.begin(9600);
	//Startup all the pins.
	startupPeripherals();
	startupScreen();
	drawMainScreen();
	updateAll();
	playSequence(beep1);
}

void loop() {
	updateBlinkerStatus();
	updateInputs();
	updateSoundPlay();
}

