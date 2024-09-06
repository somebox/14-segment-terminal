// 14 Segment 12-digit PCB Example
// uses IS31FL3733 16x12 LED driver
// pins SW1-12 are wired to the common cathode of each digit
// pins CS1-16 are wired to each segment of each digit



#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <sstream> // used for parsing and building strings
#include <iostream>
#include <string>
#include "is31fl3733.hpp"
#include "FourteenSegmentAlpha.h"
#include "Ticker.h"
#include "words.h"
#include "content.h"
#include "base64.h"

using namespace IS31FL3733;

// Arduino pin for the SDB line which is set high to enable the IS31FL3733 chip.
const uint8_t SDB_PIN = 4;
// Arduino pin for the IS13FL3733 interrupt pin.
const uint8_t INTB_PIN = 3;

// Function prototypes for the read and write functions defined later in the file.
uint8_t i2c_read_reg(const uint8_t i2c_addr, const uint8_t reg_addr, uint8_t *buffer, const uint8_t length);
uint8_t i2c_write_reg(const uint8_t i2c_addr, const uint8_t reg_addr, const uint8_t *buffer, const uint8_t count);

#define USER_BUTTON 0

// Timezone config
/* 
  Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  based on https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
*/
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Switzerland

// Wifi
WiFiManager wm;   // looking for credentials? don't need em! ... google "ESP32 WiFiManager"

// timer
Ticker timer;

// Time 
tm timeinfo;
time_t now;
int hour = 0;
int minute = 0;
int second = 0;

// Time, date, and tracking state
int t = 0;
int number = 0;
int animation=0;
String formattedDate;
String dayStamp;
long millis_offset=0;
int last_hour=0;

