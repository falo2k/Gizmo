#include <avdweb_Switch.h>
#include <FastLED.h>
#include <EEPROM.h>

#define NUM_LEDS 9
#define ERROR_LED 8

#define LED_PIN 10
#define SW1_PIN 6
#define SW2_PIN 7

CRGB leds[NUM_LEDS];

#define EEPROM_ANIMATION 0
#define EEPROM_COLOURSTATE 1

enum AnimationPattern {SlowFlicker, FastFlicker, Sequence, Pulse, PowerCell, Fill, Knight, END_PATTERNS};
enum ColourState{animationDefault, red, green, blue, white, fixedRainbow, movingRainbow, randomColour, END_COLOURSTATES, customColourState};

CRGB customColour = CRGB::Black;

const int animationFrequency = 100;
const int animationPeriod = 1000 / animationFrequency;

AnimationPattern currentPattern = AnimationPattern::SlowFlicker;
ColourState currentColourState = ColourState::animationDefault;

#define prevSwitchId 0
#define nextSwitchId 1
Switch prevSwitch = Switch(SW1_PIN);
Switch nextSwitch = Switch(SW2_PIN);

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);

	FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

    prevSwitch.setSingleClickCallback(&switchSingleClick, prevSwitchId);
    prevSwitch.setLongPressCallback(&switchHeld, prevSwitchId);
    prevSwitch.setDoubleClickCallback(&switchDoubleClick, prevSwitchId);

    nextSwitch.setSingleClickCallback(&switchSingleClick, nextSwitchId);
    nextSwitch.setLongPressCallback(&switchHeld, nextSwitchId);
    nextSwitch.setDoubleClickCallback(&switchDoubleClick, nextSwitchId);

    loadEEPROM();

    Serial.println("Setup Complete");
}

bool switchHeldState[2] = { false, false };

// Single Click to scroll through animations
void switchSingleClick(void* ref) {
    switch ((int)ref) {
        case prevSwitchId:
            if (currentPattern == 0) {
                currentPattern = AnimationPattern::END_PATTERNS;
            }

            currentPattern = currentPattern - 1;
            break;

        case nextSwitchId:
            currentPattern = currentPattern + 1;

            if (currentPattern == AnimationPattern::END_PATTERNS) {
                currentPattern = 0;
            }
            break;

        default:
            break;
    }

    Serial.print("Current Pattern: "); Serial.println(currentPattern);

    FastLED.clear(true);
}

void switchHeld(void* ref) {
    if (currentColourState == customColourState) {
        currentColourState = 0;
        return;
    }
    
    switch ((int)ref) {
    case prevSwitchId:
        if (currentColourState == 0) {
            currentColourState = ColourState::END_COLOURSTATES;
        }

        currentColourState = currentColourState - 1;
        break;

    case nextSwitchId:
        currentColourState = currentColourState + 1;

        if (currentColourState == ColourState::END_COLOURSTATES) {
            currentColourState = 0;
        }
        break;

    default:
        break;
    }

    Serial.print("Current Pattern: "); Serial.println(currentPattern);
}

void switchDoubleClick(void* ref) {
    switchHeldState[(int)ref] = true;

    if ((int)ref == prevSwitchId) {
        loadEEPROM();
    }
    else {
        EEPROM.update(EEPROM_ANIMATION, currentPattern);
        EEPROM.update(EEPROM_COLOURSTATE, currentColourState);
    }
}

void loadEEPROM() {
    int anim = EEPROM.read(EEPROM_ANIMATION);
    int cs = EEPROM.read(EEPROM_COLOURSTATE);

    //Serial.println(anim);
    //Serial.println(cs);

    if (anim != 255) { currentPattern = anim; }
    else { currentPattern = 0; }
    if (cs != 255) { currentColourState = cs; }
    else { currentColourState = 0; }
}

unsigned long lastAnimationUpdate = 0;
void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastAnimationUpdate) {
        switch (currentPattern) {
        case Sequence:
            doSequence(currentMillis);
            break;
        case Pulse:
            doPulse(currentMillis);
            break;
        case PowerCell:
            doPowerCell(currentMillis);
            break;
        case Fill:
            doFill(currentMillis);
            break;
        case Knight:
            doKnight(currentMillis);
            break;

        case SlowFlicker:
            doFlicker(currentMillis, false);
            break;

        case FastFlicker:
            doFlicker(currentMillis, true);
            break;

        default:
            break;
        }

        lastAnimationUpdate = currentMillis;
    }

    prevSwitch.poll();
    nextSwitch.poll();
}

uint8_t colourMoveShift = 0;
const int colourMoveStep = 25;
const int colourMoveStepAmount = 1;
unsigned long lastColourMove = 0;

