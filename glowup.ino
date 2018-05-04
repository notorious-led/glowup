#include <DmxSimple.h>
#include <FastLED.h>
#include <Cmd.h>
#include <NotoriousSync.h>

#define NUMPIXELS 23
#define DEBUG true
#define FRAMERATE 60
#define WALLSIZE 8
#define BUTTON_ADVANCE 6
#define BUTTON_STOP A2

CRGB leds[NUMPIXELS];
uint8_t brightness[NUMPIXELS];
uint8_t strobe[NUMPIXELS];
uint8_t voice[NUMPIXELS];
uint8_t speed[NUMPIXELS];

unsigned long frameCount;
unsigned long timer1s;

//EFFECT SHIT
byte effect = 2;
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

CRGBPalette16 currentPalette = CloudColors_p;

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

//Countdown shit
unsigned long midnight = 0;
bool pulseNow = false;
NotoriousSync sync;


void setup() {
    Serial.begin(9600);
    Serial.println("glowup");

    Serial.println("Starting dmx");
    DmxSimple.usePin(3);
    DmxSimple.write(1, 5); DmxSimple.write(5, 255); //first light dim red

    Serial.print("Initializing "); Serial.print(NUMPIXELS); Serial.println(" lights");
    for ( int i=0; i<NUMPIXELS; i++ ) {
        DmxSimple.write(i*7+1, 255); //100% brightness
        DmxSimple.write(i*7+2, 0); //no strobe
        DmxSimple.write(i*7+3, 0); //no effect
        DmxSimple.write(i*7+4, 0); //no speed
    }

    Serial.println("Setting brightness");
    for ( int i=0; i<NUMPIXELS; i++ ) {
        brightness[i] = 100; //While debugging it's 0. usually it's 100
    }

    Serial.println("Doing fastled shit");
    FastLED.setMaxRefreshRate(FRAMERATE);
    leds[0] = CRGB::Green;
    FastLED.addLeds<NEOPIXEL, 10>(leds, NUMPIXELS);

    Serial.println("Starting command line");
    cmdInit(&Serial);
    cmdAdd("e", cmdEffect);
    cmdAdd("b", cmdSetting);
    cmdAdd("f", cmdSetting);
    //cmdAdd("v", cmdSetting);
    //cmdAdd("s", cmdSetting);
    cmdAdd("c", cmdColor);
    cmdAdd("count", cmdCount);
    cmdAdd("rtt", cmdRtt);
    cmdAdd("sched", cmdSchedule);
    cmdAdd("reboot", cmdReboot);

    Serial.println("Starting butan");
    pinMode(BUTTON_ADVANCE, INPUT_PULLUP);
    pinMode(BUTTON_STOP, INPUT_PULLUP);

    Serial.println("Init done!");

}

