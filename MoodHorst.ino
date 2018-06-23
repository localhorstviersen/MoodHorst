#include <FastLED.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ethernet.h>
#include <NTPClient.h>
#include <EthernetUdp.h>

// thankfully based on the code examples of:
// -Mark Kriegsman, December 2014

FASTLED_USING_NAMESPACE                                         // Initialize WS2812B matrix and required config
#define DATA_PIN    5                                           // Pin used for communicating with the WS812B LEDs
#define AMBIENCE_LIGHT_PIN  0                                   // For input from photocell (or potentiometer) to dim the leds by external input
int reading = 0;                                                
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    256
CRGB leds[NUM_LEDS];
#define BASE_BRIGHTNESS     160
#define FRAMES_PER_SECOND  240
int startFlag = 0;
unsigned long startTime;
unsigned long previousTime;

bool on;
int buzzer = 4;

CRGBPalette16 currentPalette = PartyColors_p;                   // Palette definitions
CRGBPalette16 targetPalette = PartyColors_p;
TBlendType    currentBlending = LINEARBLEND;                    // NOBLEND or LINEARBLEND

LiquidCrystal lcd(7, 8, 0, 1, 2, 3);                            // Initialize LCD

// Ethernet & doorstatus
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };            // physical mac address
char serverName[] = "10.234.2.103";
EthernetClient client;
String doorstatus;
String readString;
int x = 0;                                                      // for counting line feeds
char lf = 10;                                                   // line feed character
int i = 0;
EthernetUDP Udp;
NTPClient timeClient(Udp, 7200);

String getDoorstatus()                                          // client function to send/receive GET request data.
{
  x = 0;
  i++;
  readString = ("");
  if (client.connect(serverName, 8090)) {                       // starts client connection, checks for connection
    client.println("GET /doorstatus/status.txt HTTP/1.1");      // download text
    client.println("Host: 10.234.2.103");
    client.println("Connection: close");                        // close 1.1 persistent connection
    client.println();                                           // end of get request
  }

  while (client.connected() && !client.available()) delay(1);   // waits for data
  while (client.connected() || client.available()) {            // connected or data available
    char c = client.read();                                     // gets byte from ethernet buffer
    if (c == lf) x = (x + 1);                                   // counting line feeds
    if (x == 10) readString += c;                               // building readString
  }
  lcd.setCursor(0, 0);
  readString.trim();
  doorstatus = readString;
  client.stop();                                                // stop client
  return readString;
}

void setup() {
  // initialize led strip/panel/matrix
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  reading  = analogRead(AMBIENCE_LIGHT_PIN);
  FastLED.clear(true); 
  
  lcd.begin(16, 2);                                             // display boot screen with three second delay for power up recovery
  lcd.setCursor(0, 0);
  lcd.print("Booting up ... 5");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 4");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 3");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 2");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 1");
  delay(1000);

  lcd.clear();
  lcd.print("Requesting IP");
  delay(200);
  if (Ethernet.begin(mac) == 0) {
    lcd.clear();
    lcd.print("DHCP failed!");                                  // no point in carrying on, so do nothing forevermore:
    lcd.setCursor(0, 1);
    lcd.print("Please restart.");
    while (true);
  }

  on = false;                                                   // configuring necessary values for alert & buzzer
  pinMode(buzzer, OUTPUT);

  timeClient.begin();                                           // start ntp client & request current time
  timeClient.forceUpdate();

  getDoorstatus();                                              // fetch doorstatus to have a value on boot
}

typedef void (*SimplePatternList[])();                          // List of patterns to cycle through.  Each is defined as a separate function below.
SimplePatternList Patterns = { sinelon, confetti };

uint8_t CurrentPattern = 0;                                     // Index number of which pattern is current
uint8_t Hue = 0;                                                // rotating "base color" may be used by patterns