// Days of week. Day 1 = Sunday
String DoW[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
// Months
String Months[] { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

String getFormattedDate(){
  char time_output[30];
  strftime(time_output, 30, "%a  %d-%m-%y", &timeinfo);
  return String(time_output);
}

String getFormattedTime(){
  char time_output[30];
  strftime(time_output, 30, "%H:%M:%S", &timeinfo);
  return String(time_output);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(wm.getConfigPortalSSID());
}

// ---------------------------------------------------------------------------------------------

uint8_t i2c_read_reg(const uint8_t i2c_addr, const uint8_t reg_addr, uint8_t *buffer, const uint8_t length)
/**
 * @brief Read a buffer of data from the specified register.
 * 
 * @param i2c_addr I2C address of the device to read the data from.
 * @param reg_addr Address of the register to read from.
 * @param buffer Buffer to read the data into.
 * @param length Length of the buffer.
 * @return uint8_t 
 */
{
  Wire.beginTransmission(i2c_addr);
  Wire.write(reg_addr);
  Wire.endTransmission();
  byte bytesRead = Wire.requestFrom(i2c_addr, length);
  for (int i = 0; i < bytesRead && i < length; i++)
  {
    buffer[i] = Wire.read();
  }
  return bytesRead;
}
uint8_t i2c_write_reg(const uint8_t i2c_addr, const uint8_t reg_addr, const uint8_t *buffer, const uint8_t count)
/**
 * @brief Writes a buffer to the specified register. It is up to the caller to ensure the count of
 * bytes to write doesn't exceed 31, which is the Arduino's write buffer size (32) minus one byte for
 * the register address.
 * 
 * @param i2c_addr I2C address of the device to write the data to.
 * @param reg_addr Address of the register to write to.
 * @param buffer Pointer to an array of bytes to write.
 * @param count Number of bytes in the buffer.
 * @return uint8_t 0 if success, non-zero on error.
 */
{
  Wire.beginTransmission(i2c_addr);
  Wire.write(reg_addr);
  Wire.write(buffer, count);
  return Wire.endTransmission();
}

// Total LED driver boards
#define NUM_BOARDS 16
// Create a driver for each board with the address, and provide the I2C read and write functions.
IS31FL3733Driver drivers[NUM_BOARDS] = {
  IS31FL3733Driver(ADDR::GND, ADDR::SCL, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::GND, ADDR::SDA, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::GND, ADDR::VCC, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::GND, ADDR::GND, &i2c_read_reg, &i2c_write_reg), 
  IS31FL3733Driver(ADDR::VCC, ADDR::SCL, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::VCC, ADDR::SDA, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::VCC, ADDR::VCC, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::VCC, ADDR::GND, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SCL, ADDR::SCL, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SCL, ADDR::SDA, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SCL, ADDR::VCC, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SCL, ADDR::GND, &i2c_read_reg, &i2c_write_reg), 
  IS31FL3733Driver(ADDR::SDA, ADDR::SCL, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SDA, ADDR::SDA, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SDA, ADDR::VCC, &i2c_read_reg, &i2c_write_reg),
  IS31FL3733Driver(ADDR::SDA, ADDR::GND, &i2c_read_reg, &i2c_write_reg),
};

// each LED Driver board can drive 16x12 LEDs
#define WIDTH 16  // cs
#define HEIGHT 12 // sw
// each driver handles 12x 14-segment characters in a single row, 
//   mapping the 16x12 grid (with some unsed LEDS)
// each module joins 4 driver boards vertically, making a 4x12 character matrix
#define MODULE_HEIGHT 3
#define MODULE_WIDTH 16
// There are two modules, arranged to create a 3x32 character screen
#define NUM_MODULES 4
// Screen size (in characters) 
// currently using 2 modules, side-by-side
#define SCREEN_HEIGHT 6
#define SCREEN_WIDTH 32
// that's a total of 12*4*2 = 96 14-segment digits, or over 1400 LEDs!

uint8_t dig_buffer[NUM_BOARDS][WIDTH*HEIGHT+1];  // screen buffer for bulk updates




bool getNTPtime(int sec) {
  if (WiFi.isConnected()) {
    bool timeout = false;
    bool date_is_valid = false;
    long start = millis();

    Serial.println(" updating:");
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_INFO, 1);

    do {
      timeout = (millis() - start) > (1000 * sec);
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(" . ");
      date_is_valid = timeinfo.tm_year > (2016 - 1900);
      delay(100);
      
      // show animation
      static int dp_pos = 0;
      // draw_segment_pattern(drivers[0], dp_pos++, 0b10000000, random(3)*100+55);
      dp_pos %= 12;

    } while (!timeout && !date_is_valid);
    
    if (!date_is_valid){
      Serial.println("Error: Invalid date received!");
      Serial.println(timeinfo.tm_year);
      return false;  // the NTP call was not successful
    } else if (timeout) {
      Serial.println("Error: Timeout while trying to update the current time with NTP");
      return false;
    } else {
      Serial.println("\n[ok] time updated: ");
      Serial.print("System time is now:");
      Serial.println(getFormattedTime());
      Serial.println(getFormattedDate());
      return true;
    }
  } else {
    Serial.println("Error: Update time failed, no WiFi connection!");
    return false;
  }
}

void ConnectToWifi(){
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wm.setAPCallback(configModeCallback);

  //wm.resetSettings();   // uncomment to force a reset
  bool wifi_connected = wm.autoConnect("ESP32_RGB_Ticker");
  int t=0;
  if (wifi_connected){
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println("db");

    delay(1000);

    Serial.println("getting current time...");
    
    if (getNTPtime(10)) {  // wait up to 10sec to sync
      Serial.println("Time sync complete");
    } else {
      Serial.println("Error: NTP time update failed!");
    }
  } else {
    Serial.println("ERROR: WiFi connect failure");
    // update fastled display with error message
  }
}


// ---------------------------------------------------------------------------------------------

int dots[4][4];

struct pixel {
  uint8_t cs; // X
  uint8_t sw; // Y
  float level;
  float speed;
};
pixel pixels[WIDTH*HEIGHT];



void draw_buffer(){
  for (int b=0; b<NUM_BOARDS; b++){
    drivers[b].SetPWM(dig_buffer[b]);
  }
}

void clear_buffer(){
  for (int b=0; b<NUM_BOARDS; b++){
    std::fill_n(dig_buffer[b], WIDTH*HEIGHT, 0);
  }
}

void dim_buffer(uint8_t amount){
  for (int b = 0; b < NUM_BOARDS; b++) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      if (dig_buffer[b][i] > 35){
        dig_buffer[b][i] *= 0.85; // move faster over bright levels
      }
      if (amount > dig_buffer[b][i]) {
        dig_buffer[b][i] = 0;
      } else {
        dig_buffer[b][i] -= amount;
      }
    }
  }
}

#define MAX_BALLS 3
void metaballs(){
  static struct ball {
    float x;
    float y;
    float vx;
    float vy;
    float r;
  } balls[MAX_BALLS] = {
    { 5.0, 0.0, 0.1, 0.1, 1.5 },
    { 3.0, 2.0, -0.1, 0.1, 1.3 },
    { 7.0, 1.0, 0.1, -0.1, 2.7 }
  };
  // draw balls in buffer

}



void draw_segment_pattern(IS31FL3733Driver &driver, uint8_t pos, uint16_t byte_value, uint8_t level=255){
  // Sets value of a single 14-seg digit in the display.
  // The position (pos) is from left to right (0..11).
  // The byte_value is a byte value that defines which segments are turned on.

  // decode byte value into segments
  int sw = pos;

  for (int i=0; i<WIDTH; i++) {   
    if (byte_value & (1<<i)) {
      driver.SetLEDSinglePWM(i, sw, level); // turn on segment      
    } else {
      driver.SetLEDSinglePWM(i, sw, 0); // turn off segment
    }
  }
}

void draw_segment_pattern(uint8_t row, uint8_t col, uint16_t byte_value, uint8_t level=255){
  row = constrain(row, 0, SCREEN_HEIGHT-1);
  col = constrain(col, 0, SCREEN_WIDTH-1);
  // first we find which module to write to
  int board = col/4 + row/3*8; // which of the four boards, laid out 2x2, 4 drivers per board
  // then calculate the digit position in the module (0..11), each module is 4x3
  int pos = (col % 4) + (row % 3)*4;

  // draw_segment_pattern(drivers[board], pos, byte_value, level);
  for (int bit = 0; bit < WIDTH; bit++) {
    uint8_t value = (byte_value & (1 << (bit))) ? level : 0;
    dig_buffer[board][pos*WIDTH+bit] = value;
  }
}

