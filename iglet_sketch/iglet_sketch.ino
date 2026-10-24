// ============================================================================
//  IGLET — A Cute Desk Companion & Focus Partner Gift for Her
//  Based on Project IGOR by Urban Circles (https://makerworld.com/en/models/1000048)
//
//  Hardware:
//    - Microcontroller: Wemos D1 Mini (ESP8266)
//    - Display:         Adafruit SSD1306 128x64 I2C OLED (Yellow/Blue dual color)
//    - Controller:      KY-040 Rotary Encoder with Push Button
//
//  Pinout:
//    - OLED SDA:        D2 (GPIO4)
//    - OLED SCL:        D1 (GPIO5)
//    - OLED VCC:        3.3V or 5V (check display specs)
//    - OLED GND:        GND
//    - Encoder CLK:     D6 (GPIO12)
//    - Encoder DT:      D7 (GPIO13)
//    - Encoder SW:      D4 (GPIO2)
//    - Encoder VCC:     3.3V
//    - Encoder GND:     GND
//
//  Required Libraries (install via Arduino Library Manager):
//    - Adafruit SSD1306 by Adafruit
//    - Adafruit GFX Library by Adafruit
// ============================================================================

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// ── Display ─────────────────────────────────────────────────────────────────
// Pass -1 for reset pin since standard 4-pin I2C OLED has no dedicated reset.
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ── Rotary Encoder Pins ─────────────────────────────────────────────────────
#define CLK    D6
#define DT     D7
#define SW     D4

// ============================================================================
//  PERSONALISE THESE FOR YOUR GIRLFRIEND!
// ============================================================================
#define GF_NAME       "Cutie"          // Her name or nickname (max ~10 chars)
#define DEVICE_NAME   "Iglet"          // Name of the device

// ── Love Notes: 20 notes she can browse, also shown during auto-wake ────────
// Stored as triplets: line1 (yellow zone), line2 (blue zone), separator "---"
const char* const loveNotes[] = {
  "You are my",    "sunshine!",    "---",
  "I love you",    "3000 <3",      "---",
  "You make me",   "so happy!",    "---",
  "Thinking of",   "you always",   "---",
  "You're the",    "best thing",   "---",
  "Smile! You",    "look cute :)", "---",
  "You got this",  "I believe!",   "---",
  "Miss you",      "right now <3", "---",
  "You deserve",   "all the love", "---",
  "My favorite",   "person ever!", "---",
  "Sending you",   "a big hug!",   "---",
  "Dream big,",    "my love <3",   "---",
  "You are so",    "so special!",  "---",
  "I'm lucky",     "to have you",  "---",
  "You light up",  "my world!",    "---",
  "Stay amazing",  "beautiful!",   "---",
  "I adore you",   "endlessly <3", "---",
  "Be kind to",    "yourself <3",  "---",
  "You make my",   "heart sing!",  "---",
  "Forever and",   "always <3",    "---"
};
const uint8_t LOVE_NOTE_COUNT = 20;

// ── Compliments for the Roulette ────────────────────────────────────────────
const char* const compliments[] = {
  "Amazing!",
  "Stunning!",
  "10/10 !!",
  "Gorgeous!",
  "Brilliant",
  "Adorable!",
  "Perfect!",
  "Kind soul",
  "So smart!",
  "My queen!",
  "So brave!",
  "A gem!",
  "Radiant!",
  "One & only",
  "Magical!",
  "Precious!"
};
const uint8_t COMPLIMENT_COUNT = 16;

// ── Decision Maker (Magic 8-Ball / Cute decider) ────────────────────────────
const char* const oracleAnswers[] = {
  "Yes!",
  "Nope!",
  "Maybe...",
  "Do it!",
  "Hmm, no",
  "100%!",
  "Ask later",
  "Oh yes!!",
  "Nah :P",
  "Go for it",
  "Not today",
  "Totally!",
  "Why not?",
  "Bad idea",
  "Trust me!",
  "Meh..."
};
const uint8_t ORACLE_COUNT = 16;

// ── Gentle Reminders ────────────────────────────────────────────────────────
// Stored as triplets: line1 (yellow zone), line2 (blue zone), separator "---"
const char* const reminderMsgs[] = {
  "Drink",        "water! :)",    "---",
  "Stretch",      "break! <3",    "---",
  "Deep",         "breath in",    "---",
  "You are",      "amazing <3",   "---",
  "Look up &",    "blink eyes",   "---",
  "Stand up",     "& shake it!",  "---"
};
const uint8_t REMINDER_COUNT = 6;

// ── Mood Check-in ───────────────────────────────────────────────────────────
const char* const moodLabels[] = {"Happy", "Okay", "Sad", "Angry", "Love!"};
const char* const moodResponses[][2] = {
  {"Yay! Keep",   "shining!"},
  {"That's ok!",  "You're great"},
  {"Sending a",   "big hug <3"},
  {"Take a deep", "breath <3"},
  {"Aww someone", "is in love!"}
};
const uint8_t MOOD_COUNT = 5;

// ============================================================================
//  STATE MACHINE
// ============================================================================
enum State {
  BOOT_ANIM,
  MENU,
  IDLE,
  // ── Original Focus Partner timer states ──
  COUNTING_UP,
  COUNTING_DOWN,
  SELECTING_DOWN_DURATION,
  SELECTING_POMO_FOCUS,
  SELECTING_POMO_REST,
  POMODORO_FOCUS,
  POMODORO_REST,
  // ── Iglet Companion states ──
  LOVE_NOTE_DISPLAY,
  COMPLIMENT_ROULETTE,
  COMPLIMENT_RESULT,
  DECISION_ORACLE,
  DECISION_RESULT,
  BREATHING_SELECT,
  BREATHING_EXERCISE,
  MOOD_SELECT,
  MOOD_RESPONSE,
  GREET_SELECT,
  REMINDER_DISPLAY,
  GREETING_WAKEUP
};
State currentState = BOOT_ANIM;

