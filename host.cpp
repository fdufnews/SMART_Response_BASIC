/*
        fdufnews
        12/2019

        Added support to SMART Response XE using SmartResponseXE library
        use terminal keyboard for input and display text on its LCD screen
        use ON/OFF button to put terminal in sleep mode
        added host_setFont to select active font
        added host_setLineWrap to choose between line wrap or truncated line
        added fontStatus structure to retrieve information relative to the text interface (size of font, nb car per line and line per screen)
        added setConf in order to change font (only at command line level)
        added support of MENU key which calls setConf to change font

          DONE: ADC configuration (using 1.6V internal ref)
          DONE: battery management, display battery state
          DONE 01/2020: font as an option --> dy// fdufnews 6/08/2020namic screen width and screen height
          2/3 DONE: last line used for status (battery state, font used, freemem) but issue with battery state
        
        fdufnews 6/08/2020
        added support for an SD card
        added save, load, dir, mount, unmount statements to manage the SD card
*/

#include "host.h"
#include "basic.h"

#include "logo2_rle.h"

#include <SmartResponseXE.h>
#include <MemoryFree.h>

#include <EEPROM.h>

extern EEPROMClass EEPROM;
int timer1_counter;

// Allocates a buffer sized for the larger screen mode
char screenBuffer[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
char lineDirty[MAX_SCREEN_HEIGHT];
int curX = 0, curY = 0;   // current position in the screen buffer
byte lineCount=0;         // count how many lines were scrolled (for dir and list)
volatile char flash = 0, redraw = 0;
char inputMode = 0;
char inkeyChar = 0;
char buzPin = 0;    // not 0 if buzzer present
char ledPin = 0;    // not 0 if LED wired
char ledState = 0;  // current state of the LED (to restore state after a sleep)

#if SD_CARD
#include <SPI.h>
#include <SD.h>

boolean sdCardPresent = false;  // true if an SD card has been initialized successfully
const char ok_card[] PROGMEM = " SD CARD OK ";
const char ko_card[] PROGMEM = " NO SD CARD ";
#endif
char currentFile[13] = " NO SD CARD ";

char *fontName[4] = {"NORMAL", "SMALL", "MEDIUM", "LARGE"};

struct sizeOfFont {
  unsigned char width;
  unsigned char height;
};
struct sizeOfFont fontSize[4] = {{9, 8}, {6, 8}, {12, 16}, {15, 16}};

struct sizeOfScreen {
  unsigned char nbCar;
  unsigned char nbLine;
};
// note that the screenSize definitions here under reserve one block of 8 lines to display status in all modes
struct sizeOfScreen screenSize[4] = {{42, 16}, {64, 16}, {32, 8}, {25, 8}};

// this structure holds all the information concerning the current screen configuration
// it is updated every time a new font is selected.
struct {
  unsigned char currentFont;  // font currently in use
  unsigned char width;          // width of the font
  unsigned char height;         // height of the font
  unsigned char nbCar;          // number of car per line
  unsigned char nbLine;         // number of line per screen
  unsigned int bufSize;         // size of the screen buffer
  boolean lineWrap;             // line wrap flag (not used)
} fontStatus = {FONT_NORMAL, 9, 8, 42, 16, 42 * 16, true};

const char bytesFreeStr[] PROGMEM = "bytes free";

void initTimer() {
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  timer1_counter = 34286;   // preload timer 65536-16MHz/256/2Hz
  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_OVF_vect)        // interrupt service routine
{
  TCNT1 = timer1_counter;   // preload timer
  flash = !flash;
  redraw = 1;
}

/*
 * host_init
 * Init parts specific to the host (display, dedicated I/Os)
 * 
 * 
 */
void host_init(int buzzer_Pin, int led_Pin) {
  char ledState = 0;

  //  Serial.begin(115200);
  buzPin = buzzer_Pin;
  ledPin = led_Pin;
  SRXEInit(0xe7, 0xd6, 0xa2); // initialize and clear display // CS, D/C, RESET
  host_setFont(FONT_NORMAL);
  SRXEFill(0);     // clear display
  // set on/off button as input
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  // Blink the LED
#if SHOW_SPLASHSCREEN
  host_splashscreen();  // displays the splashscreen
#endif
  if (buzPin)
    pinMode(buzPin, OUTPUT);
  SRXEFill(0);     // clear display
  initTimer();
#if SD_CARD
  if (sdCardPresent = SD.begin(SD_CARD_CS))   // true if an sd card is present
    strcpy_P(currentFile, ok_card);
  else
    strcpy_P(currentFile, ko_card);
#endif
  host_setBatStat();    // Initialize A0 for battery measurement
}

void host_sleep(long ms) {
  delay(ms);
}

void host_digitalWrite(int pin, int state) {
  if ((ledPin != 0) && (pin == ledPin)) // If LED present and we are modifying its state
    ledState = state;              // save its new state.
  digitalWrite(pin, state ? HIGH : LOW);
}

int host_digitalRead(int pin) {
  return digitalRead(pin);
}

int host_analogRead(int pin) {
  return analogRead(pin);
}

void host_pinMode(int pin, int mode) {
  pinMode(pin, mode);
}

/*
 * host_click
 * Generates a short click
 * 
 */
void host_click() {
  if (!buzPin) return;
  digitalWrite(buzPin, HIGH);
  delay(1);
  digitalWrite(buzPin, LOW);
}

/*
 * host_startupTone
 * A short "melody" after a reset
 * 
 */
void host_startupTone() {
  if (!buzPin) return;
  for (int i = 1; i <= 2; i++) {
    for (int j = 0; j < 50 * i; j++) {
      digitalWrite(buzPin, HIGH);
      delay(3 - i);
      digitalWrite(buzPin, LOW);
      delay(3 - i);
    }
    delay(100);
  }
}

// host_notone
//  halt tone generated on buzpin
//
void host_notone() {
  if (!buzPin) return;
  noTone(buzPin);
}

// host_tone
//  generates a tone on buzpin
//
void host_tone(int note) {
  if (!buzPin) return;
  tone(buzPin, note);
}

//    host_BASICFreeMem
// returns free BASIC memory
//
unsigned int host_BASICFreeMem(void) {
  return sysVARSTART - sysSTACKEND;
}

//    host_CFreeMem
// returns free C memory
//
unsigned int host_CFreeMem(void) {
  return freeMemory();
}

//    host_setFont
//  set the active font used to display text on screen
// input fontNum shall be one of FONT_NORMAL, FONT_SMALL, FONT_MEDIUM, FONT_LARGE
// output nothing
// updates fontStatus structure with new parameters
//
void host_setFont(unsigned char fontNum) {
  if (fontNum < 0 || fontNum > 3) return ;
  fontStatus.currentFont = fontNum;
  fontStatus.width = fontSize[fontNum].width;
  fontStatus.height = fontSize[fontNum].height;
  fontStatus.nbCar = screenSize[fontNum].nbCar;
  fontStatus.nbLine = screenSize[fontNum].nbLine;
  fontStatus.bufSize = fontStatus.nbCar * fontStatus.nbLine;
}

//   host_setBatStat
// set ADC configuration
// Select 1.6V ref voltage
// Select A0 as input
void host_setBatStat(void) {
  ADCSRA = 0; // Disable ADC
  ADMUX = 0xC0; // Int ref 1.6V
  ADCSRA = 0x87; // Enable ADC
  ADCSRB = 0x00; // MUX5= 0, freerun
  ADCSRC = 0x54; // Default value
  ADCSRA = 0x97; // Enable ADC
  DIDR0 = DIDR0 | ADC0D; // Disable logic buffer on ADC0
  //delay(5);
  ADCSRA |= (1 << ADSC); // start conversion
}

//    host_getBatStat
// returns battery's voltage in millivolts
// input nothing
// output battery voltage
// Battery is connected through a resistor divider (825k and 300k) with gain of 0.266666
// ADC ref is 1.6V with 1024 steps
//
float host_getBatStat(void) {
  uint16_t low, high;

  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  low = ADCL;
  high = ADCH;
  return (float)((high << 8) + low) * 1.6 / 1024 / 0.26666;
}

//    host_printStatus
//  print system status on bottom line of display
// Battery voltage, selected font, SD card state, memory usage
// Memory usage is Free BASIC mem and Free C mem
//  input
//  boolean now: if true print status even if STATUS_RATE has not expired
//  output nothing
//
void host_printStatus(boolean now) {
  static uint32_t last_update = 0L;
  uint32_t currentMillis;
  char buffer[43];
  char buf[16];

  currentMillis = millis();
  if (currentMillis - last_update > STATUS_RATE || now == true) {
    sprintf(buffer, " %4.4sV| %6.6s | %12s | %05d/%04d ", host_floatToStr(host_getBatStat(), buf), fontName[fontStatus.currentFont], currentFile, host_BASICFreeMem(), host_CFreeMem());
    SRXEWriteString(0, 135 - 8 + 1, buffer, FONT_NORMAL, 0, 3);
    last_update = currentMillis;
  }
}

//    host_setLineWrap
//  set Line Wrap status used with print
//  input wrap true if line wrap active, false if line truncated active
//  NOTE: not currently used
//
boolean host_setLineWrap(boolean wrap) {
  fontStatus.lineWrap = wrap;
  return wrap;
}

/*
 * host_cls
 * Clear screen and put cursor at home position
 * 
 */
void host_cls() {
  memset(screenBuffer, 32, fontStatus.bufSize);
  memset(lineDirty, 1, fontStatus.nbLine);
  curX = 0;
  curY = 0;
}

void host_moveCursor(int x, int y) {
  if (x < 0) x = 0;
  if (x >= fontStatus.nbCar) x = fontStatus.nbCar - 1;
  if (y < 0) y = 0;
  if (y >= fontStatus.nbLine) y = fontStatus.nbLine - 1;
  curX = x;
  curY = y;
}


/*
 * host_showBuffer
 * Updates display
 * To speed things, only the lines tagged as dirty are redrawn.
 * 
 */
void host_showBuffer() {
  for (int y = 0; y < fontStatus.nbLine; y++) {
    if (lineDirty[y] || (inputMode && y == curY)) {  // if line is tagged as changed or is the input line
      for (int x = 0; x < fontStatus.nbCar; x++) {   // scan the cars of this line
        char c = screenBuffer[y * fontStatus.nbCar + x];
        if (c < 32) c = ' ';     // ignore control cars
        if (x == curX && y == curY && inputMode && flash) c = 127;  // cursor management
        char buf[2] = {0, 0};
        buf[0] = c;
        if (c != 127) {
          SRXEWriteString(x * fontStatus.width, y * fontStatus.height, buf, fontStatus.currentFont, 3, 0);  // printable car
        } else {
          buf[0] = 0x20;
          SRXEWriteString(x * fontStatus.width, y * fontStatus.height, buf, fontStatus.currentFont, 0, 3);  // cursor
        }
      }
      lineDirty[y] = 0;  // line marked as updated
    }
  }
}


/*
 * scrollBuffer
 * scrolls the display one line up
 * count how many line were scrolled (used in newLine for list and dir, and maybe others)
 */
void scrollBuffer() {
  memcpy(screenBuffer, screenBuffer + fontStatus.nbCar, fontStatus.nbCar * (fontStatus.nbLine - 1)); // move content of screenbuffer one line up
  memset(screenBuffer + fontStatus.nbCar * (fontStatus.nbLine - 1), 32, fontStatus.nbCar);          // clear bottom line in the buffer
  memset(lineDirty, 1, fontStatus.nbLine);    // tag last line as dirty 
  lineCount++;     // count the total of scrolled lines
  curY--;           // update cursor position
}


/*
 * host_outputString
 * Puts a string in the screen buffer starting at the cursor position
 * 
 */
void host_outputString(char *str) {
  int pos = curY * fontStatus.nbCar + curX;
  while (*str) {
    lineDirty[pos / fontStatus.nbCar] = 1;
    screenBuffer[pos++] = *str++;
    if (pos >= fontStatus.bufSize) {
      scrollBuffer();
      pos -= fontStatus.nbCar;
    }
  }
  curX = pos % fontStatus.nbCar;
  curY = pos / fontStatus.nbCar;
}

/*
 * host_outputProgMemString
 * Puts a string stored in PROGMEM in the screen buffer starting at the cursor position
 * 
 */
void host_outputProgMemString(const char *p) {
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    host_outputChar(c);
  }
}