void random_animation(){
 // startup fadout animation
  for (int level=60; level>=-60; level-=1){
    for (int d=0; d<NUM_BOARDS; d++) {
      for (int pos=0; pos<12; pos++){
        if (random(255)==0){
          draw_segment_pattern(drivers[d], pos, random(UINT16_MAX), 5+abs(level));
        }
      }
    }
    // delay(50);
  }
}

/*
Layout:
- Virtual screen: 32x6
- 4 modules
- each module is 16x3, with 4 boards
- each board shows 12 characters, arranged 4x3

┌Module 0 ───────────────────┐ ┌Module 1 ───────────────────┐
│┌─────┐┌─────┐┌─────┐┌─────┐│ │┌─────┐┌─────┐┌─────┐┌─────┐│
││Board││Board││Board││Board││ ││Board││Board││Board││Board││
││ 4x3 ││ 4x3 ││ 4x3 ││ 4x3 ││ ││ 4x3 ││ 4x3 ││ 4x3 ││ 4x3 ││
││     ││     ││     ││     ││ ││     ││     ││     ││     ││
│└─────┘└─────┘└─────┘└─────┘│ │└─────┘└─────┘└─────┘└─────┘│
└────────────────────────────┘ └────────────────────────────┘
┌Module 2 ───────────────────┐ ┌Module 3 ───────────────────┐
│┌─────┐┌─────┐┌─────┐┌─────┐│ │┌─────┐┌─────┐┌─────┐┌─────┐│
││Board││Board││Board││Board││ ││Board││Board││Board││Board││
││ 4x3 ││ 4x3 ││ 4x3 ││ 4x3 ││ ││ 4x3 ││ 4x3 ││ 4x3 ││ 4x3 ││
││     ││     ││     ││     ││ ││     ││     ││     ││     ││
│└─────┘└─────┘└─────┘└─────┘│ │└─────┘└─────┘└─────┘└─────┘│
└────────────────────────────┘ └────────────────────────────┘

*/

// draws a single character at position row, col on the display
// row can be 0-5, col can be 0-31
// level is the brightness of the character, 0-255
// each module has 12 characters, arranged in a 4x3 grid
void draw_character(uint16_t ascii_code, int row, int col, uint8_t level, bool show_dot=false){
  // get the bit pattern for the character
  uint16_t pattern = alphafonttable[constrain(ascii_code, 0, 126)];
  if (show_dot || ascii_code==33 || ascii_code==46 || ascii_code==63){   // draw period, decimal, and question with a dot
    pattern |= 1 << 7;
  }
  row = constrain(row, 0, SCREEN_HEIGHT-1);
  col = constrain(col, 0, SCREEN_WIDTH-1);
  // first we find which module to write to
  int board = col/4 + row/3*8; // which of the four boards, laid out 2x2, 4 drivers per board
  // then calculate the digit position in the module (0..11), each module is 4x3
  int pos = (col % 4) + (row % 3)*4;
  
  if (board > NUM_BOARDS){
    Serial.printf("error, board is %d\n", board);
    return;
  }
  // Serial.printf("char: '%c'\n row: %d col %d\n board: %d pos: %d\n", ascii_code, row, col, board, pos);

  // then fill the buffer with all the segement levels for the character
  for (int bit = 0; bit < WIDTH; bit++) {
    uint8_t value = (pattern & (1 << (bit))) ? level : 0;
    dig_buffer[board][pos*WIDTH+bit] = value;
  }
}


// word wrap and pad spaces to make long sentence fit on the display
// https://chat.openai.com/share/22465504-3300-4ccd-8cb9-29fd031747d7

String wrap_string(const String& source_string) {
    String s = source_string;
    String result = "";
    String currentLine = "";
    int spaceLeft = SCREEN_WIDTH;

    // pad string with spaces so it's exactly 48 characters long
    String padded = s;
    while (padded.length() < 48) {
        padded += ' ';
    }
    s = padded;

    // Splitting the input string into words
    int startPos = 0;
    while (startPos < s.length()) {
        int spacePos = s.indexOf(' ', startPos);
        if (spacePos == -1) {
            spacePos = s.length();
        }

        String word = s.substring(startPos, spacePos);
        int wordLength = word.length();

        // Adjust the condition to move word to next line when word length plus a space equals space left
        if (wordLength + (currentLine.length() > 0 ? 1 : 0) > spaceLeft) {
            // Pad the current line with spaces and start a new line
            while (currentLine.length() < SCREEN_WIDTH) {
                currentLine += ' ';
            }
            result += currentLine;
            currentLine = word;
            spaceLeft = SCREEN_WIDTH - wordLength;
        } else {
            // Add word to the current line
            if (currentLine.length() > 0) {
                currentLine += ' ';
            }
            currentLine += word;
            spaceLeft -= (wordLength + (currentLine.length() > 0 ? 1 : 0));
        }

        startPos = spacePos + 1;
    }

    // Pad and add the last line
    while (currentLine.length() < SCREEN_WIDTH) {
        currentLine += ' ';
    }
    result += currentLine;

    return result;
}

