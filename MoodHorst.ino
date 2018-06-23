#include <FastLED.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>
#include <SPI.h>
#include <Ethernet.h>

// thankfully based on the code examples of:
// -Mark Kriegsman, December 2014

// Initialize WS2812B matrix and required config
FASTLED_USING_NAMESPACE
#define DATA_PIN    5
#define AMBIENCE_LIGHT_PIN  0 // For input from photocell (or potentiometer) to dim the leds by external input
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    256
CRGB leds[NUM_LEDS];

#define BASE_BRIGHTNESS     160
#define FRAMES_PER_SECOND  240

// Initialize real time clock and buzzer for alerting features
DS3231 clock;
RTCDateTime dt;
bool on;
int buzzer = 4;

// Palette definitions
CRGBPalette16 currentPalette = PartyColors_p;
CRGBPalette16 targetPalette = PartyColors_p;
TBlendType    currentBlending = LINEARBLEND;                  // NOBLEND or LINEARBLEND


// Initialize LCD
LiquidCrystal lcd(7, 8, 0, 1, 2, 3);

// Ethernet & doorstatus
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address
char serverName[] = "10.234.2.103"; // zoomkat's test web page server
EthernetClient client;
String doorstatus;
String readString;
int x = 0; //for counting line feeds
char lf = 10; //line feed character
int i = 0;

String sendGET() //client function to send/receive GET request data.
{
  x = 0;
  i++;
  readString = ("");
  if (client.connect(serverName, 8090)) {  //starts client connection, checks for connection
    client.println("GET /doorstatus/status.txt HTTP/1.1"); //download text
    client.println("Host: 10.234.2.103");
    client.println("Connection: close");  //close 1.1 persistent connection
    client.println(); //end of get request
  }

  while (client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available()) { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer
    if (c == lf) x = (x + 1); //counting line feeds
    if (x == 10) readString += c; //building readString
  }
  lcd.setCursor(0, 0);
  readString.trim();
  doorstatus = readString;
  client.stop(); //stop client
  return readString;
}

void setup() {
  // start realtime clock and set datetime
  clock.begin();
  clock.setDateTime(__DATE__, __TIME__);
  on = false;
  pinMode(buzzer, OUTPUT);

  // display boot screen
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  // three second delay for power up recovery
  lcd.print("Booting up ... 3");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 2");
  delay(1000);
  lcd.clear();
  lcd.print("Booting up ... 1");
  delay(1000);

  // initialize led strip/panel/matrix
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  lcd.clear();
  lcd.print("Requesting IP ...");
  if (Ethernet.begin(mac) == 0) {
    lcd.clear();
    lcd.print("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    while (true);
  }

  sendGET();
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList Patterns = { confetti, sinelon };

uint8_t CurrentPattern = 0; // Index number of which pattern is current
uint8_t Hue = 0; // rotating "base color" may be used by patterns

void loop()
{
  dt = clock.getDateTime();
  int reading  = analogRead(AMBIENCE_LIGHT_PIN);
  int cur_brightness = (BASE_BRIGHTNESS * 8) / reading * 2; // adjust the MAX brightness value to match input readings
  if ( cur_brightness > 255 ) ( cur_brightness = 255 ) ; // limit brightness value to original W2812B maximum of 255
  int i;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(clock.dateFormat("H:i:s", dt));

  lcd.setCursor(0, 1);
  lcd.print(doorstatus);

  int min_hue = 0;
  int max_hue = 255;

  // check doorstatus and set appropriate hue limits
  if (doorstatus == "closed") {
    min_hue = 0;
    max_hue = 63;
  }
  if (doorstatus == "internal") {
    min_hue = 40;
    max_hue = 70;
  }
  if (doorstatus == "open") {
    min_hue = 96;
    max_hue = 159;
  }

  // set appropriate hue limits
  if ((Hue < min_hue) || (Hue > max_hue)) {
    Hue = min_hue;
  }

  // trigger audiovisual alert at specific time
  if (strcmp(clock.dateFormat("H:i", dt), "22:00") == 0) {
    if (on) {
      on = false;
      fill_solid( leds, NUM_LEDS, CRGB(0, 0, 30));
      FastLED.show();
      for (i = 0; i < 100; i++)
      {
        digitalWrite(buzzer, HIGH);
        delay(2);//wait for 2ms
        digitalWrite(buzzer, LOW);
        delay(2);//wait for 2ms
      }
      delay(50);
    } else {
      fill_solid( leds, NUM_LEDS, CRGB(50, 0, 0));
      FastLED.show();
      on = true;
      delay(50);
    }
  } else {
    // clear LEDs after alert animation
    if (strcmp(clock.dateFormat("H:i:s", dt), "22:01:00") == 0) {
      FastLED.clear(true);
    }
    if (reading < 200 ) {
      FastLED.setBrightness(cur_brightness);
      // Call the current pattern function once, updating the 'leds' array
      Patterns[CurrentPattern]();

      // send the 'leds' array out to the actual LED strip
      FastLED.show();
      // insert a delay to keep the framerate modest
      FastLED.delay(1000 / FRAMES_PER_SECOND);

      // do some periodic updates
      EVERY_N_MILLISECONDS( 80 ) {
        Hue++;  // cycle the "base color" through the rainbow
      }
      EVERY_N_SECONDS( 60 ) {
        nextPattern();  // change pattern every 60 seconds (if there is more than one)
      }
    } else {
      FastLED.clear(true);
    }
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  CurrentPattern = (CurrentPattern + 1) % ARRAY_SIZE( Patterns);

  // fetch current doorstatus
  sendGET();
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
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( Hue, 255, 192);
}