/*
 * host_outputChar
 * Puts a char in the screen buffer starting at the cursor position
 * 
 */
void host_outputChar(char c) {
  int pos = curY * fontStatus.nbCar + curX;
  lineDirty[pos / fontStatus.nbCar] = 1;
  screenBuffer[pos++] = c;
  if (pos >= fontStatus.bufSize) {
    scrollBuffer();
    pos -= fontStatus.nbCar;
  }
  curX = pos % fontStatus.nbCar;
  curY = pos / fontStatus.nbCar;
}

/*
 * host_outputInt
 * Puts an int in the screen buffer starting at the cursor position
 * 
 */
int host_outputInt(long num) {
  // returns len
  long i = num, xx = 1;
  int c = 0;
  do {
    c++;
    xx *= 10;
    i /= 10;
  }
  while (i);

  for (int i = 0; i < c; i++) {
    xx /= 10;
    char digit = ((num / xx) % 10) + '0';
    host_outputChar(digit);
  }
  return c;
}

/*
 * host_floatToStr
 * Converts a float to its string representation
 * 
 */
char *host_floatToStr(float f, char *buf) {
  // floats have approx 7 sig figs
  float a = fabs(f);
  if (f == 0.0f) {
    buf[0] = '0';
    buf[1] = 0;
  }
  else if (a < 0.0001 || a > 1000000) {
    // this will output -1.123456E99 = 13 characters max including trailing nul
    dtostre(f, buf, 6, 0);
  }
  else {
    int decPos = 7 - (int)(floor(log10(a)) + 1.0f);
    dtostrf(f, 1, decPos, buf);
    if (decPos) {
      // remove trailing 0s
      char *p = buf;
      while (*p) p++;
      p--;
      while (*p == '0') {
        *p-- = 0;
      }
      if (*p == '.') *p = 0;
    }
  }
  return buf;
}

