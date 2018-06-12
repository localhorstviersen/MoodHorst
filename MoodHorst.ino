#include "FastLED.h"

FASTLED_USING_NAMESPACE

#define DATA_PIN    3
#define AMBIENCE_LIGHT_PIN  0 // For input from photocell (or potentiometer) to dim the leds by external input
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    256
CRGB leds[NUM_LEDS];

#define MAX_BRIGHTNESS     80
#define FRAMES_PER_SECOND  240

void setup() {
  delay(3000); // 3 second delay for recovery
  
  // initialize led strip/panel/matrix
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList Patterns = { confetti };

uint8_t CurrentPattern = 0; // Index number of which pattern is current
uint8_t Hue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
    int reading  = analogRead(AMBIENCE_LIGHT_PIN);

    int cur_brightness = (MAX_BRIGHTNESS * 8) / reading * 2; // adjust the MAX brightness value to match input readings

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