// ── Main Menu ───────────────────────────────────────────────────────────────
const char* const mainMenuItems[] = {
  "Notes",       // 0: Love notes
  "Spin!",       // 1: Compliment roulette
  "Decide",      // 2: Decision maker
  "Breathe",     // 3: Breathing exercise
  "Mood",        // 4: Mood check-in
  "Wake",        // 5: Auto-wake interval settings
  "UP",          // 6: Count up focus timer
  "DOWN",        // 7: Countdown focus timer
  "POMO",        // 8: Pomodoro focus timer
  "Reset"        // 9: Reset total flow minutes
};
const uint8_t MAIN_MENU_COUNT = 10;

// ── Focus Timer Variables ───────────────────────────────────────────────────
int flowMinutes = 0;
int menuIndex = 0;
int countdownValue = 20;
int initialCountdownValue = 20;
int pomodoroFocusMinutes = 25;
int pomodoroRestMinutes = 5;
int elapsedMinutes = 0;
bool isCounting = false;
unsigned long previousMillis = 0;

// ── Input Handling & Debounce ───────────────────────────────────────────────
unsigned long lastActivityTime = 0;
unsigned long buttonDebounceTime = 0;
const unsigned long buttonDebounceDelay = 400;   // Snappy, reliable button debounce
unsigned long lastRotaryTime = 0;
const unsigned long rotaryDebounceDelay = 120;   // Quadrature debounce

// ── Inactivity & Sleep ──────────────────────────────────────────────────────
const unsigned long inactivityLimit = 2UL * 60000UL;      // 2 min to sleep face
const unsigned long displayOffTimeLimit = 30UL * 60000UL; // 30 min to display off
unsigned long idleStartTime = 0;
bool displayOff = false;

// ── Auto-Wake Greeting Settings ─────────────────────────────────────────────
const uint16_t greetIntervals[] = {0, 30, 60, 120, 240};
const char* const greetLabels[] = {"Off", "30m", "1 hr", "2 hr", "4 hr"};
const uint8_t GREET_OPTION_COUNT = 5;
uint8_t greetIntervalIdx = 2;  // Default: 1 hour
unsigned long lastGreetingTime = 0;

// ── Gentle Reminder Settings ────────────────────────────────────────────────
bool remindersEnabled = true;
const unsigned long reminderIntervalMs = 45UL * 60000UL;  // Every 45 minutes
unsigned long lastReminderTime = 0;
uint8_t currentReminderIndex = 0;

// ── Feature Specific States ─────────────────────────────────────────────────
uint8_t currentNoteIndex = 0;

uint8_t rouletteIndex = 0;
unsigned long rouletteLastSpin = 0;
bool rouletteSpinning = false;

uint8_t decisionIndex = 0;
unsigned long decisionAnimStart = 0;

int breathCycles = 3;
int breathCurrentCycle = 0;
uint8_t breathPhase = 0;       // 0=in (4s), 1=hold (2s), 2=out (6s)
unsigned long breathPhaseStart = 0;
const unsigned long breathInMs = 4000;
const unsigned long breathHoldMs = 2000;
const unsigned long breathOutMs = 6000;

uint8_t moodSelection = 0;
unsigned long moodResponseStart = 0;

unsigned long bootStartTime = 0;
uint16_t rngState = 0;

// ── EEPROM Non-Volatile Storage ─────────────────────────────────────────────
#define EEPROM_SIZE        64
#define EEPROM_MAGIC_ADDR  0     // 2 bytes: 0xAC, 0xE1
#define EEPROM_FLOW_ADDR   2     // 4 bytes
#define EEPROM_GREET_ADDR  6     // 1 byte
#define EEPROM_REMIND_ADDR 7     // 1 byte
#define EEPROM_MAGIC_A     0xAC
#define EEPROM_MAGIC_B     0xE1

// ============================================================================
//  FORWARD DECLARATIONS
// ============================================================================
void updateDisplay();
void successAnimation();
int  getRotation();
bool buttonPressed();
void handleRotaryInput();
void handleButtonPresses(unsigned long now);
void handleMenuSelection(unsigned long now);
void handleCounting(unsigned long now);
void handleInactivity(unsigned long now);
void handleAutoGreeting(unsigned long now);
void handleReminders(unsigned long now);
void handleBreathingAnimation(unsigned long now);
void handleRouletteSpin(unsigned long now);
void handleDecisionSpin(unsigned long now);
void drawHeart(int cx, int cy, int size);
void drawSleepyFace();
void drawMoodFace(uint8_t mood, int cx, int cy, int r);
uint16_t cheapRandom(uint16_t maxVal);
void saveSettings();
void loadSettings();
void bootAnimation();
void renderMenu();
void renderIdle();
void renderTimerUp();
void renderTimerDown();
void renderPomoSelect(bool isFocus);
void renderPomoRunning(bool isFocus);
void renderLoveNote();
void renderCompliment();
void renderDecision();
void renderBreathSelect();
void renderMoodSelect();
void renderMoodResponse();
void renderGreetSelect();
void renderReminder();
void renderGreeting();

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("Starting Iglet..."));

  EEPROM.begin(EEPROM_SIZE);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring/address (0x3C/0x3D)"));
    for (;;);
  }

  display.clearDisplay();
  display.display();

  // Seed the pseudo-random generator from analog noise
  rngState = (uint16_t)(analogRead(A0) ^ millis());
  if (rngState == 0) rngState = 0x1337;

  loadSettings();

  lastActivityTime = millis();
  lastGreetingTime = millis();
  lastReminderTime = millis();
  bootStartTime    = millis();
  currentState     = BOOT_ANIM;

  Serial.println(F("Iglet setup complete!"));
}