/*
 * host_outputFloat
 * Prints a float in the screen buffer starting at the cursor position
 * 
 */
void host_outputFloat(float f) {
  char buf[16];
  host_outputString(host_floatToStr(f, buf));
}

#if SHOW_SPLASHSCREEN
//   host_splashscreen
//  displays a greeting screen
//
void host_splashscreen(void) {
  const int startHit = 138;
  const int lengthArrow = 84;
  const int startArrow = startHit + 132;
  int pos = startArrow;
  unsigned long pauseArrow = 500ul;
  unsigned long lastTimeArrow = 0;
  unsigned long currenTime = millis();

  SRXELoadBitmapRLE(0, 0, bitmap_logo2_rle);
  SRXEWriteString(startHit, 110, "Hit a key         ", FONT_MEDIUM, 3, 1);
  while (!SRXEGetKey()) {
    currenTime = millis();
    if (currenTime - lastTimeArrow >= pauseArrow) {
      SRXEWriteString(pos, 110, " ", FONT_MEDIUM, 3, 1);
      pos += 12;
      if (pos > startHit + 120 + lengthArrow) pos = startArrow;
      SRXEWriteString(pos, 110, ">", FONT_MEDIUM, 3, 1);
      lastTimeArrow = currenTime;
    }
  };
}
#endif