CRGB getColour(unsigned long currentMillis, int lightIndex) {
    if (currentMillis - lastColourMove > colourMoveStep) {
        colourMoveShift = (colourMoveShift + colourMoveStepAmount) % 255;

        lastColourMove = currentMillis;
    }

    switch (currentColourState) {
        case red:
            return CRGB::Red;

        case green:
            return CRGB::Green;
        
        case blue:
            return CRGB::Blue;

        case white:
            return CRGB::White;

        case fixedRainbow:
            return CHSV(255.0 * ((float)lightIndex / (NUM_LEDS - 1)), 255, 255);
        case movingRainbow:
            //Serial.println((colourMoveShift + (int)(255 * ((float)lightIndex / (NUM_LEDS - 1)))) % 255);
            return CHSV((colourMoveShift + (int)(255 * ((float)lightIndex / (NUM_LEDS - 1)))) % 255, 255, 255);
        case randomColour:
            return CHSV(random(255), 255, 255);
            break;

        case customColourState:
            return customColour;

        case animationDefault:
        default:
            switch (currentPattern) {
                case SlowFlicker:
                case FastFlicker:
                    return (lightIndex < ERROR_LED) ? CRGB::LightSkyBlue : CRGB::Red;
                
                case Pulse:
                    return (lightIndex < ERROR_LED) ? CRGB::FireBrick : CRGB::Red;
                
                case PowerCell:
                    return (lightIndex < ERROR_LED) ? CRGB::DarkBlue : CRGB::DarkSlateBlue;
                
                case Sequence:
                    return (lightIndex < ERROR_LED) ? CRGB::MediumSeaGreen : CRGB::Red;
                
                case Fill:
                    return (lightIndex < ERROR_LED) ? CRGB::Blue : CRGB::AliceBlue;
                
                case Knight:
                    return (lightIndex < ERROR_LED) ? CRGB::Red : CRGB::White;
                
                default:
                    return CRGB::BlueViolet;
            }
    }

    return customColour;
}

void doFlicker(unsigned long currentMillis, bool fast) {
    // Fade all LEDS
    fadeToBlackBy(leds, NUM_LEDS, 32);
    
    // Fast: 5% of the time, set a random LED to colour
    // Slow: 0.5% of the time, set a random LED to colour
    if (random(fast ? 100 : 1000) < 5) {
        int ledIndex = random(9);

        leds[ledIndex] = getColour(currentMillis, ledIndex);
    }

    FastLED.show();
}

int sequencePos = 0;
const int sequenceStep = 150;
unsigned long lastSequenceStep = 0;

void doSequence(unsigned long currentMillis) {
    if (currentMillis - lastSequenceStep > sequenceStep) {
        FastLED.clear(false);

        leds[sequencePos] = getColour(currentMillis, sequencePos);

        if (++sequencePos >= NUM_LEDS) {
            sequencePos = 0;
        }

        FastLED.show();
        lastSequenceStep = currentMillis;
    }
}

float pulseLevel = 0;
bool pulseAscend = true;
const int pulseStep = 50;
const float pulseStepIncrease = 0.025;
unsigned long lastPulseStep = 0;

void doPulse(unsigned long currentMillis) {
    if (currentMillis - lastPulseStep > pulseStep) {
        
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = getColour(currentMillis, i).nscale8(pulseLevel * 255);
        }

        pulseLevel = pulseLevel + (pulseAscend ? pulseStepIncrease : -pulseStepIncrease);

        if (pulseLevel < 0) {
            pulseLevel = 0; pulseAscend = true;
        }

        if (pulseLevel > 1) {
            pulseLevel = 1; pulseAscend = false;
        }

        lastPulseStep = currentMillis;

        FastLED.show();
    }
}

int powerCellLevel = 0;
const int powerCellStep = 120;
unsigned long lastPowerCellStep = 0;

void doPowerCell(unsigned long currentMillis) {
    if (currentMillis - lastPowerCellStep > powerCellStep) {
        FastLED.clear(false);
        powerCellLevel = (powerCellLevel + 1) % (NUM_LEDS + 1);

        for (int i = 0; i < powerCellLevel; i++) {
            leds[i] = getColour(currentMillis, i);
        }

        FastLED.show();
        lastPowerCellStep = currentMillis;
    }
}

int fillLevel = 0;
int fillDrop = NUM_LEDS;
const int topHoldSteps = 10;
int topHoldStep = 10;
const int fillStep = 75;
unsigned long lastFillStep = 0;

void doFill(unsigned long currentMillis) {
    if (currentMillis - lastFillStep > fillStep) {
        if (fillLevel == NUM_LEDS) {
            // Hold the top level for a number of steps
            if (topHoldStep > 0) {
                --topHoldStep;
                for (int i = 0; i < ERROR_LED; i++) {
                    leds[i] = getColour(currentMillis, i);
                }
                FastLED.show();
                lastFillStep = currentMillis;
                return;
            }

            fillLevel = 0;
            fillDrop = NUM_LEDS;
            topHoldStep = topHoldSteps;
        }

        FastLED.clear(false);

        for (int i = 0; i < fillLevel; i++) {
            leds[i] = getColour(currentMillis, i);
        }

        if (fillDrop < NUM_LEDS) {
            leds[fillDrop] = getColour(currentMillis, fillDrop).scale8(175);
        }
        
        fillDrop = fillDrop - 1;

        if (fillDrop == fillLevel) {
            fillLevel = fillLevel + 1;
            fillDrop = NUM_LEDS;
        }

        FastLED.show();
        lastFillStep = currentMillis;
    }
}

int knightPos = 0;
bool knightAscend = true;
const int knightStep = 50;
unsigned long lastKnightStep = 0;

void doKnight(unsigned long currentMillis) {
    if (currentMillis - lastKnightStep > knightStep) {
        fadeToBlackBy(leds, NUM_LEDS, 90);

        if (knightAscend) {
            knightPos = knightPos + 1;

            if (knightPos > 7) {
                leds[ERROR_LED] = getColour(currentMillis, ERROR_LED);
                
                knightPos = 7;
                knightAscend = false;
            }
        }
        else {
            knightPos = knightPos - 1;

            if (knightPos < 0) { 
                knightPos = 0; 
                knightAscend = true;
            }
        }

        leds[knightPos] = getColour(currentMillis, knightPos);

        lastKnightStep = currentMillis;

        FastLED.show();
    }
}