// ============================================================================
//  MAIN LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();

  // Boot animation takes priority at power on
  if (currentState == BOOT_ANIM) {
    bootAnimation();
    return;
  }

  // Periodic events (only when device is in MENU or IDLE)
  handleAutoGreeting(now);
  handleReminders(now);

  // Non-blocking animation states
  if (currentState == BREATHING_EXERCISE) {
    handleBreathingAnimation(now);
  }

  if (currentState == COMPLIMENT_ROULETTE && rouletteSpinning) {
    handleRouletteSpin(now);
  }

  if (currentState == DECISION_ORACLE) {
    handleDecisionSpin(now);
  }

  // Auto-dismiss timeouts
  if (currentState == MOOD_RESPONSE && (now - moodResponseStart > 3500)) {
    currentState = MENU;
    updateDisplay();
  }

  if (currentState == COMPLIMENT_RESULT && (now - rouletteLastSpin > 4500)) {
    currentState = MENU;
    updateDisplay();
  }

  if (currentState == DECISION_RESULT && (now - decisionAnimStart > 4500)) {
    currentState = MENU;
    updateDisplay();
  }

  if (currentState == GREETING_WAKEUP && (now - lastGreetingTime > 8000)) {
    currentState = MENU;
    updateDisplay();
  }

  if (currentState == REMINDER_DISPLAY && (now - lastReminderTime > 10000)) {
    currentState = MENU;
    updateDisplay();
  }

  // Inputs and background handlers
  handleRotaryInput();
  handleButtonPresses(now);
  handleCounting(now);
  handleInactivity(now);
}

// ============================================================================
//  EEPROM STORAGE
// ============================================================================
void loadSettings() {
  uint8_t a = EEPROM.read(EEPROM_MAGIC_ADDR);
  uint8_t b = EEPROM.read(EEPROM_MAGIC_ADDR + 1);
  if (a == EEPROM_MAGIC_A && b == EEPROM_MAGIC_B) {
    flowMinutes = (int)EEPROM.read(EEPROM_FLOW_ADDR) |
                  ((int)EEPROM.read(EEPROM_FLOW_ADDR + 1) << 8) |
                  ((int)EEPROM.read(EEPROM_FLOW_ADDR + 2) << 16) |
                  ((int)EEPROM.read(EEPROM_FLOW_ADDR + 3) << 24);
    greetIntervalIdx = EEPROM.read(EEPROM_GREET_ADDR);
    if (greetIntervalIdx >= GREET_OPTION_COUNT) greetIntervalIdx = 2;
    remindersEnabled = EEPROM.read(EEPROM_REMIND_ADDR) != 0;
    Serial.println(F("Settings loaded from EEPROM."));
  } else {
    Serial.println(F("Using default settings."));
  }
}

void saveSettings() {
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_A);
  EEPROM.write(EEPROM_MAGIC_ADDR + 1, EEPROM_MAGIC_B);
  EEPROM.write(EEPROM_FLOW_ADDR, flowMinutes & 0xFF);
  EEPROM.write(EEPROM_FLOW_ADDR + 1, (flowMinutes >> 8) & 0xFF);
  EEPROM.write(EEPROM_FLOW_ADDR + 2, (flowMinutes >> 16) & 0xFF);
  EEPROM.write(EEPROM_FLOW_ADDR + 3, (flowMinutes >> 24) & 0xFF);
  EEPROM.write(EEPROM_GREET_ADDR, greetIntervalIdx);
  EEPROM.write(EEPROM_REMIND_ADDR, remindersEnabled ? 1 : 0);
  EEPROM.commit();
  Serial.println(F("Settings saved to EEPROM."));
}

// ============================================================================
//  PSEUDO-RANDOM NUMBER GENERATOR (xorshift16)
// ============================================================================
uint16_t cheapRandom(uint16_t maxVal) {
  if (maxVal == 0) return 0;
  rngState ^= rngState << 7;
  rngState ^= rngState >> 9;
  rngState ^= rngState << 8;
  if (rngState == 0) rngState = 1;
  return rngState % maxVal;
}

// ============================================================================
//  BOOT ANIMATION — "Hi [GF_NAME]!" + Beating Heart
// ============================================================================
void bootAnimation() {
  unsigned long elapsed = millis() - bootStartTime;

  if (elapsed < 600) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    String greeting = "Hi " + String(GF_NAME) + "!";
    int len = greeting.length();
    int tSize = (len > 9) ? 1 : 2;
    display.setTextSize(tSize);
    int charW = (tSize == 2) ? 12 : 6;
    int w = len * charW;
    display.setCursor((128 - w) / 2, (tSize == 2) ? 4 : 8);
    display.print(greeting);
    display.display();
  } else if (elapsed < 2600) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    String greeting = "Hi " + String(GF_NAME) + "!";
    int len = greeting.length();
    int tSize = (len > 9) ? 1 : 2;
    display.setTextSize(tSize);
    int charW = (tSize == 2) ? 12 : 6;
    int w = len * charW;
    display.setCursor((128 - w) / 2, (tSize == 2) ? 0 : 4);
    display.print(greeting);

    // Beating heart in blue zone
    int heartPhase = ((elapsed - 600) / 250) % 4;
    int heartSize = (heartPhase < 2) ? 9 + heartPhase * 3 : 15 - heartPhase * 2;
    drawHeart(64, 44, heartSize);

    display.display();
  } else {
    currentState = MENU;
    updateDisplay();
  }
}