/*
 * host_newLine
 * Generates a linefeed
 * Increments line position and scroll buffer if necessary
 * 
 */
void host_newLine() {
  curX = 0;
  curY++;
  if (curY == fontStatus.nbLine)
    scrollBuffer();
  memset(screenBuffer + fontStatus.nbCar * (curY), 32, fontStatus.nbCar);
  lineDirty[curY] = 1;
}

/*
 * host_newLine
 * Generates a linefeed
 * Increments line position and scroll buffer if necessary
 * If one screen length has been scrolled wait for a keypress before scrolling
 *
 * Input
 *      num : if num = 0 init lineCount else test if bottom of screen has been reached
 * 
 */
void host_newLine(byte num) {
    if (num == 0){
        lineCount=1;        // initialize count
    } else {
        if(lineCount >= fontStatus.nbLine){  // if bottom of screen is reach
            lineCount=1;                       // reset count
            while(SRXEGetKey() == 0);       // wait for a keypress
        }
    }
    host_newLine();
}


// Function that put the terminal in sleep mode
// When waking up restores the display and the ADC configuration
void host_goToSleep(void) {
  if (ledPin) digitalWrite(ledPin, LOW); // if LED present switch it off
#if SD_CARD  
  host_unmountSD();   // unmount SD card so it can be unplugged while the terminal is sleeping
#endif
  SRXESleep(); // go into sleep mode and wait for an event (on/off button)
  // returning from sleep
#if SD_CARD  
  host_mountSD();     // try mounting the SD card just in case....
#endif
  // restore screen
#if SHOW_SPLASHSCREEN
  host_splashscreen();
#endif
  memset(lineDirty, 1, fontStatus.nbLine);  // all line dirty so the screen will be completely refreshed
  host_showBuffer();
  if (ledPin) digitalWrite(ledPin, ledState); // If LED present restore its state
  host_setBatStat(); // restore ADC configuration
}