void loop() {

    frameCount++;

    if ( ! digitalRead(BUTTON_STOP) ) {
		effect = 15;
    }
	EVERY_N_MILLISECONDS(1000) {

		//time to do our every-second tasks
		#ifdef DEBUG
		double fr = (double)frameCount/((double)(millis()-timer1s)/1000);
		Serial.print("[Hbeat] bright="); Serial.print(brightness[0]);
        Serial.print(" effect="); Serial.print(effect);
        Serial.print(" fps="); Serial.print(fr);
		Serial.println();
		#endif /*DEBUG*/

		timer1s = millis();
		frameCount = 0;

		if ( effect <= 2 && millis() < 10000 ) {
			effect = 16;
		}

        if ( ! digitalRead(BUTTON_ADVANCE) ) {
            effect++;
            if ( effect > 20 ) {
                effect = 3;
            }
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
        case 18:
            runCylon();
            break;
        case 19:
            runBreathe();
            break;
        case 20:
            runAlternate();
            break;
        default:
            Serial.print("[blink] Unknown effect selected: "); Serial.println(effect);
            delay(10);
    }

    if ( midnight > 0 ) {
        doCountdown();
    }

    dmxBlit(); //This actually shows to the DMX "strip"
    FastLED.show(); //We literally just do this so that it will limit our frame rate

    cmdPoll();

}

void dmxBlit() {
    CRGB temp;
    for ( int i=0; i<NUMPIXELS; i++ ) {
        //DmxSimple.write(i*7+1, brightness[i]);
        DmxSimple.write(i*7+1, 255); //always 100% brightness
        temp = leds[i];
        temp.nscale8_video(brightness[i]); //but scale the rgb instead
        DmxSimple.write(i*7+5, temp.r);
        DmxSimple.write(i*7+6, temp.g);
        DmxSimple.write(i*7+7, temp.b);

        DmxSimple.write(i*7+2, strobe[i]);
        DmxSimple.write(i*7+3, voice[i]);
        DmxSimple.write(i*7+4, speed[i]);
    }
}

void splitForTimesBuilding() {
    //First we move leds 0-5 to leds 2-7. This is the side of the building facing the SiP parking lot
    for ( int i=2+WALLSIZE-1; i>=2; i-- ) {
        leds[i] = leds[i-2];
    }
    //Then we fix LEDs 0 and 1 to be the same as their incremented counterparts.
    for ( int i=0; i<=1; i++ ) {
        leds[i] = leds[i+WALLSIZE];
    }
    //Now we fill in the rest of the strip with the values we can steal from 2-7.
    for ( int i=2+WALLSIZE; i < NUMPIXELS; i++ ) {
        leds[i] = leds[i%WALLSIZE];
    }
}

void doCountdown() {
    unsigned long now = millis();
    unsigned long delta = midnight - now;

    if ( now > midnight ) {
        //It's the new year.
        if ( now < (midnight+500) ) {
            runFill(CRGB::White); //fill the strip with white
        } else if ( now > (midnight+10000) ) {
            effect = 7; //less intense confetti
            midnight = 0; //stop the counting shit.
        } else {
            effect = 9; //go to some intense confetti
        }
    } else if ( (delta) > 10000 ) {
        effect = 6; //FastCirc while we wait
    } else {
        //We're within 10 seconds.
        effect = 17; //Pulsing rainbow
        if ( ((delta+100) % 1000) < 100 ) {
            //we're within 100 ms of a second-rollover having just happened. (Sorry I hate both english and math)
            pulseNow = true;
            Serial.print("[count] delta="); Serial.print(delta); Serial.print(" millis="); Serial.println(now);
        }

        if ( (delta) < 5200 ) {
            effect = 11; //REALLY fast rainbow
            if ( delta < 5000 ) leds[0] = CRGB::Black;
            if ( delta < 4000 ) leds[5] = CRGB::Black;
            if ( delta < 3000 ) leds[1] = CRGB::Black;
            if ( delta < 2000 ) leds[4] = CRGB::Black;
            if ( delta < 1000 ) leds[2] = CRGB::Black;

            splitForTimesBuilding(); //replicate to other sides of the building.
            leds[NUMPIXELS-1] = CRGB::Black; leds[0] = CRGB::Black; leds[1] = CRGB::Black; //turn off the weird ones
        }
    }
}

void cmdSetting(int argc, char ** argv) {
    if ( argc > 1 ) {
        uint8_t x = String(argv[1]).toInt();
        for ( int i=0; i<NUMPIXELS; i++ ) {
            switch(argv[0][0]) {
                case 'b':
                    brightness[i] = x;
                    FastLED.setBrightness(x);
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
void cmdCount(int argc, char ** argv) {
    Serial.print("millis(): "); Serial.println(millis());
    midnight = millis() + ( 12 * 1000 );
    Serial.print("Midnight: "); Serial.println(midnight);
}
void cmdRtt(int argc, char ** argv) {
    sync.setRtt(String(argv[1]).toInt());
    Serial.print("rtt = "); Serial.println(sync.getRtt());
}
void cmdSchedule(int argc, char ** argv) {
    midnight = millis() + ( String(argv[1]).toInt() ) - ( sync.getRtt() / 2 );
    Serial.print("[nsync] It's "); Serial.print(millis()); 
    Serial.print(" - event scheduled for "); Serial.println(midnight);
}
void cmdReboot(int argc, char ** argv) {
    delay(String(argv[1]).toInt() - (sync.getRtt() / 2));
    asm volatile ("  jmp 0");
}

void runFill(CRGB dest) {
    fill_solid(leds, NUMPIXELS, dest);
}
void runFill() {
    runFill(CRGB::Black);
}

void runFullPalette() {
    //uint8_t beatA = beat8(30); //, 0, 255); //was beatsin8
    uint8_t beatA = 0; //static

    fill_palette(leds, NUMPIXELS, beatA, 0, currentPalette, 255, LINEARBLEND);
}
void runRotatingPalette() {
    uint8_t beatA = beat8(30); //, 0, 255); //was beatsin8
    fill_palette(leds, NUMPIXELS, beatA, 18, currentPalette, 255, LINEARBLEND);
}
void runPulsingPalette() {
    uint8_t beatA = beat8(50);
    uint8_t beatB = beat8(77);
    if ( pulseNow || ( midnight == 0 && beatA < 10 ) ) {
        fill_palette(leds, NUMPIXELS, beatB, 0, RainbowColors_p, 255, NOBLEND);
        pulseNow = false;
    } else {
        fadeToBlackBy(leds, NUMPIXELS, 10);
    }
}

void runConfetti() {
    EVERY_N_MILLISECONDS(5000) {
        switch(effect) {
            case 7: thisinc=1; thishue=192; thissat=255; thisfade=16; huediff=256; break;  // You can change values here, one at a time , or altogether.
            case 8: thisinc=2; thishue=128; thissat=100; thisfade=24; huediff=64; break;
            case 9: thisinc=4; thishue=random16(255); thissat=125; thisfade=40; huediff=64; break;      // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
        }
    }

    fadeToBlackBy(leds, NUMPIXELS, thisfade);                    // Low values = slower fade.
    for ( int i=effect; i>=7; i-- ) {
        leds[random16(NUMPIXELS)] += CHSV((thishue + random16(huediff))/4 , thissat, thisbri);  // I use 12 bits for hue so that the hue increment isn't too quick.
    }
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
        count += 8;
    } else { 
        count += 1;
    }
}

void runJuggle() {
    switch(effect) {
            case 12: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break;                  // You can change values here, one at a time , or altogether.
            case 13: numdots = 4; basebeat = 10; hueinc = 16; faderate = 16; thishue = 128; break;
    }
    curhue = thishue;                                           // Reset the hue values.
    fadeToBlackBy(leds, NUMPIXELS, faderate);
    for( int i = 0; i < numdots; i++) {
        int temp = beatsin16(basebeat+i+numdots,0,NUMPIXELS);
        if ( temp >= NUMPIXELS ) temp = NUMPIXELS-1;
        switch(effect) {
            case 12: leds[temp] = color; break;
            default: leds[temp] += CHSV(curhue, thissat, thisbri); break;
        }
        curhue += hueinc;
    }
}

void runCylon() {
    if ( count >= WALLSIZE-1 ) {
        count = WALLSIZE-1;
        thisdir = -1;
    } else if ( count == 0 ) {
        thisdir = 1;
    }
    EVERY_N_MILLISECONDS(1000/(WALLSIZE*2-2)) {
        runFill(); //XXX we should be able to fade here but i'm too tired to work it out today
        leds[count] = color;
        count += thisdir;
        splitForTimesBuilding();
    }
    //fadeToBlackBy(leds, NUMPIXELS, 32);

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
    } else {
        runFill(); //blackout
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

void runBreathe(){
    runFill(color);
    fadeToBlackBy(leds, NUMPIXELS, beatsin8(15));
}

void runAlternate(){
    runFill(color);                                       //pick a color
    CRGB color2 = CRGB(color.g, color.b, color.r);        //and this is some somewhat opposite color
                      
    for(int i=0; i<NUMPIXELS; i++){                  
      if (i%2 ==1) {                                       //for the odd ones
        leds[i]=color2;                                    //make the odd LEDS some opposite color
        leds[i].fadeToBlackBy(beatsin8(10, 0));     //fade a sine wave, starting at theta = 0
      }else{
       leds[i].fadeToBlackBy(beatsin8(10, 0, 255, 0, 255/2));   //fade another sine wave, but starting at 
      }
    }
}