void draw_string(String str, int row, int col, int level=100){
  // draw a string on the display
  if (!rand) Serial.printf("draw_string('%s', %d, %d, %d)\n", str.c_str(), row, col, level);
  int position = 0;
  for (char c : str){
    bool show_dot = (str[position+1] == '.');
    if (c == '.'){
      position++;
      continue;
    }

    draw_character(c, row, col, level, show_dot);

    col++;
    if (col == SCREEN_WIDTH) {  // crude wrapping
      row++;
      col = 0;
    }
    if (row == SCREEN_HEIGHT) { break; }

    position++;
  }
}

enum ButtonPressStatus {
  NOT_PRESSED,
  SHORT_PRESS,
  LONG_PRESS
};
ButtonPressStatus button_pressed() {
  // handle button press for clock_mode change
  if (digitalRead(USER_BUTTON) == LOW) {
    Serial.print("Button pressed ... ");

    unsigned long startTime = millis();
    while (digitalRead(USER_BUTTON) == LOW) {
      dim_buffer(5);
      draw_buffer();
    }
    unsigned long endTime = millis();
    unsigned long hold_time = endTime - startTime;
    Serial.printf("released. (%0.1f sec)\n", hold_time/1000.0);
    if (hold_time > 1000) {
      return LONG_PRESS;
    } else {
      return SHORT_PRESS;
    }
  } else {
    return NOT_PRESSED;
  } 
}

// Function to join words with spaces and ensure the sentence is exactly 32 characters
std::string joinWordsWithPadding(const std::vector<std::string>& words) {
    std::string sentence = "";
    for (const auto& word : words) {
        sentence += word + " ";
    }
    sentence.pop_back(); // Remove the trailing space
    // Pad with spaces to ensure the sentence is exactly 32 characters
    // while (sentence.length() < 32) {
    //     sentence += " ";
    // }
    sentence += ". ";
    return sentence;
}

void generate_sentences(){
   // Define patterns for sentence generation
    std::vector<std::string> patterns = {
        "properNoun adverb verb adjective noun",
        "adjective verb noun verbPhrase pronoun adjective",
        "properNoun verb adverb noun",
        "adjective noun verb adverb",
        "properNoun verbPhrase noun",
        "noun pronoun noun verb pronoun"
    };

    std::vector<std::string> chosenWords;
    std::string sentence = "";

    int iterations=0;
    do {
        iterations++;
        //Serial.printf("iteration %d\n", iterations);

        // Clear previous words
        chosenWords.clear();

        // Select a random pattern
        std::string pattern = patterns[random(0, patterns.size() - 1)];

        // Split the pattern and replace placeholders with words from corresponding vectors
        std::vector<std::string> words;
        std::string patternCopy = pattern;
        std::string delimiter = " ";
        size_t pos = 0;
        std::string token;
        while ((pos = patternCopy.find(delimiter)) != std::string::npos) {
          //Serial.printf(" pattern: '%s'\n", patternCopy.c_str());
          token = patternCopy.substr(0, pos);
          words.push_back(token);
          patternCopy.erase(0, pos + delimiter.length());
        }
        words.push_back(patternCopy);

        for (const auto& word_type : words) {
          //Serial.printf(" generating pattern for '%s'\n", word_type.c_str());
          if (word_type == "properNoun") {
              chosenWords.push_back(properNouns[random(0, properNouns.size() - 1)]);
          } else if (word_type == "adverb") {
              chosenWords.push_back(adverbs[random(0, adverbs.size() - 1)]);
          } else if (word_type == "verb") {
              chosenWords.push_back(verbs[random(0, verbs.size() - 1)]);
          } else if (word_type == "adjective") {
              chosenWords.push_back(adjectives[random(0, adjectives.size() - 1)]);
          } else if (word_type == "noun") {
              chosenWords.push_back(nouns[random(0, nouns.size() - 1)]);
          } else if (word_type == "verbPhrase") {
              chosenWords.push_back(verbPhrases[random(0, verbPhrases.size() - 1)]);
          } else if (word_type == "pronoun") {
              chosenWords.push_back(pronouns[random(0, pronouns.size() - 1)]);
          }
        }
        // Join words to form a sentence
        sentence += joinWordsWithPadding(chosenWords);
    } while (sentence.length() < SCREEN_WIDTH*SCREEN_HEIGHT);
    // Capitalize the sentence
    // std::transform(sentence.begin(), sentence.end(), sentence.begin(), ::toupper);
    draw_string(String(sentence.c_str()), 0, 0);
    Serial.printf("generated sentence: '%s'\n", sentence.c_str());
}