// ============================================================================
//  GRAPHICS HELPERS
// ============================================================================
void drawHeart(int cx, int cy, int size) {
  int r = size / 3;
  if (r < 2) r = 2;
  display.fillCircle(cx - r, cy - r / 2, r, WHITE);
  display.fillCircle(cx + r, cy - r / 2, r, WHITE);
  display.fillRect(cx - r, cy - r / 2, 2 * r + 1, r / 2 + 1, WHITE);
  display.fillTriangle(
    cx - size / 2 - r / 2, cy,
    cx + size / 2 + r / 2, cy,
    cx, cy + size,
    WHITE
  );
}

void drawSleepyFace() {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(16, 32);
  display.print(F("(-_-)zzZ"));
}

void drawMoodFace(uint8_t mood, int cx, int cy, int r) {
  display.drawCircle(cx, cy, r, WHITE);
  int eyeY = cy - r / 3;
  int eyeSpacing = r / 2;

  switch (mood) {
    case 0: // Happy :)
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        int sy = cy + r / 4 + (i * i) / (r * 2);
        display.drawPixel(cx + i, sy, WHITE);
      }
      break;

    case 1: // Okay :|
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      display.drawLine(cx - eyeSpacing, cy + r / 4, cx + eyeSpacing, cy + r / 4, WHITE);
      break;

    case 2: // Sad :(
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        int sy = cy + r / 2 - (i * i) / (r * 2);
        display.drawPixel(cx + i, sy, WHITE);
      }
      break;

    case 3: // Angry >:(
      display.drawLine(cx - eyeSpacing - 2, eyeY - 4, cx - eyeSpacing + 2, eyeY - 2, WHITE);
      display.drawLine(cx + eyeSpacing + 2, eyeY - 4, cx + eyeSpacing - 2, eyeY - 2, WHITE);
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        int sy = cy + r / 2 - (i * i) / (r * 2);
        display.drawPixel(cx + i, sy, WHITE);
      }
      break;

    case 4: // Love <3
      drawHeart(cx - eyeSpacing, eyeY, 4);
      drawHeart(cx + eyeSpacing, eyeY, 4);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        int sy = cy + r / 4 + (i * i) / r;
        display.drawPixel(cx + i, sy, WHITE);
      }
      break;
  }
}

// ============================================================================
//  MAIN DISPLAY DISPATCHER
// ============================================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (currentState) {
    case MENU:
      renderMenu();
      break;
    case IDLE:
      renderIdle();
      break;
    case COUNTING_UP:
      renderTimerUp();
      break;
    case COUNTING_DOWN:
    case SELECTING_DOWN_DURATION:
      renderTimerDown();
      break;
    case SELECTING_POMO_FOCUS:
      renderPomoSelect(true);
      break;
    case SELECTING_POMO_REST:
      renderPomoSelect(false);
      break;
    case POMODORO_FOCUS:
      renderPomoRunning(true);
      break;
    case POMODORO_REST:
      renderPomoRunning(false);
      break;
    case LOVE_NOTE_DISPLAY:
      renderLoveNote();
      break;
    case COMPLIMENT_ROULETTE:
    case COMPLIMENT_RESULT:
      renderCompliment();
      break;
    case DECISION_ORACLE:
    case DECISION_RESULT:
      renderDecision();
      break;
    case BREATHING_SELECT:
      renderBreathSelect();
      break;
    case BREATHING_EXERCISE:
      // Rendered continuously in handleBreathingAnimation
      break;
    case MOOD_SELECT:
      renderMoodSelect();
      break;
    case MOOD_RESPONSE:
      renderMoodResponse();
      break;
    case GREET_SELECT:
      renderGreetSelect();
      break;
    case REMINDER_DISPLAY:
      renderReminder();
      break;
    case GREETING_WAKEUP:
      renderGreeting();
      break;
    default:
      break;
  }

  display.display();
}

// ── Render: Main Menu ───────────────────────────────────────────────────────
void renderMenu() {
  // Yellow zone: Flow time on left, Auto-Wake interval on right
  display.setTextSize(1);
  String flowStr;
  if (flowMinutes < 1000) {
    flowStr = "Flow:" + String(flowMinutes) + "m";
  } else {
    flowStr = "F:" + String(flowMinutes / 60) + "h";
  }
  display.setCursor(0, 4);
  display.print(flowStr);

  display.setCursor(88, 4);
  if (greetIntervals[greetIntervalIdx] > 0) {
    display.print(greetLabels[greetIntervalIdx]);
  } else {
    display.print(F("Off"));
  }

  // Blue zone: Large current menu item
  display.setTextSize(3);
  String item = mainMenuItems[menuIndex];
  int w = item.length() * 18;
  display.setCursor((128 - w) / 2, 26);
  display.print(item);

  // Bottom navigation dots
  display.setTextSize(1);
  int dotsWidth = MAIN_MENU_COUNT * 6;
  int dotsX = (128 - dotsWidth) / 2;
  for (uint8_t i = 0; i < MAIN_MENU_COUNT; i++) {
    if (i == menuIndex) {
      display.fillCircle(dotsX + i * 6 + 2, 60, 2, WHITE);
    } else {
      display.drawCircle(dotsX + i * 6 + 2, 60, 1, WHITE);
    }
  }
}

// ── Render: Idle ────────────────────────────────────────────────────────────
void renderIdle() {
  display.setTextSize(1);
  display.setCursor(26, 4);
  display.print(DEVICE_NAME);
  display.print(F(" sleeping"));
  drawSleepyFace();
}

