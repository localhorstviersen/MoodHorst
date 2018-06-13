#include "FastLED.h"
#include "LiquidCrystal.h"
#include <Wire.h>
#include <DS3231.h>

// thankfully based on the code examples of:
// -Mark Kriegsman, December 2014

FASTLED_USING_NAMESPACE

#define DATA_PIN    3
#define AMBIENCE_LIGHT_PIN  0 // For input from photocell (or potentiometer) to dim the leds by external input
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    256
CRGB leds[NUM_LEDS];

#define BASE_BRIGHTNESS     90
#define FRAMES_PER_SECOND  240

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

DS3231 clock;
RTCDateTime dt;
bool on;
int buzzer = 4;

void setup() {
  clock.begin();
  clock.setDateTime(__DATE__, __TIME__);
  on = false;
  pinMode(buzzer, OUTPUT);

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
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList Patterns = { confetti, sinelon };

uint8_t CurrentPattern = 0; // Index number of which pattern is current
uint8_t Hue = 0; // rotating "base color" may be used by patterns

void loop()
{
  int i;
  lcd.clear();
  lcd.setCursor(0, 0);
  int reading  = analogRead(AMBIENCE_LIGHT_PIN);
  lcd.print("Light: " + (String)reading);

  int cur_brightness = (BASE_BRIGHTNESS * 8) / reading * 2; // adjust the MAX brightness value to match input readings
  if ( cur_brightness > 255 ) ( cur_brightness = 255 ) ; // limit brightness value to original W2812B maximum of 255

  lcd.setCursor(0, 1);
  //lcd.print("Brightness: " + (String)cur_brightness);
  dt = clock.getDateTime();
  lcd.print(clock.dateFormat("H:i:s", dt));

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
      delay(200);
    } else {
      fill_solid( leds, NUM_LEDS, CRGB(50, 0, 0));
      FastLED.show();
      on = true;
      delay(200);
    }
  } else {
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
}

void confetti()
{
  fadeToBlackBy( leds, NUM_LEDS, 7);
  delay(80);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( Hue + random8(64), 200, random(150, 255));
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( Hue, 255, 192);
}