struct Drip{
  uint8_t row=0;
  uint8_t col=0;
  uint8_t tick=0;
  uint8_t speed=1;
  uint8_t sleep=0;
  bool active=false;
  uint16_t symbol='.';
};
#define MAX_DRIPS 30
Drip drips[MAX_DRIPS];
uint8_t lanes[SCREEN_WIDTH]; // keep track of columns that are already in use so we don't run over too many characters

void matrix(){
  static long tick=0;
    // init Particles
  for (int i=0; i<MAX_DRIPS; i++){
    if (tick==0){
      drips[i] = Drip(); // init on first run
      for (int j=0; j<SCREEN_WIDTH; j++){
        lanes[j] = 0;
      }
    }
    Drip *drip = &drips[i];
    if (!drip->active){
      if (drip->sleep > 0){
        // keep sleeping
        drip->sleep--;
      } else {
        // wake up
        drip->active = true;
        drip->speed = random(2,8);  // lower is faster
        drip->row = 0;
        // select a new column to start, avoiding used ones
        drip->col = random(SCREEN_WIDTH);
        if (lanes[drip->col]==0){
          // ok to use
          lanes[drip->col]=1;
        } else {
          // search for a new col
          for (int j=0; j<SCREEN_WIDTH; j++){
            if (j==i) continue;
            if (lanes[j]==0){
              lanes[j]=1;
              drip->col = j;
              break;
            }
          }
        }
      }
    }
    drip->tick++;
    if (drip->active){
      if (drip->row > SCREEN_HEIGHT){
        // deactivate drip
        drip->active = false;
        drip->sleep = random(5,20);  // how long to pause after
        lanes[drip->col] = 0; // free it up to be used again
        drip->tick = 0;
        drip->speed = random(30,100); // delay after leaving the screen
      } else {
        // render drip
        if (drip->tick > drip->speed){
          drip->tick=0;

          if (random(4)==0){
            // generate a random character            
            uint16_t pattern = 0;
            int bits = random(2,5);
            for (int r=0; r<bits; r++){
              pattern |= (1 << random(16));
            }
            drip->symbol = pattern;
          }

          draw_segment_pattern(drip->row, drip->col, drip->symbol, random(150,230));
          drip->row++;
        }
      }
    }          
    // Serial.printf("drip %d %s %d\n",i,drip->active?"active":"sleep",drip->tick);
  }
  
  tick++;
}


void random_characters(int num, int level=100) {
  for (int i=0; i<num; i++) {
    uint16_t letter = random(32, 127);
    // draw character
    uint16_t pattern = alphafonttable[letter];
    if (letter==33 || letter==46 || letter==63){   // draw pariod, deicmal, and question with a dot
      pattern |= 1 << 7;
    }
    // create random movement across grid using row and col
    static int r = 0;
    static int c = 0;
    static int dir = 1;
    static int dir2 = 1;
    r += dir;
    c += dir2;
    if (r > SCREEN_HEIGHT) { r = SCREEN_HEIGHT-1; dir = -1; }
    if (r < 0) { r = 0; dir = 1; }
    if (c > SCREEN_WIDTH) { c = SCREEN_WIDTH-1; dir2 = -1; }
    if (c < 0) { c = 0; dir2 = 1; }
    if (random(2)==0) { r = random(SCREEN_HEIGHT); c = random(SCREEN_WIDTH); } 
    draw_segment_pattern(r, c, pattern, level);
  }
}

bool test_all_segments(uint8_t level=30, long speed=3){
  static int test_mode = 0;
  static uint16_t segment = 0;
  for (int board=0; board<NUM_BOARDS; board++){
    for (int pos=0; pos<12; pos++){
      uint16_t pattern=0;
      for (uint16_t b=0; b<16; b++){
        if (test_mode==1 && b != segment) continue;
        pattern |= 1 << b;
        draw_segment_pattern(drivers[board], pos, pattern, level);
        if (test_mode == 0) delay(speed);
        ButtonPressStatus result = button_pressed();
        if (result == LONG_PRESS){
          return false;
        } else if (result == SHORT_PRESS){
          test_mode = (test_mode+1)%2;
        }
        
      }
    }
  }
  segment = (segment+1)%16;
  return true;
}


static float counter=1; // tracks time for animation
static float wave1_period = 0.1;
static float wave2_period = 0.01;
static float wave3_period = 2;
static long frame = 1;
static float scale = 1000.0;

void spirals(){
  // loop through rows and cols (SCREEN_WIDTH x SCREEN_HEIGHT) and
  // call draw_character for each element
  
  for (int col=0; col<SCREEN_WIDTH; col++){
    for (int row=0; row<SCREEN_HEIGHT; row++){  
      // calculate brightness of each segment based on time and position, using the three wave periods
      float level = cos((col+counter)/(counter*5)+wave1_period) * sin((row+counter)/20.0+wave2_period);
     //level += cos(frame/wave2_period) * cos(row/wave2_period)+sin(col/wave2_period);
      // level += cos(frame/wave3_period) * cos(row/wave3_period);
      //level /= 2.0;
      // draw character at position row, col, with brightness level
      draw_character(map(level*100, -100.0, 100.0, 7, 15), row, col, map(sin(cos(col)+cos(row)+wave3_period)*1000, -1000, 1000, 0, 255));            
    }
  }
  // slowly adjust frequencies of the three waves
  wave1_period = wave1_period + 0.001;
  wave2_period = wave2_period + 0.002;
  wave3_period = wave3_period + 0.03;

  // increment frame counter
  counter += sin((millis()%120000)/1000.0)/500.0;
  counter = fmod(counter, 2*PI/10);
}