// ── Render: Timer Up ────────────────────────────────────────────────────────
void renderTimerUp() {
  display.setTextSize(2);
  String topText = "Focus! \x18";
  int tw = topText.length() * 12;
  display.setCursor((128 - tw) / 2, 0);
  display.print(topText);

  display.setTextSize(4);
  String mins = String(elapsedMinutes);
  int mw = mins.length() * 24;
  display.setCursor((128 - mw) / 2, 30);
  display.print(mins);
}

// ── Render: Timer Down / Select Duration ────────────────────────────────────
void renderTimerDown() {
  display.setTextSize(2);
  String topText = (currentState == SELECTING_DOWN_DURATION) ? "Set time" : "Focus! \x19";
  int tw = topText.length() * 12;
  display.setCursor((128 - tw) / 2, 0);
  display.print(topText);

  display.setTextSize(4);
  String mins = String(countdownValue);
  int mw = mins.length() * 24;
  display.setCursor((128 - mw) / 2, 30);
  display.print(mins);
}

// ── Render: Pomodoro Select ─────────────────────────────────────────────────
void renderPomoSelect(bool isFocus) {
  display.setTextSize(2);
  String topText = isFocus ? "Set Focus" : "Set Rest";
  int tw = topText.length() * 12;
  display.setCursor((128 - tw) / 2, 0);
  display.print(topText);

  display.setTextSize(4);
  String mins = String(countdownValue);
  int mw = mins.length() * 24;
  display.setCursor((128 - mw) / 2, 30);
  display.print(mins);
}

// ── Render: Pomodoro Running ────────────────────────────────────────────────
void renderPomoRunning(bool isFocus) {
  display.setTextSize(2);
  String topText = isFocus ? "Focus! \x19" : "Rest! <3";
  int tw = topText.length() * 12;
  display.setCursor((128 - tw) / 2, 0);
  display.print(topText);

  display.setTextSize(4);
  String mins = String(countdownValue);
  int mw = mins.length() * 24;
  display.setCursor((128 - mw) / 2, 30);
  display.print(mins);
}

// ── Render: Love Notes ──────────────────────────────────────────────────────
void renderLoveNote() {
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.print(F("To "));
  display.print(GF_NAME);
  display.print(F(" <3"));

  display.setCursor(96, 4);
  display.print(currentNoteIndex + 1);
  display.print(F("/"));
  display.print(LOVE_NOTE_COUNT);

  uint8_t idx = (currentNoteIndex % LOVE_NOTE_COUNT) * 3;

  display.setTextSize(2);
  const char* line1 = loveNotes[idx];
  int w1 = strlen(line1) * 12;
  display.setCursor((128 - w1) / 2, 22);
  display.print(line1);

  display.setTextSize(2);
  const char* line2 = loveNotes[idx + 1];
  int w2 = strlen(line2) * 12;
  display.setCursor((128 - w2) / 2, 42);
  display.print(line2);
}

// ── Render: Compliment ──────────────────────────────────────────────────────
void renderCompliment() {
  display.setTextSize(2);
  display.setCursor(18, 0);
  display.print(F("Spin me!"));

  const char* comp = compliments[rouletteIndex % COMPLIMENT_COUNT];
  int len = strlen(comp);
  int textSize = (len > 7) ? 2 : 3;
  display.setTextSize(textSize);
  int charW = (textSize == 3) ? 18 : 12;
  int w = len * charW;
  display.setCursor((128 - w) / 2, (textSize == 3) ? 28 : 34);
  display.print(comp);

  if (currentState == COMPLIMENT_RESULT) {
    display.drawPixel(10, 24, WHITE);
    display.drawPixel(118, 24, WHITE);
    display.drawPixel(15, 54, WHITE);
    display.drawPixel(113, 54, WHITE);
    drawHeart(10, 38, 4);
    drawHeart(118, 38, 4);
  }
}

// ── Render: Decision ────────────────────────────────────────────────────────
void renderDecision() {
  display.setTextSize(2);
  display.setCursor(14, 0);
  display.print(F("Ask me!"));

  if (currentState == DECISION_RESULT) {
    const char* answer = oracleAnswers[decisionIndex % ORACLE_COUNT];
    int len = strlen(answer);
    int textSize = (len > 7) ? 2 : 3;
    display.setTextSize(textSize);
    int charW = (textSize == 3) ? 18 : 12;
    int w = len * charW;
    display.setCursor((128 - w) / 2, (textSize == 3) ? 28 : 34);
    display.print(answer);
  } else {
    display.setTextSize(4);
    display.setCursor(52, 26);
    display.print(F("?"));
  }
}

// ── Render: Breathing Select ────────────────────────────────────────────────
void renderBreathSelect() {
  display.setTextSize(2);
  display.setCursor(14, 0);
  display.print(F("Breathe"));

  display.setTextSize(1);
  display.setCursor(30, 22);
  display.print(F("Cycles:"));

  display.setTextSize(3);
  String cyc = String(breathCycles);
  int w = cyc.length() * 18;
  display.setCursor((128 - w) / 2, 36);
  display.print(cyc);
}

// ── Render: Mood Select ─────────────────────────────────────────────────────
void renderMoodSelect() {
  display.setTextSize(2);
  display.setCursor(20, 0);
  display.print(F("How r u?"));

  drawMoodFace(moodSelection, 64, 38, 12);

  display.setTextSize(1);
  const char* label = moodLabels[moodSelection];
  int w = strlen(label) * 6;
  display.setCursor((128 - w) / 2, 56);
  display.print(label);
}