//    host_setConf
//  Modifies configuration
// UP and DOWN change font
// LEFT and RIGHT change LCD contrast
// MENU or ENTER returns to calling function
// input
//  nothing
//
// output
//  true if display need to be updated (if font changed)
//
boolean host_setConf(void) {
  char c;
  boolean done = false;
  boolean needRedraw = false;
  unsigned char font = fontStatus.currentFont; // get current font
  while (!done) {
    while (c = SRXEGetKey()) {    // get a key
      switch (c) {
        case KEY_UP:
          font += 1;  // next font
          needRedraw = true;
          break;
        case KEY_DOWN:
          font -= 1;  // previous font
          needRedraw = true;
          break;
        case KEY_LEFT:
          SRXEDecreaseVop();
          break;
        case KEY_RIGHT:
          SRXEIncreaseVop();
          break;
        case KEY_MENU:
        case KEY_ENTER:
          done = true;  // exit setConf
          break;
      }
      if(needRedraw){
        font &= 3;          // limit font number to 0..3 range
        host_setFont(font); // update font parameters
        host_printStatus(true); // display updated status
        //delay(100);         // little delay just in case
      }
    }
  }
  return(needRedraw);
}

/*
 * host_readLine
 * high level keyboard scanning loop
 * wait for keyboard press
 *      keeps status line updated
 *      tests if power button depressed --> goto sleep
 *      tests if menu key depressed --> goto into configuration mode
 *      fills input buffer until enter key is depressed
 * 
 */
char *host_readLine() {
  inputMode = 1;

  if (curX == 0) memset(screenBuffer + fontStatus.nbCar * (curY), 32, fontStatus.nbCar);
  else host_newLine();

  int startPos = curY * fontStatus.nbCar + curX;
  int pos = startPos;

  bool done = false;
  while (!done) {
    host_printStatus(false);
    // test if we want to enter sleep mode
    if (digitalRead(POWER_BUTTON) == 0) {
      host_goToSleep();
    }
    while (char c = SRXEGetKey()) {

      host_click();
      // read the next key
      lineDirty[pos / fontStatus.nbCar] = 1;
      //char c = keyboard.read();
      if (c >= 32 && c <= 126)
        screenBuffer[pos++] = c;
      else if (c == KEY_DELETE && pos > startPos)
        screenBuffer[--pos] = 0;
      else if ((c == KEY_MENU)) {     // if we want to change configuration
        if(host_setConf()){           // call setConf to change font
          // update screen content if new font
          // move current line to top of screen
          memcpy(screenBuffer, screenBuffer + startPos, pos - startPos);
          pos = pos - startPos;
          startPos = 0;
          // clear from current pos to end of buffer
          memset(screenBuffer + pos, 32, fontStatus.bufSize - pos); // screenBuffer + pos + 1 or + pos???
          // tels that all the lines have changed
          memset(lineDirty, 1, fontStatus.nbLine);
        }
      }
      else if (c == KEY_ENTER)
        done = true;
      curX = pos % fontStatus.nbCar;
      curY = pos / fontStatus.nbCar;
      // scroll if we need to
      if (curY == fontStatus.nbLine) {
        if (startPos >= fontStatus.nbCar) {
          startPos -= fontStatus.nbCar;
          pos -= fontStatus.nbCar;
          scrollBuffer();
        }
        else
        {
          screenBuffer[--pos] = 0;
          curX = pos % fontStatus.nbCar;
          curY = pos / fontStatus.nbCar;
        }
      }
      redraw = 1;
    }
    if (redraw)
      host_showBuffer();
  }
  screenBuffer[pos] = 0;
  inputMode = 0;
  // remove the cursor
  lineDirty[curY] = 1;
  host_showBuffer();
  return &screenBuffer[startPos];
}

/*
 * host_getKey
 * non blocking keyboard read
 * returns 0 if no key pressed
 * 
 */
char host_getKey() {
  char c = inkeyChar;
  inkeyChar = 0;
  //if (c >= 32 && c <= 126)
  return c;
  //else return 0;
}

/*
 * host_ESCPressed
 * updates the status line and tests if the escape key is depressed
 * used during program execution to halt the interpreter
 * escape key is square root
 * 
 */