void loop()
{
  timeClient.update();                                          // refresh date and time
  String dateTime = timeClient.getFormattedTime();

  EVERY_N_SECONDS(1) {                                          // only reading ambient light sensor every second to smoothen out transitions
    reading  = analogRead(AMBIENCE_LIGHT_PIN);
  }
  int cur_brightness = (BASE_BRIGHTNESS * 8) / reading * 2;     // adjust the MAX brightness value to match input readings
  if ( cur_brightness > 255 ) ( cur_brightness = 255 ) ;        // limit brightness value to original W2812B maximum of 255
  int i;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(dateTime);

  lcd.setCursor(10, 0);
  lcd.print("L: ");
  lcd.setCursor(13, 0);
  lcd.print(reading);

  int min_hue = 0;
  int max_hue = 255;

  lcd.setCursor(0, 1);
  lcd.print("Door: ");
  lcd.setCursor(6, 1);
  if (doorstatus == "closed") {                                 // check doorstatus and set appropriate hue limits
    min_hue = 0;
    max_hue = 63;
    lcd.print("Closed");
  }
  if (doorstatus == "internal") {
    min_hue = 40;
    max_hue = 70;
    lcd.print("Internal");
  }
  if (doorstatus == "open") {
    min_hue = 96;
    max_hue = 159;
    lcd.print("Open");
  }

  if ((Hue < min_hue) || (Hue > max_hue)) {                     // set appropriate hue limits
    Hue = min_hue;
  }

  if ((timeClient.getHours() == 22) && (timeClient.getMinutes() == 0)) {
    if (on) {                                                   // trigger audiovisual alert at specific time
      on = false;
      fill_solid( leds, NUM_LEDS, CRGB(0, 0, 30));
      FastLED.show();
      for (i = 0; i < 100; i++)
      {
        digitalWrite(buzzer, HIGH);
        delay(10);
        digitalWrite(buzzer, LOW);
        delay(10);
      }
      delay(50);
    } else {
      fill_solid( leds, NUM_LEDS, CRGB(50, 0, 0));
      FastLED.show();
      on = true;
      delay(50);
    }
  } else {                                                      // clear LEDs after alert animation
    if (dateTime == "22:01:00") {
      FastLED.clear(true);
    }
    if (reading < 150 ) {
      if ((reading < 150) && (startFlag == 0) ) {               // ensure ambience light reading is < 200 for at least 5 seconds
        startFlag = 1;
        startTime = millis();
        previousTime = startTime;
      }
      unsigned long currTime = millis();
      previousTime = previousTime++;
      if ((currTime - startTime ) <= 5000) {                    // still waiting it out
        if (reading > 150) {
          startFlag = 0;
        }
        delay(100);                                         
      }
      if ((currTime - startTime) > 5000) {                      // ambience light has been "dark" enough for long enough
        FastLED.setBrightness(cur_brightness);
        Patterns[CurrentPattern]();                             // Call the current pattern function once, updating the 'leds' array
        FastLED.show();                                         // send the 'leds' array out to the actual LED strip
        FastLED.delay(1000 / FRAMES_PER_SECOND);                // insert a delay to keep the framerate modest

        EVERY_N_MILLISECONDS( 80 ) {                            // do some periodic updates
          Hue++;                                                // cycle the "base color" through the rainbow
        }
        EVERY_N_SECONDS( 60 ) {
          nextPattern();                                        // change pattern every 60 seconds (if there is more than one)
        }
      }
    } else {
      delay(300);                                               // add a delay to deflicker the LCD when not running time consuming
      FastLED.clear(true);                                      // animations, also keep the LEDs cleared.
      startFlag = 0;
    }
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()                                              // add one to the current pattern number, and wrap around at the end
{
  CurrentPattern = (CurrentPattern + 1) % ARRAY_SIZE( Patterns);

  getDoorstatus();                                              // fetch current doorstatus
}

void confetti()
{
  EVERY_N_MILLISECONDS(100) {
    fadeToBlackBy( leds, NUM_LEDS, 7);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( Hue + random8(8), 200, random(150, 255));
  }
}

void sinelon()
{
  fadeToBlackBy( leds, NUM_LEDS, 20);                           // a colored dot sweeping back and forth, with fading trails
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( Hue, 255, 192);
}
