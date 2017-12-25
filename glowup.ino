#include <DmxMaster.h>
#include <FastLED.h>
#include <Cmd.h>

#define NUMPIXELS 23
#define DEBUG true
#define FRAMERATE 60

CRGB leds[NUMPIXELS];
uint8_t brightness[NUMPIXELS];
uint8_t strobe[NUMPIXELS];
uint8_t voice[NUMPIXELS];
uint8_t speed[NUMPIXELS];

unsigned long frameCount;
unsigned long timer1s;

//EFFECT SHIT
byte effect = 0;
CRGB color = CRGB::Teal;
CRGB nextColor = CRGB::Black;

// Gradient palette "flame_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/flame.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.
DEFINE_GRADIENT_PALETTE( flame_gp ) {
    0, 252, 42,  1,
   43, 217,  6,  1,
   89, 213, 66,  1,
  127,   3, 74,  1,
  165, 213, 66,  1,
  255, 252, 42,  1
};
DEFINE_GRADIENT_PALETTE( christmas_gp ) {
    0,   0,  0,  0,
  127,   0,255,  0,
    0,   0,  0,  0,
  255, 255,  0,  0, 
    0,   0,  0,  0
};

CRGBPalette16 currentPalette = christmas_gp;

//BlinkOne/SolidOne
uint8_t offset = 0; //how many to skip when writing the LED.
//Confetti
uint8_t  thisfade = 16;                                        // How quickly does it fade? Lower = slower fade rate.
int       thishue = 50;                                       // Starting hue.
uint8_t   thisinc = 1;                                        // Incremental value for rotating hues
uint8_t   thissat = 100;                                      // The saturation, where 255 = brilliant colours.
uint8_t   thisbri = 255;                                      // Brightness of a sequence. Remember, max_bright is the overall limiter.
int       huediff = 256;                                      // Range of random #'s to use for hue
//DotBeat
uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
uint8_t bpm = 30;
//EaseMe
bool rev= false;
//FastCirc
int thiscount = 0;
int thisdir = 1;
int thisgap = 8;
//Juggle
uint8_t    numdots =   4;                                     // Number of dots in use.
uint8_t   faderate =   2;                                     // How long should the trails be. Very low value = longer trails.
uint8_t     hueinc =  16;                                     // Incremental change in hue between each dot.
uint8_t     curhue =   0;                                     // The current hue
uint8_t   basebeat =   5;                                     // Higher = faster movement.
//Lightning
uint8_t frequency = 50;                                       // controls the interval between strikes
uint8_t flashes = 8;                                          //the upper limit of flashes per strike
uint8_t flashCounter = 0;                                     //how many flashes have we done already, during this cycle?
unsigned long lastFlashTime = 0;                              //when did we last flash?
unsigned long nextFlashDelay = 0;                             //how long do we wait since the last flash before flashing again?
unsigned int dimmer = 1;
uint8_t ledstart;                                             // Starting location of a flash
uint8_t ledlen;                                               // Length of a flash



void setup() {
    Serial.begin(9600);
    Serial.println("glowup");
    delay(500);

    Serial.println("Starting dmx");
    DmxMaster.usePin(3);
    DmxMaster.write(1, 5); DmxMaster.write(5, 255); //first light dim red

    Serial.print("Initializing "); Serial.print(NUMPIXELS); Serial.println(" lights");
    for ( int i=0; i<NUMPIXELS; i++ ) {
        DmxMaster.write(i*7+1, 255); //100% brightness
        DmxMaster.write(i*7+2, 0); //no strobe
        DmxMaster.write(i*7+3, 0); //no effect
        DmxMaster.write(i*7+4, 0); //no speed
    }

    Serial.println("Setting brightness");
    for ( int i=0; i<NUMPIXELS; i++ ) {
        brightness[i] = 100;
    }

    Serial.println("Doing fastled shit");
    FastLED.setMaxRefreshRate(FRAMERATE);
    leds[0] = CRGB::Green;

    Serial.println("Starting command line");
    cmdInit(&Serial);
    cmdAdd("e", cmdEffect);
    cmdAdd("b", cmdSetting);
    cmdAdd("f", cmdSetting);
    cmdAdd("v", cmdSetting);
    cmdAdd("s", cmdSetting);
    cmdAdd("c", cmdColor);

    Serial.println("Init done!");

}