bool host_ESCPressed() {
  int c;

  host_printStatus(false);
  while (c = SRXEGetKey()) {
    inkeyChar = c;
    if (inkeyChar == KEY_ESC)
      return true;
  }
  return false;
}

void host_outputFreeMem(unsigned int val)
{
  host_newLine();
  host_outputInt(val);
  host_outputChar(' ');
  host_outputProgMemString(bytesFreeStr);
}

void host_saveProgram(bool autoexec) {
  EEPROM.write(0, autoexec ? MAGIC_AUTORUN_NUMBER : 0x00);
  EEPROM.write(1, sysPROGEND & 0xFF);
  EEPROM.write(2, (sysPROGEND >> 8) & 0xFF);
  for (int i = 0; i < sysPROGEND; i++)
    EEPROM.write(3 + i, mem[i]);
}

void host_loadProgram() {
  // skip the autorun byte
  sysPROGEND = EEPROM.read(1) | (EEPROM.read(2) << 8);
  for (int i = 0; i < sysPROGEND; i++)
    mem[i] = EEPROM.read(i + 3);
}

#if EXTERNAL_EEPROM
#include <I2cMaster.h>
extern TwiMaster rtc;

void writeExtEEPROM(unsigned int address, byte data)
{
  if (address % 32 == 0) host_click();
  rtc.start((EXTERNAL_EEPROM_ADDR << 1) | I2C_WRITE);
  rtc.write((int)(address >> 8));   // MSB
  rtc.write((int)(address & 0xFF)); // LSB
  rtc.write(data);
  rtc.stop();
  delay(5);
}

byte readExtEEPROM(unsigned int address)
{
  rtc.start((EXTERNAL_EEPROM_ADDR << 1) | I2C_WRITE);
  rtc.write((int)(address >> 8));   // MSB
  rtc.write((int)(address & 0xFF)); // LSB
  rtc.restart((EXTERNAL_EEPROM_ADDR << 1) | I2C_READ);
  byte b = rtc.read(true);
  rtc.stop();
  return b;
}

// get the EEPROM address of a file, or the end if fileName is null
unsigned int getExtEEPROMAddr(char *fileName) {
  unsigned int addr = 0;
  while (1) {
    unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr + 1) << 8);
    if (len == 0) break;

    if (fileName) {
      bool found = true;
      for (int i = 0; i <= strlen(fileName); i++) {
        if (fileName[i] != readExtEEPROM(addr + 2 + i)) {
          found = false;
          break;
        }
      }
      if (found) return addr;
    }
    addr += len;
  }
  return fileName ? EXTERNAL_EEPROM_SIZE : addr;
}

void host_directoryExtEEPROM() {
  unsigned int addr = 0;
  while (1) {
    unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr + 1) << 8);
    if (len == 0) break;
    int i = 0;
    while (1) {
      char ch = readExtEEPROM(addr + 2 + i);
      if (!ch) break;
      host_outputChar(readExtEEPROM(addr + 2 + i));
      i++;
    }
    addr += len;
    host_outputChar(' ');
  }
  host_outputFreeMem(EXTERNAL_EEPROM_SIZE - addr - 2);
}

bool host_removeExtEEPROM(char *fileName) {
  unsigned int addr = getExtEEPROMAddr(fileName);
  if (addr == EXTERNAL_EEPROM_SIZE) return false;
  unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr + 1) << 8);
  unsigned int last = getExtEEPROMAddr(NULL);
  unsigned int count = 2 + last - (addr + len);
  while (count--) {
    byte b = readExtEEPROM(addr + len);
    writeExtEEPROM(addr, b);
    addr++;
  }
  return true;
}

bool host_loadExtEEPROM(char *fileName) {
  unsigned int addr = getExtEEPROMAddr(fileName);
  if (addr == EXTERNAL_EEPROM_SIZE) return false;
  // skip filename
  addr += 2;
  while (readExtEEPROM(addr++)) ;
  sysPROGEND = readExtEEPROM(addr) | (readExtEEPROM(addr + 1) << 8);
  for (int i = 0; i < sysPROGEND; i++)
    mem[i] = readExtEEPROM(addr + 2 + i);
}

