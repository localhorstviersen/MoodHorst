#include "FastLED.h"
#include "LiquidCrystal.h"

FASTLED_USING_NAMESPACE

#define DATA_PIN    3
#define AMBIENCE_LIGHT_PIN  0 // For input from photocell (or potentiometer) to dim the leds by external input
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    256
CRGB leds[NUM_LEDS];

#define BASE_BRIGHTNESS     255
#define FRAMES_PER_SECOND  240

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
  delay(3000); // 3 second delay for recovery
  
  // initialize led strip/panel/matrix
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  lcd.begin(16, 2);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList Patterns = { confetti };

uint8_t CurrentPattern = 0; // Index number of which pattern is current
uint8_t Hue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    int reading  = analogRead(AMBIENCE_LIGHT_PIN);
    lcd.print("Light: " + (String)reading);

    int cur_brightness = (BASE_BRIGHTNESS * 8) / reading * 2; // adjust the MAX brightness value to match input readings
    if( cur_brightness > 255 ) ( cur_brightness = 255 ) ; // limit brightness value to original W2812B maximum of 255

    lcd.setCursor(0, 1);
    lcd.print("Brightness: " + (String)cur_brightness);

    if(reading < 200 ) {
      FastLED.setBrightness(cur_brightness);  
      // Call the current pattern function once, updating the 'leds' array
      Patterns[CurrentPattern]();
    
      // send the 'leds' array out to the actual LED strip
      FastLED.show();  
      // insert a delay to keep the framerate modest
      FastLED.delay(1000/FRAMES_PER_SECOND); 
    
      // do some periodic updates
      EVERY_N_MILLISECONDS( 80 ) { Hue++; } // cycle the "base color" through the rainbow
      EVERY_N_SECONDS( 60 ) { nextPattern(); } // change pattern every 60 seconds (if there are more than one)
    } else {
        FastLED.clear(true);
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
  leds[pos] += CHSV( Hue + random8(64), 200, random(150,255));
}