#define TIMER_PERIOD 5
static int last_frames = 0;
void timerStatusMessage(){
  Serial.println("-----");
  // Serial.printf("counter: %f\n", counter);
  // Serial.printf("wave1_period: %f\n", wave1_period);
  // Serial.printf("wave2_period: %f\n", wave2_period);
  // Serial.printf("wave3_period: %f\n", wave3_period);
  Serial.printf("frame: %d\n", frame);
  float fps = (frame - last_frames) / TIMER_PERIOD;
  Serial.printf("fps: %f\n", fps);
  last_frames = frame;

}
#define RANDOM_RANGE 2000
void randomize_dots(){
  for (int i=0; i<4; i++){
    for (int j=0; j<4; j++){
      dots[i][j] = random(RANDOM_RANGE)+1;
    }
  }
}


int current_board = 2;
void setup()
{
  // // init pixel stucts
  // for (int i=0; i < WIDTH*HEIGHT; i++) {
  //   pixels[i].cs = i % WIDTH;
  //   pixels[i].sw = i / WIDTH;
  //   pixels[i].level = 0;
  //   pixels[i].speed = random(20, 200) / 200.0;    
  // }

  // clear_buffer();

  // Initialize serial and I2C.
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(800000); // use 800 kHz I2C

  // Wait for serial to connect so the debug output can be seen.
  while (!Serial); // Waiting for Serial Monitor
  Serial.println("Waiting to begin.");
  delay(500);

  Serial.print("I2C speed is ");
  Serial.println(Wire.getClock());

  Serial.println("Initializing LED Drivers");
  for (int d=0; d<NUM_BOARDS; d++) {
    //if (d != 2) continue; // only initialize the 3rd board
    Serial.print("\nIS31FL3733B driver init at address 0x");
    Serial.println(drivers[d].GetI2CAddress(), HEX);  
    drivers[d].Init();

    Serial.println(" -> Setting global current control");
    drivers[d].SetGCC(100);

    Serial.println(" -> Setting PWM state for all LEDs to half power");
    drivers[d].SetLEDMatrixPWM(140);

    Serial.println(" -> Setting state of all LEDs to OFF");
    drivers[d].SetLEDMatrixState(LED_STATE::ON);
    drivers[d].SetLEDMatrixPWM(50); // set brightness
  }

  //timer.attach(TIMER_PERIOD, timerStatusMessage);

 // randomize_dots();

  delay(100);

  //random_animation();
  //test_all_segments(1, 0); 
  clear_buffer();
  draw_buffer();
  // Serial.println("Connecting to WIFI...");
  // ConnectToWifi(); 
  // drivers[0].SetLEDMatrixState(LED_STATE::ON);

}

void scroll_all_characters(){
  static int ch = 32;  
  // clear_buffer(); draw_buffer();
  for (int pos=0; pos<12; pos++){
    int ascii;
    if (ch+pos >= 127) { ascii = 32; } else { ascii = ch+pos; };
    for (int d=0; d<NUM_BOARDS; d++){
      draw_segment_pattern(drivers[d], pos, alphafonttable[ascii], 128);
    }
  }
  ch++;  
  if (ch > 127-12) { ch = 32; }
  
}

void design_CLI(){

  // wait for input from serial
  while (Serial.available() == 0) {}
  
  // read input string, as a bit pattern of 1s and 0s
  String input = Serial.readString();
  if (input.startsWith("0b")) {
    input = input.substring(2);
  }
  int bit_count = 0;
  
  for (int i=0; i<input.length(); i++){
    if (input[i] == '0' || input[i] == '1') {
      bit_count++;
    }
  }
  if (bit_count != 16) {
    Serial.println("invalid input! must be a 16-bit binary number");
  } else {
    uint16_t pattern = strtol(input.c_str(), nullptr, 2);
    Serial.print("input: ");
    Serial.print(input);
    Serial.print(" -> ");
    Serial.println(pattern, BIN);

    // set bit pattern of all characters on display
    for (int i=0; i<12; i++){
      draw_segment_pattern(drivers[0], i, pattern, 255);
    }
  }  
}


std::string inputBuffer;

