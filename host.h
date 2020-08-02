#include <stdint.h>
#include <Arduino.h>
#include <SmartResponseXE.h>

// put 0 if you don't want to display the splashscreen
// very flash-mem hungry
#define SHOW_SPLASHSCREEN       1

// set display between two status updates
#define STATUS_RATE             (2000UL)

// that is size of display when using the small font
#define MAX_SCREEN_WIDTH        64
#define MAX_SCREEN_HEIGHT       16

#define KEY_DELETE 0x08
#define KEY_ENTER 0x0D
#define KEY_ESC 0x1B

// Put 1 if you use an external EEPROM on I2C bus
// NOTE
// ONLY ONE OF EXTERNAL_EEPROM OR  SD_CARD CAN BE ACTIVE AT THE SAME TIME ON A CONFIGURATION
#define EXTERNAL_EEPROM         0
#define EXTERNAL_EEPROM_ADDR    0x50    // I2C address (7 bits)
#define EXTERNAL_EEPROM_SIZE    32768   // only <=32k tested (64k might work?)

#define MAGIC_AUTORUN_NUMBER    0xFC

// fdufnews 28/07/2020
// Put 1 if using an SD card on SPI
// NOTE
// ONLY ONE OF EXTERNAL_EEPROM OR  SD_CARD CAN BE ACTIVE AT THE SAME TIME ON A CONFIGURATION
#define SD_CARD 1
#define SD_CARD_CS FLASH_CS

#if EXTERNAL_EEPROM && SD_CARD
#error "EXTERNAL_EEPROM and SD_CARD cannot be active at the same time"
#endif


void host_init(int buzzer_Pin, int led_Pin);
void host_sleep(long ms);
void host_digitalWrite(int pin, int state);
int host_digitalRead(int pin);
int host_analogRead(int pin);
void host_pinMode(int pin, int mode);
void host_click();
void host_startupTone();
void host_notone();
void host_tone(int note);
unsigned int host_BASICFreeMem(void);
unsigned int host_CFreeMem(void);
void host_setFont(unsigned char fontNum);
void host_setBatStat(void);
float host_getBatStat(void);
void host_printStatus(boolean now);
boolean host_setLineWrap(boolean wrap);
void host_cls();
void host_showBuffer();
void host_moveCursor(int x, int y);
void host_outputString(char *str);
void host_outputProgMemString(const char *str);
void host_outputChar(char c);
void host_outputFloat(float f);
char *host_floatToStr(float f, char *buf);
int host_outputInt(long val);
#if SHOW_SPLASHSCREEN
void host_splashscreen(void);
#endif
void host_newLine();
void host_goToSleep();
char *host_readLine();
char host_getKey();
bool host_ESCPressed();
void host_outputFreeMem(unsigned int val);
void host_saveProgram(bool autoexec);
void host_loadProgram();

#if EXTERNAL_EEPROM
#include <I2cMaster.h>
void writeExtEEPROM(unsigned int address, byte data);
void host_directoryExtEEPROM();
bool host_saveExtEEPROM(char *fileName);
bool host_loadExtEEPROM(char *fileName);
bool host_removeExtEEPROM(char *fileName);
#endif

#if SD_CARD
#include <SPI.h>
#include <SD.h>
void host_directorySD(void);
void host_saveSD(char *filename); 
void host_loadSD(char *filename);
void host_removeSD(char *filename);
/* 
 *  void host_openSD();
 *  void host_readSD();
 *  void host_writeSD();
 */
 
#endif