void loop() {

    frameCount++;
    FastLED.show(); //We literally just do this so that it will limit our frame rate

	EVERY_N_MILLISECONDS(1000) {

		//time to do our every-second tasks
		#ifdef DEBUG
		double fr = (double)frameCount/((double)(millis()-timer1s)/1000);
		Serial.print("[Hbeat] FRAME RATE: "); Serial.print(fr);
		Serial.println();
		#endif /*DEBUG*/

		timer1s = millis();
		frameCount = 0;

		if ( effect <= 2 && millis() < 10000 ) {
			effect = 16;
		}

	}

    switch (effect) {
        case 1:
            runBlinkOne();
            break;
        case 2:
            runSolidOne();
            break;
        case 3:
            runFill(color);
            break;
        case 4:
            runDotBeat();
            break;
        case 5:
            runEaseMe();
            break;
        case 6:
            runFastCirc();
            break;
        case 7: //Multicolor confetti
        case 8: //Broken confetti?
        case 9: //White confetti
            runConfetti();
            break;
        case 10: //slow
        case 11: //fast
            runRotatingRainbow();
            break;
        case 12:
        case 13:
            runJuggle();
            break;
        case 14:
            runLightning();
            break;
        case 15:
            runFullPalette();
            break;
        case 16:
            runRotatingPalette();
            break;
        case 17:
            runPulsingPalette();
            break;
        default:
            //Serial.print("[blink] Unknown effect selected: "); Serial.println(effect);
            delay(10);
    }

    dmxBlit();

    cmdPoll();

}

void dmxBlit() {
    CRGB temp;
    for ( int i=0; i<NUMPIXELS; i++ ) {
        //DmxMaster.write(i*7+1, brightness[i]);
        DmxMaster.write(i*7+1, 255); //always 100% brightness
        temp = leds[i];
        temp.nscale8_video(brightness[i]); //but scale the rgb instead
        DmxMaster.write(i*7+5, temp.r);
        DmxMaster.write(i*7+6, temp.g);
        DmxMaster.write(i*7+7, temp.b);

        DmxMaster.write(i*7+2, strobe[i]);
        DmxMaster.write(i*7+3, voice[i]);
        DmxMaster.write(i*7+4, speed[i]);
    }
}

void cmdSetting(int argc, char ** argv) {
    if ( argc > 1 ) {
        uint8_t x = String(argv[1]).toInt();
        for ( int i=0; i<NUMPIXELS; i++ ) {
            switch(argv[0][0]) {
                case 'b':
                    brightness[i] = x;
                    break;
                case 'f':
                    strobe[i] = x;
                    break;
                case 'v':
                    voice[i] = x;
                    break;
                case 's':
                    speed[i] = x;
                    break;
            }
        }
                    
        Serial.println(x);
    }
}
void cmdColor(int argc, char ** argv) {
    if ( argc > 3 ) {
        color = CRGB(String(argv[1]).toInt(), String(argv[2]).toInt(), String(argv[3]).toInt());
        Serial.print(color.r); Serial.print(","); Serial.print(color.g); Serial.print(","); Serial.println(color.b);
    }
}
void cmdEffect(int argc, char ** argv) {
    if ( argc > 1 ) {
        effect = String(argv[1]).toInt();
    }
    Serial.println(effect);
}


void runFill(CRGB dest) {
    fill_solid(leds, NUMPIXELS, dest);
}
void runFill() {
    runFill(CRGB::Black);
}

void runFullPalette() {
    uint8_t beatA = beat8(30); //, 0, 255); //was beatsin8

    fill_palette(leds, NUMPIXELS, beatA, 0, currentPalette, 255, LINEARBLEND);
}
void runRotatingPalette() {
    uint8_t beatA = beat8(30); //, 0, 255); //was beatsin8
    fill_palette(leds, NUMPIXELS, beatA, 18, currentPalette, 255, LINEARBLEND);
}
void runPulsingPalette() {
    uint8_t beatA = beat8(60);
    uint8_t beatB = beat8(77);
    if ( beatA < 10 ) {
        fill_palette(leds, NUMPIXELS, beatB, 0, RainbowColors_p, 255, NOBLEND);
    } else {
        fadeToBlackBy(leds, NUMPIXELS, 10);
    }
}

void runConfetti() {
    EVERY_N_MILLISECONDS(5000) {
        switch(effect) {
            case 7: thisinc=1; thishue=192; thissat=255; thisfade=16; huediff=256; break;  // You can change values here, one at a time , or altogether.
            case 8: thisinc=2; thishue=128; thissat=100; thisfade=8; huediff=64; break;
            case 9: thisinc=1; thishue=random16(255); thissat=100; thisfade=8; huediff=16; break;      // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
        }
    }

    fadeToBlackBy(leds, NUMPIXELS, thisfade);                    // Low values = slower fade.
    int pos = random16(NUMPIXELS);                               // Pick an LED at random.
    leds[pos] += CHSV((thishue + random16(huediff))/4 , thissat, thisbri);  // I use 12 bits for hue so that the hue increment isn't too quick.
    thishue = thishue + thisinc;                                // It increments here.
}