// ── Render: Mood Response ───────────────────────────────────────────────────
void renderMoodResponse() {
  display.setTextSize(2);
  const char* r1 = moodResponses[moodSelection][0];
  int w1 = strlen(r1) * 12;
  display.setCursor((128 - w1) / 2, 4);
  display.print(r1);

  display.setTextSize(2);
  const char* r2 = moodResponses[moodSelection][1];
  int w2 = strlen(r2) * 12;
  display.setCursor((128 - w2) / 2, 34);
  display.print(r2);
}

// ── Render: Auto-Wake Interval Setting ──────────────────────────────────────
void renderGreetSelect() {
  display.setTextSize(2);
  display.setCursor(14, 0);
  display.print(F("Auto Wake"));

  display.setTextSize(1);
  display.setCursor(28, 22);
  display.print(F("Wake every:"));

  display.setTextSize(3);
  const char* lbl = greetLabels[greetIntervalIdx];
  int w = strlen(lbl) * 18;
  display.setCursor((128 - w) / 2, 36);
  display.print(lbl);
}

// ── Render: Gentle Reminder ─────────────────────────────────────────────────
void renderReminder() {
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.print(F("Gentle Reminder <3"));

  uint8_t rIdx = (currentReminderIndex % REMINDER_COUNT) * 3;
  display.setTextSize(2);
  const char* r1 = reminderMsgs[rIdx];
  int w1 = strlen(r1) * 12;
  display.setCursor((128 - w1) / 2, 22);
  display.print(r1);

  display.setTextSize(2);
  const char* r2 = reminderMsgs[rIdx + 1];
  int w2 = strlen(r2) * 12;
  display.setCursor((128 - w2) / 2, 42);
  display.print(r2);
}

// ── Render: Auto-Wake Greeting ──────────────────────────────────────────────
void renderGreeting() {
  renderLoveNote();
}

// ============================================================================
//  ANIMATION HANDLERS (non-blocking)
// ============================================================================
void handleRouletteSpin(unsigned long now) {
  unsigned long elapsed = now - rouletteLastSpin;
  unsigned long interval;
  if (elapsed < 500)       interval = 50;
  else if (elapsed < 1000) interval = 100;
  else if (elapsed < 1600) interval = 220;
  else if (elapsed < 2200) interval = 400;
  else {
    rouletteSpinning = false;
    currentState = COMPLIMENT_RESULT;
    rouletteLastSpin = now;
    updateDisplay();
    return;
  }

  static unsigned long lastFrame = 0;
  if (now - lastFrame >= interval) {
    lastFrame = now;
    rouletteIndex = (rouletteIndex + 1) % COMPLIMENT_COUNT;
    updateDisplay();
  }
}

void handleDecisionSpin(unsigned long now) {
  unsigned long elapsed = now - decisionAnimStart;

  if (elapsed < 1500) {
    static unsigned long lastFrame = 0;
    if (now - lastFrame >= 90) {
      lastFrame = now;
      decisionIndex = (decisionIndex + 1) % ORACLE_COUNT;
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(14, 0);
      display.print(F("Thinking"));

      display.setTextSize(4);
      display.setCursor(52, 26);
      int phase = (elapsed / 160) % 3;
      if (phase == 0) display.print(F("?"));
      else if (phase == 1) display.print(F("!"));
      else display.print(F("*"));
      display.display();
    }
  } else {
    currentState = DECISION_RESULT;
    decisionIndex = cheapRandom(ORACLE_COUNT);
    decisionAnimStart = now;
    updateDisplay();
  }
}

void handleBreathingAnimation(unsigned long now) {
  unsigned long phaseElapsed = now - breathPhaseStart;
  unsigned long phaseDuration;
  const char* phaseLabel;

  switch (breathPhase) {
    case 0:  phaseDuration = breathInMs;   phaseLabel = "Breathe in";  break;
    case 1:  phaseDuration = breathHoldMs; phaseLabel = "Hold...";     break;
    case 2:  phaseDuration = breathOutMs;  phaseLabel = "Breathe out"; break;
    default: phaseDuration = breathInMs;   phaseLabel = "Breathe in";  break;
  }

  if (phaseElapsed >= phaseDuration) {
    breathPhase++;
    if (breathPhase > 2) {
      breathPhase = 0;
      breathCurrentCycle++;
      if (breathCurrentCycle >= breathCycles) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(14, 4);
        display.print(F("Well done!"));
        display.setTextSize(2);
        display.setCursor(14, 34);
        display.print(F("<3 <3 <3"));
        display.display();
        delay(2000);
        currentState = MENU;
        updateDisplay();
        return;
      }
    }
    breathPhaseStart = now;
    phaseElapsed = 0;
  }

  float progress = (float)phaseElapsed / (float)phaseDuration;

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  int lw = strlen(phaseLabel) * 6;
  display.setCursor((128 - lw) / 2, 4);
  display.print(phaseLabel);

  display.setCursor(100, 4);
  display.print(breathCurrentCycle + 1);
  display.print(F("/"));
  display.print(breathCycles);

  int minR = 5;
  int maxR = 21;
  int radius;

  if (breathPhase == 0) {
    radius = minR + (int)((maxR - minR) * progress);
  } else if (breathPhase == 1) {
    radius = maxR;
  } else {
    radius = maxR - (int)((maxR - minR) * progress);
  }

  display.drawCircle(64, 42, radius, WHITE);
  display.drawCircle(64, 42, radius - 1, WHITE);
  display.fillCircle(64, 42, 2, WHITE);

  display.display();
}