void readSerialInput() {
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();

    if (incomingByte == '\r' || incomingByte == '\n') {
      // End of command
      Serial.println(); // Move to the next line
      inputBuffer = ""; // reset
    } else if (incomingByte == '\b' || incomingByte == 127) {
      // Handle backspace or delete
      if (!inputBuffer.empty()) {
        inputBuffer.pop_back();        
        Serial.print("\b \b"); // Move cursor back, print space, move cursor back again
      }
    } else {
      // Store the character in the buffer
      if (inputBuffer.length() < SCREEN_WIDTH*SCREEN_HEIGHT) {
        inputBuffer += incomingByte;
        Serial.print(incomingByte); // Echo the character back to the terminal
      }
    }
  }
}

void text_cli(){
  readSerialInput();
  clear_buffer();
  if (inputBuffer.empty()){
    draw_string(" // TERMINAL - begin typing...", 0, 0, 30);
  } else {
    draw_string(inputBuffer.c_str(), 0, 0, 100);
  }
  // draw cursor
  int cursor_level = 100+sin(millis()/150.0)*100;
  int len = inputBuffer.size() - std::count(inputBuffer.begin(), inputBuffer.end(), '.');
  int col=len%SCREEN_WIDTH;
  int row=len/SCREEN_WIDTH;
  draw_character('_', row, col, abs(cursor_level));

  draw_buffer();
}


#define BUFFER_SIZE 2000
#define DEBUG_SER false

std::string getChunk(std::string& input) {
    size_t pos = input.find('$');  // Find the position of the '$' character
    std::string chunk = "";

    if (pos == std::string::npos) {
        // If no '$' is found, return the entire input and clear the input string
        // std::string chunk = input;
        // input.clear();
        //Serial.println("no delimeter");
        return chunk;
    }

    // Extract the substring before the '$'
    chunk = input.substr(0, pos);
    // filter out unwanted characters
    std::string output;
    output.reserve(chunk.size()); // optional, avoids buffer reallocations in the loop
    for(size_t i = 0; i < chunk.size(); ++i){
      char c = chunk[i];
      if ((isalnum(c) || (c == '+') || (c == '/')) || (c == '=')) output += c;
    }
    chunk.swap(output);
    if (DEBUG_SER) Serial.printf("pos (%d) chunk (%d): %s\n", 
              pos, 
              chunk.length(), 
              chunk.c_str()
            );

    // Erase the chunk and the '$' from the input string
    input.erase(0, pos + 1);

    return chunk;
}

std::string input="";
std::string result;
std::string base64="";
// int fd = open(,  O_RDWR | O_NOCTTY);

void terminal_mode(){
  if (Serial.available() > 0) {
    //input = Serial.readString().c_str();
    //char c = Serial.read();
    //if (c == 10) break;
    //input.push_back(c);
    // std::string chunk;
    // std::getline(std::cin, chunk);
    // input += chunk;

    // input.append(Serial.readString().c_str());
    //char buffer[1024];
    //int bytes_read = read(STDIN_FILENO, buffer, sizeof buffer);
    // int bytes_read = Serial.read(buffer, sizeof buffer);
    //if (bytes_read > 0){
      //for (size_t i=0; i<bytes_read; i++){
        //u_char c=buffer[i];
      uint16_t count = 0;
      while (Serial.available() > 0){
        u_char c = Serial.read();
        if ((isalnum(c) || (c == '+') || (c == '/')) || (c == '=') || (c == '$')){
          input += c;
          count++;
        } 
      }
      if (DEBUG_SER) Serial.printf("added %d chars to input (now %d):\n", count, input.length()); 
      if (DEBUG_SER) Serial.printf("%s\n", input.c_str());
    //}
    
    //Serial.printf("Input (%d):\n%s\n", input.length(), input.c_str());
  }
  if (!input.empty()){
    base64 = getChunk(input);
  }

  if (!base64.empty()){
    result = base64_decode(base64);
    if (result.empty()){
      if (DEBUG_SER) Serial.printf("base64 decode failed!\n");
      base64.clear();
      input.clear();
    } else {
      if (DEBUG_SER) Serial.printf("Base64 decoded (%d):\n", result.length());
      // clear_buffer();
      for (int r=0; r<SCREEN_HEIGHT; r++){
        for (int c=0; c<SCREEN_WIDTH; c++){
          int pos = r*SCREEN_WIDTH+c;
          if (pos < result.length()){
            draw_character(result[pos], r, c, 120, false);
            if (DEBUG_SER) Serial.printf("%lc", result[pos]);
          }
        }
        //
      }
      if (DEBUG_SER) Serial.println("\n");
      draw_buffer();
      base64.clear();
    }
  }

}

// static long cycle = 1.0;
// void wave_pattern(){
//   for (int col=0; col<SCREEN_WIDTH; col++){
//     for (int row=0; row<SCREEN_HEIGHT; row++){  
//       // set each segement to a level that follows a sine wave based on the time and position
//       float time_mod = fmod((millis()%4000)/636.20, 2*PI);
//       float digit_mod = fmod(cos(time_mod)*sin(row/cycle)+cos(col/cycle), 20.0);
//       //dig_buffer[b][row*col] = map((sin(time_mod) * cos(digit_mod)), -1, 1, 0, 255);
//       draw_character(map(digit_mod*10, 0, cycle*10, 1, 127 ), row, col, map((sin(time_mod) * cos(digit_mod)+1.0), -1, 1, 0, 255));
//     }
//   }
//   cycle = fmod(cycle + 0.5, 100.0);
// }