void runDotBeat() {
    uint8_t inner = beatsin8(bpm, NUMPIXELS/4, NUMPIXELS/4*3);
    uint8_t outer = beatsin8(bpm, 0, NUMPIXELS-1);
    uint8_t middle = beatsin8(bpm, NUMPIXELS/3, NUMPIXELS/3*2);

    //leds[middle] = CRGB::Purple; leds[inner] = CRGB::Blue; leds[outer] = CRGB::Aqua;
    leds[middle] = CRGB::Aqua; leds[inner] = CRGB::Blue; leds[outer] = CRGB::Purple;

    nscale8(leds,NUMPIXELS,fadeval);                             // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);
}

void runFastCirc() {
    EVERY_N_MILLISECONDS(50) {
        thiscount = (thiscount + thisdir)%thisgap;
        for ( int i=thiscount; i<NUMPIXELS; i+=thisgap ) {
            leds[i] = color;
        }
    }
    fadeToBlackBy(leds, NUMPIXELS, 36);
}

void runEaseMe() {
    static uint8_t easeOutVal = 0;
    static uint8_t easeInVal  = 0;
    static uint8_t lerpVal    = 0;

    easeOutVal = ease8InOutQuad(easeInVal);
    if ( rev ) {
        easeInVal -= 3;
    } else {
        easeInVal += 3;
    }
    if ( easeInVal > 250 ) {
        rev = true;
    } else if ( easeInVal < 5 ) {
        rev = false;
    }

    lerpVal = lerp8by8(0, NUMPIXELS, easeOutVal);

    for ( int i = lerpVal; i < NUMPIXELS; i += 8 ) {
        leds[i] = color;
    }
    fadeToBlackBy(leds, NUMPIXELS, 32);                     // 8 bit, 1 = slow, 255 = fast
}

void runRotatingRainbow() {
    fill_rainbow(leds, NUMPIXELS, count, 32);
    if ( effect == 11 ) {
        count += 3;
    } else { 
        count += 1;
    }
}

void runJuggle() {
    switch(effect) {
            case 12: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break;                  // You can change values here, one at a time , or altogether.
            case 13: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
    }
    curhue = thishue;                                           // Reset the hue values.
    fadeToBlackBy(leds, NUMPIXELS, faderate);
    for( int i = 0; i < numdots; i++) {
        int temp = beatsin16(basebeat+i+numdots,0,NUMPIXELS);
        if ( temp >= NUMPIXELS ) temp = NUMPIXELS-1;
        leds[temp] += CHSV(curhue, thissat, thisbri);   //beat16 is a FastLED 3.1 function
        curhue += hueinc;
    }
}

void runLightning() {
    //Serial.print("[ltnng] entered. millis()="); Serial.print(millis()); Serial.print(" lastFlashTime="); Serial.print(lastFlashTime); Serial.print(" nextFlashDelay="); Serial.println(nextFlashDelay);
    if ( (millis() - lastFlashTime) > nextFlashDelay ) { //time to flash
        Serial.print("[ltnng] flashCounter: ");
        Serial.println(flashCounter);
        nextFlashDelay = 0;
        if ( flashCounter == 0 ) {
            //Serial.println("[ltnng] New strike");
            //new strike. init our values for this set of flashes
            ledstart = random16(NUMPIXELS);           // Determine starting location of flash
            ledlen = random16(NUMPIXELS-ledstart);    // Determine length of flash (not to go beyond NUMPIXELS-1)
            dimmer = 5;
            nextFlashDelay += 150;   // longer delay until next flash after the leader
        } else {
            dimmer = random8(1,3);           // return strokes are brighter than the leader
        }

        if ( flashCounter < random8(3,flashes) ) {
            //Serial.println("[ltnng] Time to flash");
            flashCounter++;
            fill_solid(leds+ledstart,ledlen,CHSV(255, 0, 255/dimmer));
            dmxBlit();
            delay(random8(4,10));                 // each flash only lasts 4-10 milliseconds. We will use delay() because the timing has to be tight. still will run shorter than 10ms.
            fill_solid(leds+ledstart,ledlen,CHSV(255,0,0));   // Clear the section of LED's
            dmxBlit();
            nextFlashDelay += 50+random8(100);               // shorter delay between strokes
        } else {
            Serial.println("[ltnng] Strike complete");
            flashCounter = 0;
            nextFlashDelay = random8(frequency)*100;          // delay between strikes
        }
        lastFlashTime = millis();
    }
}

void runBlinkOne() {
    EVERY_N_MILLISECONDS(500) {
        if ( nextColor == CRGB(0,0,0) ) {
            nextColor = color;
        } else {
            nextColor = CRGB::Black;
        }
    }

    runFill();
    leds[offset] = nextColor;
}

void runSolidOne() {
    runFill();
    leds[offset] = color;
}