bool host_saveExtEEPROM(char *fileName) {
  unsigned int addr = getExtEEPROMAddr(fileName);
  if (addr != EXTERNAL_EEPROM_SIZE)
    host_removeExtEEPROM(fileName);
  addr = getExtEEPROMAddr(NULL);
  unsigned int fileNameLen = strlen(fileName);
  unsigned int len = 2 + fileNameLen + 1 + 2 + sysPROGEND;
  if ((long)EXTERNAL_EEPROM_SIZE - addr - len - 2 < 0)
    return false;
  // write overall length
  writeExtEEPROM(addr++, len & 0xFF);
  writeExtEEPROM(addr++, (len >> 8) & 0xFF);
  // write filename
  for (int i = 0; i < strlen(fileName); i++)
    writeExtEEPROM(addr++, fileName[i]);
  writeExtEEPROM(addr++, 0);
  // write length & program
  writeExtEEPROM(addr++, sysPROGEND & 0xFF);
  writeExtEEPROM(addr++, (sysPROGEND >> 8) & 0xFF);
  for (int i = 0; i < sysPROGEND; i++)
    writeExtEEPROM(addr++, mem[i]);
  // 0 length marks end
  writeExtEEPROM(addr++, 0);
  writeExtEEPROM(addr++, 0);
  return true;
}

#endif

// fdufnews 6/08/2020
#if SD_CARD
/*
   host_directorySD

   Print the directory listing of the SD Card on screen

   INPUT nothing

   OUTPUT nothing

*/
void host_directorySD(void) {
  if (!sdCardPresent) return;
  File dir, entry;
  dir = SD.open("/");  // open card root
  host_newLine(0);                      // initialize count of newlines
  do {
    entry =  dir.openNextFile();        // get the next file
    if (!entry) break;                  // if no more entry exit the loop
    if (!entry.isDirectory()) {         // if it is not a directory display info
      host_outputString(entry.name());  // display name of file
      host_moveCursor(16, curY);        // move cursor to align size information
      host_outputInt(entry.size());     // display size of file
      host_showBuffer();                // display the new line
      host_newLine(1);                  // scroll one line and wait if necessary
    }
    entry.close();                      // close file
  } while (true);
  dir.close();                          // close root dir
}


/*
   host_saveSD

   Save the current BASIC program to a file on SD Card
   If the file already exist it is deleted first

   INPUT
      filename : the name of the file is a 8+3 string

   OUTPUT
      true if succeeded

*/
boolean host_saveSD(char *filename) {
  File myFile;
  if (!sdCardPresent) return false;
  if (SD.exists(filename)) SD.remove(filename); // if file already exist then delete it
  myFile = SD.open(filename, FILE_WRITE); // open the file in write mode
  unsigned long count = myFile.write(mem, sysPROGEND); // write basic program buffer to sd card
  myFile.close();
  if (count == (unsigned long)sysPROGEND) {
    strcpy(currentFile, filename);
    return true;
  } else return false;
}

/*
   host_loadSD

   Load a file from the SD Card into memory as the current BASIC program

   INPUT
      filename : the name of the file is a 8+3 string

   OUTPUT
      true if succeeded

*/
boolean host_loadSD(char *filename) {
  File myFile;
  if (!sdCardPresent) return false;
  myFile = SD.open(filename, FILE_READ); // open the file in read mode
  if (!myFile) return false;
  unsigned long filesize = myFile.size();
  unsigned long count = myFile.read(mem, filesize); // copy file to basic program buffer
  myFile.close();
  sysPROGEND = (unsigned int)filesize;
  strcpy(currentFile, filename);
  return true;
}

/*
   host_removeSD

   Delete a file from the SD Card

   INPUT
      filename : the name of the file is a 8+3 string

   OUTPUT
      true if succeeded

*/
boolean host_removeSD(char *filename) {
  if (!sdCardPresent) return false;
  if (SD.exists(filename)) SD.remove(filename);
  return true;
}

/*
   host_unmountSD

   unmount the SD card in order to extract it

   INPUT nothing

   OUTPUT nothing
*/
void host_unmountSD(void) {
  SD.end();
  sdCardPresent = false;
  strcpy_P(currentFile, ko_card);
 // digitalWrite(SD_CARD_CS,HIGH);
  host_printStatus(true);
}

/*
   host_mountSD

   mount the SD card

   INPUT nothing

   OUTPUT nothing
*/
void host_mountSD(void) {
  if (sdCardPresent = SD.begin(SD_CARD_CS))   // true if an sd card is present
    strcpy_P(currentFile, ok_card);
  else
    strcpy_P(currentFile, ko_card);
  host_printStatus(true);
}


/*
    void host_openSD();
    void host_readSD();
    void host_writeSD();
*/

#endif