void show_message(int message_number){ 
  byte spin[] = {6,9,10,11,8,12,13,14};
  
  int row = 0;
  int col = 0;
  String msg = msgs[message_number];
  msg.toUpperCase();
  // if (msg.length() < 48) { // pad space to make it exactly 48 chars
  //   msg += String(48 - msg.length(), ' ');
  // }
  msg = msg.substring(0, 47);
  Serial.println(msg);
  Serial.println(msg.length());
  
  for (int pos=0; pos<msg.length(); pos++){
    if (row >= SCREEN_HEIGHT) { break; } 
    if (char(msg[pos]) == '.') { col++; continue; }
    // animate before drawing character
    for (byte b=0; b<8; b++){
      draw_segment_pattern(row, col, 1 << spin[b], 220);
      draw_buffer();
      delay(1+random(3));
    }
    // draw character
    uint16_t pattern = alphafonttable[msg[pos]];
    if (pos > 0 && pos < msg.length()-1 && msg[pos+1]=='.'){
      pattern |= 1 << 7;
    }
    draw_segment_pattern(row, col, pattern, 100);
    draw_buffer();

    col++;
    if (col >= SCREEN_WIDTH) {
      col = 0;
      row++;
    }
  }
  // cursor
  for (int i=0; i<5; i++){
    draw_segment_pattern(row, col, 1 << 3, 200);
    draw_buffer();
    delay(500);
    draw_segment_pattern(row, col, 0, 128);
    draw_buffer();
    delay(500);      
  }
  // erase
  for (int i=SCREEN_WIDTH*SCREEN_HEIGHT; i>=0; i--){
    draw_segment_pattern(i/SCREEN_WIDTH, i%SCREEN_WIDTH, 1 << 3, 200);
    draw_segment_pattern((i+1)/SCREEN_WIDTH, (i+1)%SCREEN_WIDTH, 0, 128);
    draw_buffer();
    delay(30+(random(5)==0 ? random(4)*30 : 0));
  }

}

int message_number = 0;
int last_quote = 0;

void test_module_order(){
  Serial.println("testing module order");
  for (int board=0; board<NUM_BOARDS; board++){
    Serial.printf(" board %d\n", board);
    for (int sw=0; sw<12; sw++){
      for (int cs=0; cs<16; cs++){
        drivers[board].SetLEDSinglePWM(cs, sw, 255); // turn on segment      
        delay(50);
      }
    }
    delay(1000);
  }
}

/* --------------- main loop --------------------  */

uint8_t current_mode = 0;
std::vector<std::string> firmware_modes = {
  {"terminal mode", "random characters", "words", "matrix", "board test"}
};

void next_mode(){
  current_mode = (current_mode + 1) % firmware_modes.size();
  Serial.printf("Changed mode to: %s\n", firmware_modes[current_mode].c_str());
  clear_buffer();
}

void loop(){
  // static long last_millis = 0;
  if (button_pressed()){
    next_mode();
  }

  switch (current_mode){
    case 0:
      terminal_mode();
      break;
    case 1:
      // random characters appear and fade out
      dim_buffer(4);
      random_characters(random(3,20), random(190,250));
      draw_buffer();
      break;
    case 2:
      // randomly-generated sentences
      clear_buffer();
      generate_sentences();
      draw_buffer();
      for (int t=0; t<10000; t++){
        ButtonPressStatus result = button_pressed();
        if (result == SHORT_PRESS) {
          break; // exit an re-generate sentence on next pass
        } else if (result == LONG_PRESS) {
          next_mode();
          break;
        } else {
          delay(1);
        }
      }
      break;
    case 3:
      // alien symbols fall and leave trails that fade out
      dim_buffer(3);
      matrix();
      draw_buffer();
      break;
    case 4:
      // check each segement of every module, to check for connection problems
      clear_buffer();
      draw_buffer();
      bool completed = test_all_segments();
      if (!completed) next_mode();
      delay(1000);
      break;
  }

  // update time
  //  time(&now);
  //  localtime_r(&now, &timeinfo);
  // // static int level = 190;
  // Serial.println(getFormattedTime());
  
  // design_CLI();
  
  // while (message_number == last_quote) {
  //   message_number = random(NUM_QUOTES);
  // }
  // String quote = wrap_string(quotes[message_number]);
  // // capitalize q
  // quote.toUpperCase();
  // // clear_buffer();
  // for (int i=0; i<80; i++){
  //   draw_string(quote, 0, 0, 80, true);
  //   draw_buffer();
  //   delay(3);
  // }
  // clear_buffer(); 
  // draw_string(quote, 0, 0, 80);
  // draw_buffer();
  // delay(8000);
  // last_quote = message_number;
  // message_number = (message_number + 1) % NUM_QUOTES;

  // test_module_order();
  // spirals();
  // wave_pattern();
  // scroll_all_characters();

  frame++;
}