// ============================================================================
//  AUTO-WAKE GREETING DISPATCHER
// ============================================================================
void handleAutoGreeting(unsigned long now) {
  if (greetIntervals[greetIntervalIdx] == 0) return;
  if (currentState != MENU && currentState != IDLE) return;

  unsigned long intervalMs = (unsigned long)greetIntervals[greetIntervalIdx] * 60000UL;
  if (now - lastGreetingTime >= intervalMs) {
    lastGreetingTime = now;
    currentNoteIndex = cheapRandom(LOVE_NOTE_COUNT);

    if (displayOff) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displayOff = false;
    }

    currentState = GREETING_WAKEUP;
    lastActivityTime = now;
    updateDisplay();
    Serial.println(F("Auto greeting displayed!"));
  }
}

// ============================================================================
//  GENTLE REMINDERS DISPATCHER
// ============================================================================
void handleReminders(unsigned long now) {
  if (!remindersEnabled) return;
  if (currentState != MENU && currentState != IDLE) return;

  if (now - lastReminderTime >= reminderIntervalMs) {
    lastReminderTime = now;
    currentReminderIndex = cheapRandom(REMINDER_COUNT);

    if (displayOff) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displayOff = false;
    }

    currentState = REMINDER_DISPLAY;
    lastActivityTime = now;
    updateDisplay();
    Serial.println(F("Gentle reminder displayed!"));
  }
}

// ============================================================================
//  INPUT HANDLING
// ============================================================================
int getRotation() {
  static int previousCLK = digitalRead(CLK);
  int currentCLK = digitalRead(CLK);

  if (currentCLK == LOW && previousCLK == HIGH &&
      (millis() - lastRotaryTime > rotaryDebounceDelay)) {
    lastRotaryTime = millis();
    int DTValue = digitalRead(DT);
    previousCLK = currentCLK;
    return (DTValue != currentCLK) ? 1 : -1;
  }

  previousCLK = currentCLK;
  return 0;
}

bool buttonPressed() {
  if (digitalRead(SW) == LOW && (millis() - buttonDebounceTime > buttonDebounceDelay)) {
    buttonDebounceTime = millis();
    lastActivityTime = millis();
    return true;
  }
  return false;
}

void handleRotaryInput() {
  int rotation = getRotation();
  if (rotation == 0) return;

  lastActivityTime = millis();

  switch (currentState) {
    case MENU:
      menuIndex = (menuIndex + rotation + MAIN_MENU_COUNT) % MAIN_MENU_COUNT;
      updateDisplay();
      break;

    case IDLE:
    case GREETING_WAKEUP:
    case REMINDER_DISPLAY:
      currentState = MENU;
      if (displayOff) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        displayOff = false;
      }
      updateDisplay();
      break;

    case SELECTING_DOWN_DURATION:
      countdownValue = max(1, countdownValue + rotation);
      updateDisplay();
      break;

    case SELECTING_POMO_FOCUS:
    case SELECTING_POMO_REST:
      countdownValue = max(1, countdownValue + rotation);
      updateDisplay();
      break;

    case LOVE_NOTE_DISPLAY:
      currentNoteIndex = (currentNoteIndex + rotation + LOVE_NOTE_COUNT) % LOVE_NOTE_COUNT;
      updateDisplay();
      break;

    case COMPLIMENT_ROULETTE:
      if (!rouletteSpinning) {
        rouletteSpinning = true;
        rouletteLastSpin = millis();
        rouletteIndex = cheapRandom(COMPLIMENT_COUNT);
      }
      break;

    case BREATHING_SELECT:
      breathCycles = max(1, min(20, breathCycles + rotation));
      updateDisplay();
      break;

    case MOOD_SELECT:
      moodSelection = (moodSelection + rotation + MOOD_COUNT) % MOOD_COUNT;
      updateDisplay();
      break;

    case GREET_SELECT:
      greetIntervalIdx = (greetIntervalIdx + rotation + GREET_OPTION_COUNT) % GREET_OPTION_COUNT;
      updateDisplay();
      break;

    default:
      break;
  }
}

void handleButtonPresses(unsigned long now) {
  if (!buttonPressed()) return;

  switch (currentState) {
    case MENU:
      handleMenuSelection(now);
      break;

    case IDLE:
    case GREETING_WAKEUP:
    case REMINDER_DISPLAY:
    case COMPLIMENT_RESULT:
    case DECISION_RESULT:
    case MOOD_RESPONSE:
      currentState = MENU;
      if (displayOff) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        displayOff = false;
      }
      break;

    case LOVE_NOTE_DISPLAY:
      currentState = MENU;
      break;

    case COMPLIMENT_ROULETTE:
      if (!rouletteSpinning) {
        rouletteSpinning = true;
        rouletteLastSpin = now;
        rouletteIndex = cheapRandom(COMPLIMENT_COUNT);
      }
      return;

    case DECISION_ORACLE:
      return;

    case BREATHING_SELECT:
      breathCurrentCycle = 0;
      breathPhase = 0;
      breathPhaseStart = now;
      currentState = BREATHING_EXERCISE;
      break;

    case BREATHING_EXERCISE:
      currentState = MENU;
      break;

    case MOOD_SELECT:
      currentState = MOOD_RESPONSE;
      moodResponseStart = now;
      break;

    case GREET_SELECT:
      saveSettings();
      currentState = MENU;
      break;

    // ── Timer States ──
    case SELECTING_DOWN_DURATION:
      initialCountdownValue = countdownValue;
      currentState = COUNTING_DOWN;
      isCounting = true;
      previousMillis = now;
      break;

    case SELECTING_POMO_FOCUS:
      pomodoroFocusMinutes = countdownValue;
      currentState = SELECTING_POMO_REST;
      countdownValue = pomodoroRestMinutes;
      break;

    case SELECTING_POMO_REST:
      pomodoroRestMinutes = countdownValue;
      currentState = POMODORO_FOCUS;
      countdownValue = pomodoroFocusMinutes;
      isCounting = true;
      previousMillis = now;
      break;

    case COUNTING_UP:
      flowMinutes += elapsedMinutes;
      saveSettings();
      successAnimation();
      currentState = MENU;
      isCounting = false;
      break;

    case COUNTING_DOWN:
      flowMinutes += (initialCountdownValue - countdownValue);
      saveSettings();
      successAnimation();
      currentState = MENU;
      isCounting = false;
      break;

    case POMODORO_FOCUS:
      flowMinutes += (pomodoroFocusMinutes - countdownValue);
      saveSettings();
      successAnimation();
      currentState = MENU;
      isCounting = false;
      break;

    case POMODORO_REST:
      successAnimation();
      currentState = MENU;
      isCounting = false;
      break;

    default:
      break;
  }

  updateDisplay();
}

// ── Menu Selection Handler ──────────────────────────────────────────────────
void handleMenuSelection(unsigned long now) {
  switch (menuIndex) {
    case 0:  // Notes — Love notes
      currentNoteIndex = cheapRandom(LOVE_NOTE_COUNT);
      currentState = LOVE_NOTE_DISPLAY;
      break;

    case 1:  // Spin! — Compliment roulette
      rouletteSpinning = false;
      rouletteIndex = 0;
      currentState = COMPLIMENT_ROULETTE;
      break;

    case 2:  // Decide — Decision maker
      currentState = DECISION_ORACLE;
      decisionAnimStart = now;
      decisionIndex = 0;
      break;

    case 3:  // Breathe — Breathing exercise
      breathCycles = 3;
      currentState = BREATHING_SELECT;
      break;

    case 4:  // Mood — Mood check-in
      moodSelection = 0;
      currentState = MOOD_SELECT;
      break;

    case 5:  // Wake — Auto-wake greeting interval
      currentState = GREET_SELECT;
      break;

    case 6:  // UP — Count up focus timer
      currentState = COUNTING_UP;
      elapsedMinutes = 0;
      isCounting = true;
      previousMillis = now;
      break;

    case 7:  // DOWN — Countdown focus timer
      currentState = SELECTING_DOWN_DURATION;
      countdownValue = 20;
      break;

    case 8:  // POMO — Pomodoro focus timer
      currentState = SELECTING_POMO_FOCUS;
      pomodoroFocusMinutes = 25;
      pomodoroRestMinutes = 5;
      countdownValue = pomodoroFocusMinutes;
      break;

    case 9:  // Reset — Reset total focus minutes
      flowMinutes = 0;
      saveSettings();
      break;
  }
}

// ============================================================================
//  TIMER COUNTING ENGINE
// ============================================================================
void handleCounting(unsigned long now) {
  if (!isCounting || (now - previousMillis < 60000UL)) return;
  previousMillis = now;

  if (currentState == COUNTING_UP) {
    elapsedMinutes++;
    updateDisplay();
  } else if (currentState == COUNTING_DOWN) {
    countdownValue--;
    if (countdownValue <= 0) {
      flowMinutes += initialCountdownValue;
      saveSettings();
      successAnimation();
      currentState = MENU;
      isCounting = false;
    }
    updateDisplay();
  } else if (currentState == POMODORO_FOCUS) {
    countdownValue--;
    if (countdownValue <= 0) {
      flowMinutes += pomodoroFocusMinutes;
      saveSettings();
      successAnimation();
      currentState = POMODORO_REST;
      countdownValue = pomodoroRestMinutes;
    }
    updateDisplay();
  } else if (currentState == POMODORO_REST) {
    countdownValue--;
    if (countdownValue <= 0) {
      successAnimation();
      currentState = POMODORO_FOCUS;
      countdownValue = pomodoroFocusMinutes;
    }
    updateDisplay();
  }
}

// ============================================================================
//  SUCCESS ANIMATION (Session finished)
// ============================================================================
void successAnimation() {
  display.clearDisplay();
  int centerX = 64, centerY = 32;

  // Expanding ripple circles
  for (int radius = 2; radius <= 30; radius += 2) {
    display.drawCircle(centerX, centerY, radius, WHITE);
    display.display();
    delay(75);
    if (radius % 4 == 0) {
      display.clearDisplay();
      display.display();
      delay(2);
    }
  }

  // Heart finale
  display.clearDisplay();
  drawHeart(64, 26, 16);
  display.setTextSize(1);
  display.setCursor(42, 50);
  display.print(F("Yay! <3"));
  display.display();
  delay(1400);
  display.clearDisplay();
  display.display();
}

// ============================================================================
//  INACTIVITY & POWER SAVING
// ============================================================================
void handleInactivity(unsigned long now) {
  bool mayIdle = (currentState == MENU || currentState == SELECTING_DOWN_DURATION ||
                  currentState == SELECTING_POMO_FOCUS || currentState == SELECTING_POMO_REST ||
                  currentState == LOVE_NOTE_DISPLAY || currentState == MOOD_SELECT ||
                  currentState == BREATHING_SELECT || currentState == GREET_SELECT);

  if (mayIdle && (now - lastActivityTime > inactivityLimit)) {
    if (currentState != IDLE) {
      currentState = IDLE;
      idleStartTime = now;
      updateDisplay();
      Serial.println(F("Entered IDLE state."));
    }
  }

  if (currentState == IDLE && !displayOff &&
      (now - idleStartTime > displayOffTimeLimit)) {
    displayOff = true;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    Serial.println(F("Display turned off."));
  }
}
