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
//  Libraries (built-in to ESP8266 Core & Adafruit):
//    - ESP8266WiFi, ESP8266WebServer, DNSServer, ESP8266mDNS, time.h
//    - Adafruit SSD1306, Adafruit GFX, Wire, EEPROM
// ============================================================================

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// ── Display ─────────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ── Rotary Encoder Pins ─────────────────────────────────────────────────────
#define CLK    D6
#define DT     D7
#define SW     D4

// ============================================================================
//  PERSONALIZATION (Private config or Public defaults)
// ============================================================================
#if __has_include("config.h")
  #include "config.h"
#else
  #define GF_NAME             "Cutie"
  #define DEVICE_NAME         "Iglet"
  #define WIFI_SSID           ""
  #define WIFI_PASS           ""
  #define TIMEZONE_OFFSET_HRS 5.5
  #define ANNIVERSARY_MONTH   0
  #define ANNIVERSARY_DAY     0
  #define GF_BIRTHDAY_MONTH   0
  #define GF_BIRTHDAY_DAY     0

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

  const char* const compliments[] = {
    "Amazing!", "Stunning!", "10/10 !!", "Gorgeous!",
    "Brilliant", "Adorable!", "Perfect!", "Kind soul",
    "So smart!", "My queen!", "So brave!", "A gem!",
    "Radiant!", "One & only", "Magical!", "Precious!"
  };
  const uint8_t COMPLIMENT_COUNT = 16;

  const char* const oracleAnswers[] = {
    "Yes!", "Nope!", "Maybe...", "Do it!",
    "Hmm, no", "100%!", "Ask later", "Oh yes!!",
    "Nah :P", "Go for it", "Not today", "Totally!",
    "Why not?", "Bad idea", "Trust me!", "Meh..."
  };
  const uint8_t ORACLE_COUNT = 16;
#endif

// Fallbacks if not defined in config.h
#ifndef TELEGRAM_BOT_TOKEN
  #define TELEGRAM_BOT_TOKEN ""
#endif
#ifndef TELEGRAM_CHAT_ID
  #define TELEGRAM_CHAT_ID   ""
#endif

// ── Gentle Reminders ────────────────────────────────────────────────────────
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
//  DYNAMIC CONFIGURATION (Stored in EEPROM, editable wirelessly via Phone!)
// ============================================================================
char cfgGfName[32]        = GF_NAME;
char cfgWifiSSID[64]      = WIFI_SSID;
char cfgWifiPass[64]      = WIFI_PASS;
uint8_t cfgBdayMonth      = GF_BIRTHDAY_MONTH;
uint8_t cfgBdayDay        = GF_BIRTHDAY_DAY;
uint8_t cfgAnnivMonth     = ANNIVERSARY_MONTH;
uint8_t cfgAnnivDay       = ANNIVERSARY_DAY;
float cfgTzOffset         = TIMEZONE_OFFSET_HRS;
char cfgTelegramToken[80] = TELEGRAM_BOT_TOKEN;
char cfgTelegramChatId[32]= TELEGRAM_CHAT_ID;
unsigned long lastTelegramPoll = 0;
const unsigned long telegramPollInterval = 10000; // 10s idle check
long lastTelegramUpdateId = 0;
bool telegramInitialized = false;

// ── Hardware Interrupt Flags ────────────────────────────────────────────────
volatile int rotaryDelta = 0;
volatile bool buttonWasClicked = false;

// ============================================================================
//  WEB SERVER & NETWORKING
// ============================================================================
ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
bool apModeActive = false;
bool wifiConnecting = false;
unsigned long wifiConnectStart = 0;
bool ntpConfigured = false;

// ── Live Note from Phone ────────────────────────────────────────────────────
char liveNoteLine1[24] = "";
char liveNoteLine2[24] = "";
unsigned long liveNoteStart = 0;

// ============================================================================
//  STATE MACHINE
// ============================================================================
enum State {
  BOOT_ANIM,
  MENU,
  IDLE,
  // ── Focus Partner Timers ──
  COUNTING_UP,
  COUNTING_DOWN,
  SELECTING_DOWN_DURATION,
  SELECTING_POMO_FOCUS,
  SELECTING_POMO_REST,
  POMODORO_FOCUS,
  POMODORO_REST,
  // ── Companion Features ──
  LOVE_NOTE_DISPLAY,
  PET_VIEW,
  DAYS_VIEW,             // Couple's Countdown Timer & Live Clock
  COMPLIMENT_ROULETTE,
  COMPLIMENT_RESULT,
  DECISION_ORACLE,
  DECISION_RESULT,
  BREATHING_SELECT,
  BREATHING_EXERCISE,
  MOOD_SELECT,
  MOOD_RESPONSE,
  WIFI_PORTAL_VIEW,      // Wi-Fi Status & Phone Setup Portal
  GREET_SELECT,
  REMINDER_DISPLAY,
  GREETING_WAKEUP,
  GREETING_TEASER,       // Teaser for upcoming birthday/anniversary or celebration on auto-wake
  LIVE_NOTE_DISPLAY,     // Note sent live from phone!
  CELEBRATION_DISPLAY    // Birthday or Anniversary celebration!
};
State currentState = BOOT_ANIM;

// ── Main Menu ───────────────────────────────────────────────────────────────
const char* const mainMenuItems[] = {
  "Pet",         // 0: Virtual Pet (Tamagotchi-lite)
  "Days",        // 1: Event Countdowns (Anniversary, Birthday, Clock)
  "Spin!",       // 2: Compliment roulette
  "Decide",      // 3: Decision maker
  "Breathe",     // 4: Breathing exercise
  "Mood",        // 5: Mood check-in
  "WiFi",        // 6: Wi-Fi status / Web Portal
  "Wake",        // 7: Auto-wake greeting interval
  "UP",          // 8: Count up focus timer
  "DOWN",        // 9: Countdown focus timer
  "POMO",        // 10: Pomodoro focus timer
  "Reset"        // 11: Reset total flow minutes
};
const uint8_t MAIN_MENU_COUNT = 12;

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
const unsigned long buttonDebounceDelay = 400;
unsigned long buttonIgnoreUntil = 0;
unsigned long lastRotaryTime = 0;
const unsigned long rotaryDebounceDelay = 120;

// ── Inactivity & Sleep ──────────────────────────────────────────────────────
const unsigned long inactivityLimit = 5UL * 60000UL;      // 5 min to sleep (generous & comfortable)
const unsigned long displayOffTimeLimit = 30UL * 60000UL; // 30 min to display off
unsigned long idleStartTime = 0;
bool displayOff = false;

// ── Auto-Wake Greeting Settings ─────────────────────────────────────────────
const uint16_t greetIntervals[] = {0, 1, 30, 60, 120, 240};
const char* const greetLabels[] = {"Off", "1m", "30m", "1 hr", "2 hr", "4 hr"};
const uint8_t GREET_OPTION_COUNT = 6;
uint8_t greetIntervalIdx = 3;  // Default: 1 hour
unsigned long lastGreetingTime = 0;

// Teaser for upcoming or today's anniversary/birthday on auto-wake
uint8_t teaserType = 0; // 1: Bday today, 2: Anniv today, 3: Bday next week, 4: Anniv next week
uint8_t teaserDays = 0;
unsigned long teaserStart = 0;

// ── Gentle Reminder Settings ────────────────────────────────────────────────
bool remindersEnabled = true;
const unsigned long reminderIntervalMs = 45UL * 60000UL;
unsigned long lastReminderTime = 0;
uint8_t currentReminderIndex = 0;

// ── Feature States ──────────────────────────────────────────────────────────
uint8_t currentNoteIndex = 0;

// Pet
uint8_t petLoveLevel = 3;       // 1 to 5 hearts
uint8_t petActionIndex = 0;     // 0=Pet, 1=Feed, 2=Play, 3=Exit
uint8_t petAnimState = 0;       // 0=idle, 1=petting, 2=eating, 3=playing
unsigned long petAnimStart = 0;
const char* const petActionLabels[] = {"Pet", "Feed", "Play", "Exit"};
const uint8_t PET_ACTION_COUNT = 4;

// Days countdown view
uint8_t daysSubIndex = 0;       // 0=Anniversary, 1=Birthday, 2=Clock

// Compliment roulette
uint8_t rouletteIndex = 0;
unsigned long rouletteLastSpin = 0;
bool rouletteSpinning = false;

// Decision maker
uint8_t decisionIndex = 0;
unsigned long decisionAnimStart = 0;

// Breathing
int breathCycles = 3;
int breathCurrentCycle = 0;
uint8_t breathPhase = 0;
unsigned long breathPhaseStart = 0;
const unsigned long breathInMs = 4000;
const unsigned long breathHoldMs = 2000;
const unsigned long breathOutMs = 6000;

// Mood
uint8_t moodSelection = 0;
unsigned long moodResponseStart = 0;

// Boot
unsigned long bootStartTime = 0;
uint16_t rngState = 0;

// Celebration type: 1=Birthday, 2=Anniversary
uint8_t celebrationType = 0;
unsigned long celebrationStart = 0;

// ── EEPROM Memory Map ───────────────────────────────────────────────────────
#define EEPROM_SIZE        512
#define EEPROM_MAGIC_ADDR  0     // 2 bytes: 0xAC, 0xE3
#define EEPROM_FLOW_ADDR   2     // 4 bytes: int32
#define EEPROM_GREET_ADDR  6     // 1 byte
#define EEPROM_REMIND_ADDR 7     // 1 byte
#define EEPROM_PET_ADDR    8     // 1 byte
#define EEPROM_BDAY_M_ADDR 9     // 1 byte
#define EEPROM_BDAY_D_ADDR 10    // 1 byte
#define EEPROM_ANN_M_ADDR  11    // 1 byte
#define EEPROM_ANN_D_ADDR  12    // 1 byte
#define EEPROM_TZ_ADDR     13    // 1 byte (signed int8_t: hours * 2)
#define EEPROM_NAME_ADDR   16    // 32 bytes (char string)
#define EEPROM_SSID_ADDR     48    // 64 bytes (char string)
#define EEPROM_PASS_ADDR     112   // 64 bytes (char string)
#define EEPROM_TG_TOKEN_ADDR 176   // 80 bytes (char string)
#define EEPROM_TG_CHAT_ADDR  256   // 32 bytes (char string)
#define EEPROM_MAGIC_A       0xAC
#define EEPROM_MAGIC_B       0xE3

// ============================================================================
//  FORWARD DECLARATIONS
// ============================================================================
void updateDisplay();
void successAnimation();
int  getRotation();
bool buttonPressed(unsigned long now);
void ICACHE_RAM_ATTR handleEncoderInterrupt();
void ICACHE_RAM_ATTR handleButtonInterrupt();
bool sendTelegramMessage(const char* chatId, const String& text);
void sendTelegramMood(uint8_t mood);
void checkTelegramUpdates();
String sanitizeForOLED(const String& input);
void handleRotaryInput(unsigned long now);
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
void printCentered(const char* text, int y, uint8_t maxSize = 3);
void printCentered(const String& str, int y, uint8_t maxSize = 3);
void drawPet(int cx, int cy, uint8_t animState, int bounceY);
void drawCake(int cx, int cy);
uint16_t cheapRandom(uint16_t maxVal);
void saveSettings();
void loadSettings();
void bootAnimation();
void startWiFiOrAP();
void startAPMode();
void checkDateCelebration();
int  getDaysUntil(int targetMonth, int targetDay);
bool isToday(int month, int day);
void setupWebServer();
void renderMenu();
void renderIdle();
void renderTimerUp();
void renderTimerDown();
void renderPomoSelect(bool isFocus);
void renderPomoRunning(bool isFocus);
void renderLoveNote();
void renderPet();
void renderDays();
void renderCompliment();
void renderDecision();
void renderBreathSelect();
void renderMoodSelect();
void renderMoodResponse();
void renderWiFiPortal();
void renderGreetSelect();
void renderReminder();
void renderGreeting();
void renderGreetingTeaser();
void renderLiveNote();
void renderCelebration();

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(CLK), handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT),  handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW),  handleButtonInterrupt, FALLING);

  Serial.begin(9600);
  Serial.println(F("\nStarting Iglet..."));

  EEPROM.begin(EEPROM_SIZE);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring (0x3C/0x3D)"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextWrap(false);
  display.display();

  rngState = (uint16_t)(analogRead(A0) ^ millis());
  if (rngState == 0) rngState = 0x1337;

  loadSettings();

  lastActivityTime = millis();
  lastGreetingTime = millis();
  lastReminderTime = millis();
  bootStartTime    = millis();
  currentState     = BOOT_ANIM;

  // Start background Wi-Fi connection or setup AP
  startWiFiOrAP();

  Serial.println(F("Iglet setup complete!"));
}

// ============================================================================
//  MAIN LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();

  // Background network tasks
  if (apModeActive) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
  MDNS.update();

  // Check background Telegram Bot updates (only if connected, token provided, user has been idle for >= 6s, and not inside a timer/animation)
  if (WiFi.status() == WL_CONNECTED && strlen(cfgTelegramToken) > 10) {
    if (now - lastActivityTime > 6000) {
      if (currentState == MENU || currentState == IDLE || currentState == LIVE_NOTE_DISPLAY) {
        checkTelegramUpdates();
      }
    }
  }

  // Check background Wi-Fi connection
  if (wifiConnecting) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnecting = false;
      Serial.print(F("WiFi connected! IP: "));
      Serial.println(WiFi.localIP());

      if (!ntpConfigured) {
        configTime((long)(cfgTzOffset * 3600), 0, "pool.ntp.org", "time.nist.gov");
        ntpConfigured = true;
      }
      MDNS.begin("iglet");
      MDNS.addService("http", "tcp", 80);
      setupWebServer();
    } else if (now - wifiConnectStart > 12000) {
      // Timeout connecting to home WiFi: drop into offline mode smoothly
      wifiConnecting = false;
      Serial.println(F("WiFi connection timeout, continuing offline."));
    }
  }

  // Check if NTP time just synced
  static bool celebrationChecked = false;
  if (!celebrationChecked && time(nullptr) > 100000) {
    celebrationChecked = true;
    checkDateCelebration();
  }

  // Priority boot animation
  if (currentState == BOOT_ANIM) {
    bootAnimation();
    return;
  }

  // Special birthday/anniversary celebration screen
  if (currentState == CELEBRATION_DISPLAY) {
    if (now - celebrationStart > 5000) {
      currentState = MENU;
      updateDisplay();
    }
    return;
  }

  // Periodic events (only in MENU or IDLE)
  handleAutoGreeting(now);
  handleReminders(now);

  // Non-blocking animations
  if (currentState == BREATHING_EXERCISE) {
    handleBreathingAnimation(now);
  }

  if (currentState == COMPLIMENT_ROULETTE && rouletteSpinning) {
    handleRouletteSpin(now);
  }

  if (currentState == DECISION_ORACLE) {
    handleDecisionSpin(now);
  }

  // Auto-dismiss pet interaction animation
  if (currentState == PET_VIEW && petAnimState != 0 && (now - petAnimStart > 2200)) {
    petAnimState = 0;
    updateDisplay();
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

  // Auto-transition from GREETING_TEASER -> GREETING_WAKEUP (Note)
  if (currentState == GREETING_TEASER && (now - teaserStart > 4000)) {
    currentState = GREETING_WAKEUP;
    lastGreetingTime = now;
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
  handleRotaryInput(now);
  handleButtonPresses(now);
  handleCounting(now);
  handleInactivity(now);
}

// ============================================================================
//  WIFI & NETWORKING INITIALIZATION
// ============================================================================
void startWiFiOrAP() {
  if (strlen(cfgWifiSSID) > 0) {
    // We have saved Wi-Fi credentials: connect in background!
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfgWifiSSID, cfgWifiPass);
    wifiConnecting = true;
    wifiConnectStart = millis();
    apModeActive = false;
    ntpConfigured = false;
    Serial.print(F("Connecting to WiFi: "));
    Serial.println(cfgWifiSSID);
  } else {
    // No Wi-Fi saved yet: start setup hotspot!
    startAPMode();
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Iglet-Setup");
  apModeActive = true;
  wifiConnecting = false;

  // Captive portal DNS redirect
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Serial.println(F("Started AP mode: 'Iglet-Setup' (192.168.4.1)"));
  setupWebServer();
}

// ============================================================================
//  WEB SERVER (Phone Setup Portal & Live Note Dashboard)
// ============================================================================
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>Iglet Companion</title>"
                  "<style>"
                  "body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;"
                  "background:linear-gradient(135deg,#ff9a9e,#fecfef);min-height:100vh;margin:0;padding:16px;box-sizing:border-box;color:#333;}"
                  ".card{background:rgba(255,255,255,0.95);border-radius:20px;padding:22px;max-width:420px;margin:12px auto;box-shadow:0 10px 25px rgba(0,0,0,0.1);}"
                  "h1{color:#e63946;margin:0 0 6px;font-size:24px;text-align:center;}"
                  "p.sub{text-align:center;margin:0 0 18px;color:#777;font-size:14px;}"
                  "h2{color:#444;font-size:16px;margin:18px 0 10px;border-bottom:2px solid #ffe3e3;padding-bottom:6px;}"
                  "label{display:block;margin:10px 0 4px;font-weight:600;font-size:13px;color:#555;}"
                  "input,select{width:100%;padding:10px 12px;border:1.5px solid #ddd;border-radius:10px;box-sizing:border-box;font-size:15px;margin-bottom:8px;}"
                  "input:focus{border-color:#ff6b81;outline:none;}"
                  "button{background:linear-gradient(135deg,#ff6b81,#ff4757);color:#fff;border:none;padding:12px 18px;font-size:15px;font-weight:700;border-radius:12px;cursor:pointer;width:100%;margin-top:10px;box-shadow:0 4px 12px rgba(255,71,87,0.3);}"
                  "button:active{transform:scale(0.98);}"
                  ".row{display:flex;gap:10px;}"
                  ".badge{display:inline-block;background:#ffeaa7;color:#d35400;padding:4px 10px;border-radius:12px;font-size:12px;font-weight:700;}"
                  "</style></head><body>");

  html += F("<div class='card'><h1>🎀 Iglet Companion</h1><p class='sub'>For ");
  html += String(cfgGfName);
  html += F(" with love</p>");

  // 1. Live Love Note Sender (Available when connected to Wi-Fi)
  if (WiFi.status() == WL_CONNECTED) {
    html += F("<h2>💌 Send Live Note to Her Desk!</h2>"
              "<form action='/sendnote' method='POST'>"
              "<label>Top Line (Yellow zone, max 12 chars)</label>"
              "<input type='text' name='l1' maxlength='12' placeholder='Thinking of' required>"
              "<label>Bottom Line (Blue zone, max 12 chars)</label>"
              "<input type='text' name='l2' maxlength='12' placeholder='you always <3' required>"
              "<button type='submit'>Send to Her Screen 🚀</button></form>");
  }

  // 2. Settings & Wi-Fi Configuration
  html += F("<h2>⚙️ Device & Special Dates Settings</h2>"
            "<form action='/save' method='POST'>"
            "<label>Her Name or Nickname</label>"
            "<input type='text' name='name' maxlength='20' value='");
  html += String(cfgGfName);
  html += F("' required>"
            "<div class='row'><div><label>🎂 Birthday Month</label>"
            "<input type='number' name='bm' min='1' max='12' value='");
  html += String(cfgBdayMonth);
  html += F("'></div><div><label>Day</label>"
            "<input type='number' name='bd' min='1' max='31' value='");
  html += String(cfgBdayDay);
  html += F("'></div></div>"
            "<div class='row'><div><label>💍 Anniversary Month</label>"
            "<input type='number' name='am' min='1' max='12' value='");
  html += String(cfgAnnivMonth);
  html += F("'></div><div><label>Day</label>"
            "<input type='number' name='ad' min='1' max='31' value='");
  html += String(cfgAnnivDay);
  html += F("'></div></div>"
            "<label>⏰ Timezone Offset (Hours, e.g. 5.5 for IST)</label>"
            "<input type='number' step='0.5' name='tz' value='");
  html += String(cfgTzOffset, 1);
  html += F("'>"
            "<h2>📶 Wi-Fi Settings</h2>"
            "<label>Wi-Fi Network Name (SSID)</label>"
            "<input type='text' name='ssid' value='");
  html += String(cfgWifiSSID);
  html += F("' placeholder='Home Wi-Fi'>"
            "<label>Wi-Fi Password</label>"
            "<input type='password' name='pass' placeholder='Leave blank if unchanged'>"
            "<h2>🤖 Telegram Remote Bot</h2>"
            "<label>Bot Token ");
  if (strlen(cfgTelegramToken) > 0) {
    html += F("<span class='badge' style='background:#d4edda;color:#155724;font-size:11px;padding:2px 8px;border-radius:8px;float:right;'>Saved</span>");
  }
  html += F("</label>"
            "<input type='password' name='tgtoken' placeholder='Leave blank if unchanged'>"
            "<label>Your Chat ID ");
  if (strlen(cfgTelegramChatId) > 0) {
    html += F("<span class='badge' style='background:#d4edda;color:#155724;font-size:11px;padding:2px 8px;border-radius:8px;float:right;'>Linked</span>");
  }
  html += F("</label>"
            "<input type='password' name='tgchat' placeholder='Leave blank if unchanged'>"
            "<p style='font-size:12px;color:#666;margin:4px 0 10px;'>💡 Message your bot to pop up notes on her desk! When she logs her mood, you receive a Telegram ping back.</p>"
            "<button type='submit'>Save & Connect ✨</button></form></div></body></html>");

  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("name")) {
    strncpy(cfgGfName, server.arg("name").c_str(), sizeof(cfgGfName) - 1);
  }
  if (server.hasArg("bm")) cfgBdayMonth = server.arg("bm").toInt();
  if (server.hasArg("bd")) cfgBdayDay = server.arg("bd").toInt();
  if (server.hasArg("am")) cfgAnnivMonth = server.arg("am").toInt();
  if (server.hasArg("ad")) cfgAnnivDay = server.arg("ad").toInt();
  if (server.hasArg("tz")) {
    cfgTzOffset = server.arg("tz").toFloat();
    if (WiFi.status() == WL_CONNECTED) {
      configTime((long)(cfgTzOffset * 3600), 0, "pool.ntp.org", "time.nist.gov");
    }
  }

  if (server.hasArg("ssid") && server.arg("ssid").length() > 0) {
    strncpy(cfgWifiSSID, server.arg("ssid").c_str(), sizeof(cfgWifiSSID) - 1);
  }
  if (server.hasArg("pass") && server.arg("pass").length() > 0) {
    strncpy(cfgWifiPass, server.arg("pass").c_str(), sizeof(cfgWifiPass) - 1);
  }

  if (server.hasArg("tgtoken") && server.arg("tgtoken").length() > 0) {
    if (server.arg("tgtoken") == "none" || server.arg("tgtoken") == "clear") {
      cfgTelegramToken[0] = '\0';
    } else {
      strncpy(cfgTelegramToken, server.arg("tgtoken").c_str(), sizeof(cfgTelegramToken) - 1);
      cfgTelegramToken[sizeof(cfgTelegramToken) - 1] = '\0';
    }
  }
  if (server.hasArg("tgchat") && server.arg("tgchat").length() > 0) {
    if (server.arg("tgchat") == "none" || server.arg("tgchat") == "clear") {
      cfgTelegramChatId[0] = '\0';
    } else {
      strncpy(cfgTelegramChatId, server.arg("tgchat").c_str(), sizeof(cfgTelegramChatId) - 1);
      cfgTelegramChatId[sizeof(cfgTelegramChatId) - 1] = '\0';
    }
  }

  saveSettings();

  String res = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#ffeaa7;}"
                 ".c{background:#fff;padding:30px;border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,0.1);max-width:360px;margin:auto;}"
                 "h2{color:#e63946;}a{color:#e63946;font-weight:700;}</style></head><body>"
                 "<div class='c'><h2>Saved! ✨</h2><p>Iglet is connecting to your Wi-Fi...</p>"
                 "<p><a href='/'>Back to Dashboard</a></p></div></body></html>");
  server.send(200, "text/html", res);

  // Apply Wi-Fi immediately
  delay(500);
  startWiFiOrAP();
}

void handleSendNote() {
  if (server.hasArg("l1") && server.hasArg("l2")) {
    strncpy(liveNoteLine1, server.arg("l1").c_str(), sizeof(liveNoteLine1) - 1);
    strncpy(liveNoteLine2, server.arg("l2").c_str(), sizeof(liveNoteLine2) - 1);
    liveNoteStart = millis();

    if (displayOff) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displayOff = false;
    }
    currentState = LIVE_NOTE_DISPLAY;
    lastActivityTime = millis();
    buttonIgnoreUntil = millis() + 1500; // Block false button triggers during Wi-Fi transmission
    updateDisplay();

    String res = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                   "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#ff9a9e;}"
                   ".c{background:#fff;padding:30px;border-radius:15px;max-width:360px;margin:auto;}"
                   "h2{color:#e63946;}a{color:#e63946;font-weight:700;}</style></head><body>"
                   "<div class='c'><h2>Sent with Love! 💖</h2><p>Your note is on ");
    res += String(cfgGfName);
    res += F("'s desk right now!</p>"
             "<p style='color:#666;font-size:13px;'>It will stay on her screen until she clicks the dial to dismiss.</p>"
             "<p><a href='/'>Send another note</a></p></div></body></html>");
    server.send(200, "text/html", res);
  } else {
    server.send(400, "text/plain", "Missing lines");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/sendnote", HTTP_POST, handleSendNote);

  // Captive portal OS detection redirects
  server.on("/generate_204", handleRoot);
  server.on("/fwlink", handleRoot);
  server.on("/hotspot-detect.html", handleRoot);
  server.on("/ncsi.txt", handleRoot);

  server.onNotFound(handleRoot);
  server.begin();
  Serial.println(F("Web server active."));
}

// ============================================================================
//  TELEGRAM BOT INTEGRATION
// ============================================================================

String sanitizeForOLED(const String& input) {
  String s = input;
  // Replace heart symbols with <3
  s.replace("\xE2\x9D\xA4", "<3"); // ❤️
  s.replace("\xF0\x9F\x92\x96", "<3"); // 💖
  s.replace("\xF0\x9F\x92\x95", "<3"); // 💕
  s.replace("\xF0\x9F\x92\x97", "<3"); // 💗
  s.replace("\xF0\x9F\x92\x93", "<3"); // 💓
  s.replace("\xF0\x9F\x92\x8C", "<3"); // 💌
  // Replace smiley emojis
  s.replace("\xF0\x9F\x98\x8A", ":)"); // 😊
  s.replace("\xF0\x9F\x98\x80", ":)"); // 😀
  s.replace("\xF0\x9F\x99\x82", ":)"); // 🙂
  s.replace("\xF0\x9F\x98\x8D", "<3"); // 😍
  s.replace("\xF0\x9F\xA5\xB0", "<3"); // 🥰
  s.replace("\xF0\x9F\x98\x98", ":*"); // 😘
  s.replace("\xF0\x9F\x98\x94", ":("); // 😔
  s.replace("\xF0\x9F\xA5\xBA", ":("); // 🥺
  s.replace("\xF0\x9F\x98\xA2", ":("); // 😢
  s.replace("\xF0\x9F\x98\xA1", ">:("); // 😡

  // Strip remaining non-ASCII or control characters
  String clean = "";
  clean.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t b = (uint8_t)s[i];
    if (b >= 32 && b <= 126) {
      clean += (char)b;
    } else if (b == '\n') {
      clean += ' ';
    }
  }
  clean.trim();
  return clean;
}

String jsonEscape(const String& str) {
  String out = "";
  out.reserve(str.length() + 8);
  for (size_t i = 0; i < str.length(); i++) {
    char c = str[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') continue;
    else out += c;
  }
  return out;
}

bool sendTelegramMessage(const char* chatId, const String& text) {
  if (WiFi.status() != WL_CONNECTED || strlen(cfgTelegramToken) < 10) return false;
  if (chatId == nullptr || strlen(chatId) == 0) return false;

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 1024);
  client.setTimeout(3500);

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println(F("[Telegram] Send connect failed"));
    return false;
  }

  String payload = "{\"chat_id\":\"" + String(chatId) + "\",\"text\":\"" + jsonEscape(text) + "\"}";

  String req = "POST /bot" + String(cfgTelegramToken) + "/sendMessage HTTP/1.1\r\n";
  req += "Host: api.telegram.org\r\n";
  req += "Content-Type: application/json\r\n";
  req += "Content-Length: " + String(payload.length()) + "\r\n";
  req += "Connection: close\r\n\r\n";
  req += payload;

  client.print(req);

  unsigned long t = millis();
  while (client.connected() && millis() - t < 2500) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1 200 OK")) {
        Serial.println(F("[Telegram] Message sent successfully."));
        client.stop();
        return true;
      }
    }
    yield();
  }
  client.stop();
  return true;
}

void sendTelegramMood(uint8_t mood) {
  if (strlen(cfgTelegramChatId) == 0) return;
  if (mood >= MOOD_COUNT) return;

  const char* emojis[] = {"😊", "🙂", "🥺", "😤", "🥰"};
  String text = "🎀 " + String(cfgGfName) + " just checked in on Iglet!\n";
  text += "Mood: " + String(moodLabels[mood]) + " " + emojis[mood] + "\n";
  text += "Iglet replied: \"" + String(moodResponses[mood][0]) + " " + String(moodResponses[mood][1]) + "\"";

  sendTelegramMessage(cfgTelegramChatId, text);
}

void checkTelegramUpdates() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (strlen(cfgTelegramToken) < 10) return;

  unsigned long now = millis();
  if (now - lastTelegramPoll < telegramPollInterval) return;
  lastTelegramPoll = now;

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(1024, 256);
  client.setTimeout(1800);

  if (!client.connect("api.telegram.org", 443)) {
    return;
  }

  // On first boot, get the latest offset so we ignore stale messages sent while offline
  String url = "/bot" + String(cfgTelegramToken) + "/getUpdates?limit=1&timeout=0";
  if (!telegramInitialized) {
    url += "&offset=-1";
  } else if (lastTelegramUpdateId > 0) {
    url += "&offset=" + String(lastTelegramUpdateId + 1);
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.telegram.org\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long tStart = millis();
  while (!client.available() && millis() - tStart < 1200) {
    delay(10);
    yield();
  }

  if (!client.available()) {
    client.stop();
    return;
  }

  if (!client.find("\r\n\r\n")) {
    client.stop();
    return;
  }

  String body = client.readString();
  client.stop();

  if (!body.startsWith("{\"ok\":true")) return;

  int upIdx = body.indexOf("\"update_id\":");
  if (upIdx == -1) {
    if (!telegramInitialized) telegramInitialized = true;
    return;
  }

  int upEnd = body.indexOf(',', upIdx);
  if (upEnd == -1) upEnd = body.indexOf('}', upIdx);
  long updateId = body.substring(upIdx + 12, upEnd).toInt();
  if (updateId > 0) {
    lastTelegramUpdateId = updateId;
  }

  // If this was just the initial flush to sync offset, don't display stale message
  if (!telegramInitialized) {
    telegramInitialized = true;
    Serial.print(F("[Telegram] Initialized at updateId: "));
    Serial.println(lastTelegramUpdateId);
    return;
  }

  // Extract chat id
  int chatIdx = body.indexOf("\"chat\":{\"id\":");
  String senderChatId = "";
  if (chatIdx != -1) {
    int chatEnd = body.indexOf(',', chatIdx + 13);
    senderChatId = body.substring(chatIdx + 13, chatEnd);
    senderChatId.trim();
  }

  // Auto-link chat ID if not configured yet!
  if (strlen(cfgTelegramChatId) == 0 && senderChatId.length() > 0) {
    strncpy(cfgTelegramChatId, senderChatId.c_str(), sizeof(cfgTelegramChatId) - 1);
    cfgTelegramChatId[sizeof(cfgTelegramChatId) - 1] = '\0';
    saveSettings();
    Serial.print(F("[Telegram] Auto-linked Chat ID: "));
    Serial.println(cfgTelegramChatId);
    sendTelegramMessage(cfgTelegramChatId, "🎉 Successfully paired with Iglet!\nI have saved your Chat ID (" + senderChatId + "). Send any message to display it on " + String(cfgGfName) + "'s desk screen!");
    return;
  } else if (strlen(cfgTelegramChatId) > 0 && senderChatId != String(cfgTelegramChatId)) {
    // Ignore messages from other chats
    return;
  }

  // Extract message text
  int textIdx = body.indexOf("\"text\":\"");
  if (textIdx == -1) return;
  int textStart = textIdx + 8;
  int textEnd = -1;
  for (int i = textStart; i < (int)body.length(); i++) {
    if (body[i] == '"' && body[i - 1] != '\\') {
      textEnd = i;
      break;
    }
  }
  if (textEnd == -1) return;
  String rawText = body.substring(textStart, textEnd);

  // Unescape standard escapes
  rawText.replace("\\\"", "\"");
  rawText.replace("\\\\", "\\");

  // Commands check
  if (rawText.startsWith("/start") || rawText.startsWith("/help")) {
    sendTelegramMessage(senderChatId.c_str(),
      "✨ Iglet Desk Companion Connected!\nSend me any love note, and it will pop up directly on " + String(cfgGfName) + "'s desk!\n\n💡 Tip: Use '|' to split into two lines:\nThinking of you | see you tonight <3\n\nWhenever she checks in her mood on Iglet, you will receive a notification right here!");
    return;
  }

  // Sanitize text for 128x64 OLED (converts emojis like ❤️ -> <3, 😊 -> :))
  String cleanText = sanitizeForOLED(rawText);
  if (cleanText.length() == 0) return;

  // Split into line1 and line2
  String l1 = "";
  String l2 = "";

  if (cleanText.indexOf('|') != -1) {
    int pipe = cleanText.indexOf('|');
    l1 = cleanText.substring(0, pipe);
    l2 = cleanText.substring(pipe + 1);
  } else if (cleanText.length() > 10) {
    // Find space near middle
    int mid = cleanText.length() / 2;
    int splitAt = -1;
    for (int d = 0; d < 6; d++) {
      if (mid - d > 0 && cleanText[mid - d] == ' ') { splitAt = mid - d; break; }
      if (mid + d < (int)cleanText.length() && cleanText[mid + d] == ' ') { splitAt = mid + d; break; }
    }
    if (splitAt != -1) {
      l1 = cleanText.substring(0, splitAt);
      l2 = cleanText.substring(splitAt + 1);
    } else {
      l1 = cleanText.substring(0, 10);
      l2 = cleanText.substring(10);
    }
  } else {
    l1 = cleanText;
    l2 = "";
  }

  l1.trim();
  l2.trim();

  strncpy(liveNoteLine1, l1.c_str(), sizeof(liveNoteLine1) - 1);
  liveNoteLine1[sizeof(liveNoteLine1) - 1] = '\0';
  strncpy(liveNoteLine2, l2.c_str(), sizeof(liveNoteLine2) - 1);
  liveNoteLine2[sizeof(liveNoteLine2) - 1] = '\0';

  liveNoteStart = millis();
  if (displayOff) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayOff = false;
  }
  currentState = LIVE_NOTE_DISPLAY;
  lastActivityTime = millis();
  buttonIgnoreUntil = millis() + 1500;
  updateDisplay();

  // Send confirmation back to Telegram
  String reply = "💖 Delivered to " + String(cfgGfName) + "'s desk!\n\"" + l1;
  if (l2.length() > 0) reply += " " + l2;
  reply += "\"";
  sendTelegramMessage(senderChatId.c_str(), reply);
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
    if (greetIntervalIdx >= GREET_OPTION_COUNT) greetIntervalIdx = 3;
    remindersEnabled = EEPROM.read(EEPROM_REMIND_ADDR) != 0;
    petLoveLevel     = EEPROM.read(EEPROM_PET_ADDR);
    if (petLoveLevel > 5 || petLoveLevel == 0) petLoveLevel = 3;

    cfgBdayMonth  = EEPROM.read(EEPROM_BDAY_M_ADDR);
    cfgBdayDay    = EEPROM.read(EEPROM_BDAY_D_ADDR);
    cfgAnnivMonth = EEPROM.read(EEPROM_ANN_M_ADDR);
    cfgAnnivDay   = EEPROM.read(EEPROM_ANN_D_ADDR);

    int8_t tzInt = (int8_t)EEPROM.read(EEPROM_TZ_ADDR);
    cfgTzOffset  = (float)tzInt / 2.0f;

    for (int i = 0; i < 31; i++) cfgGfName[i] = (char)EEPROM.read(EEPROM_NAME_ADDR + i);
    cfgGfName[31] = '\0';

    for (int i = 0; i < 63; i++) cfgWifiSSID[i] = (char)EEPROM.read(EEPROM_SSID_ADDR + i);
    cfgWifiSSID[63] = '\0';

    for (int i = 0; i < 63; i++) cfgWifiPass[i] = (char)EEPROM.read(EEPROM_PASS_ADDR + i);
    cfgWifiPass[63] = '\0';

    for (int i = 0; i < 79; i++) cfgTelegramToken[i] = (char)EEPROM.read(EEPROM_TG_TOKEN_ADDR + i);
    cfgTelegramToken[79] = '\0';
    if ((uint8_t)cfgTelegramToken[0] == 0xFF) cfgTelegramToken[0] = '\0';

    for (int i = 0; i < 31; i++) cfgTelegramChatId[i] = (char)EEPROM.read(EEPROM_TG_CHAT_ADDR + i);
    cfgTelegramChatId[31] = '\0';
    if ((uint8_t)cfgTelegramChatId[0] == 0xFF) cfgTelegramChatId[0] = '\0';

    if (strlen(cfgTelegramToken) == 0 && strlen(TELEGRAM_BOT_TOKEN) > 0) {
      strncpy(cfgTelegramToken, TELEGRAM_BOT_TOKEN, sizeof(cfgTelegramToken) - 1);
      cfgTelegramToken[sizeof(cfgTelegramToken) - 1] = '\0';
    }
    if (strlen(cfgTelegramChatId) == 0 && strlen(TELEGRAM_CHAT_ID) > 0) {
      strncpy(cfgTelegramChatId, TELEGRAM_CHAT_ID, sizeof(cfgTelegramChatId) - 1);
      cfgTelegramChatId[sizeof(cfgTelegramChatId) - 1] = '\0';
    }

    Serial.println(F("Settings loaded from EEPROM."));
  } else {
    Serial.println(F("No EEPROM magic, initializing defaults."));
    saveSettings();
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
  EEPROM.write(EEPROM_PET_ADDR, petLoveLevel);

  EEPROM.write(EEPROM_BDAY_M_ADDR, cfgBdayMonth);
  EEPROM.write(EEPROM_BDAY_D_ADDR, cfgBdayDay);
  EEPROM.write(EEPROM_ANN_M_ADDR, cfgAnnivMonth);
  EEPROM.write(EEPROM_ANN_D_ADDR, cfgAnnivDay);

  int8_t tzInt = (int8_t)(cfgTzOffset * 2.0f);
  EEPROM.write(EEPROM_TZ_ADDR, (uint8_t)tzInt);

  for (int i = 0; i < 31; i++) EEPROM.write(EEPROM_NAME_ADDR + i, cfgGfName[i]);
  for (int i = 0; i < 63; i++) EEPROM.write(EEPROM_SSID_ADDR + i, cfgWifiSSID[i]);
  for (int i = 0; i < 63; i++) EEPROM.write(EEPROM_PASS_ADDR + i, cfgWifiPass[i]);
  for (int i = 0; i < 79; i++) EEPROM.write(EEPROM_TG_TOKEN_ADDR + i, cfgTelegramToken[i]);
  for (int i = 0; i < 31; i++) EEPROM.write(EEPROM_TG_CHAT_ADDR + i, cfgTelegramChatId[i]);

  EEPROM.commit();
  Serial.println(F("Settings saved to EEPROM."));
}

// ============================================================================
//  DATE & TIME HELPERS
// ============================================================================
int getDaysUntil(int targetMonth, int targetDay) {
  if (targetMonth < 1 || targetMonth > 12 || targetDay < 1 || targetDay > 31) return -1;
  time_t now = time(nullptr);
  if (now < 100000) return -1;  // NTP not synced yet

  struct tm* t = localtime(&now);
  int curMonth = t->tm_mon + 1;
  int curDay   = t->tm_mday;
  int curYear  = t->tm_year + 1900;

  // Direct check for TODAY!
  if (curMonth == targetMonth && curDay == targetDay) {
    return 0;
  }

  // Midnight of today
  struct tm today = *t;
  today.tm_hour = 0;
  today.tm_min  = 0;
  today.tm_sec  = 0;
  time_t todayMid = mktime(&today);

  // Midnight of target date this year
  struct tm target = *t;
  target.tm_year = curYear - 1900;
  target.tm_mon  = targetMonth - 1;
  target.tm_mday = targetDay;
  target.tm_hour = 0;
  target.tm_min  = 0;
  target.tm_sec  = 0;
  time_t targetMid = mktime(&target);

  if (targetMid < todayMid) {
    // Already passed this year -> count to next year!
    target.tm_year = curYear - 1900 + 1;
    targetMid = mktime(&target);
  }

  double diff = difftime(targetMid, todayMid);
  int days = (int)((diff + 43200.0) / 86400.0);
  return (days < 0) ? 0 : days;
}

bool isToday(int month, int day) {
  if (month < 1 || month > 12 || day < 1 || day > 31) return false;
  time_t now = time(nullptr);
  if (now < 100000) return false;
  struct tm* t = localtime(&now);
  return (t->tm_mon + 1 == month && t->tm_mday == day);
}

void checkDateCelebration() {
  if (isToday(cfgBdayMonth, cfgBdayDay)) {
    celebrationType = 1; // Birthday!
    celebrationStart = millis();
    currentState = CELEBRATION_DISPLAY;
    updateDisplay();
  } else if (isToday(cfgAnnivMonth, cfgAnnivDay)) {
    celebrationType = 2; // Anniversary!
    celebrationStart = millis();
    currentState = CELEBRATION_DISPLAY;
    updateDisplay();
  }
}

// ============================================================================
//  CHEAP PRNG (xorshift16)
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
//  BOOT ANIMATION — "Hi [Name]!" + Beating Heart
// ============================================================================
void bootAnimation() {
  unsigned long elapsed = millis() - bootStartTime;

  if (elapsed < 600) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    String greeting = "Hi " + String(cfgGfName) + "!";
    printCentered(greeting, 4, 2);
    display.display();
  } else if (elapsed < 2600) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    String greeting = "Hi " + String(cfgGfName) + "!";
    printCentered(greeting, 0, 2);

    int heartPhase = ((elapsed - 600) / 250) % 4;
    int heartSize = (heartPhase < 2) ? 9 + heartPhase * 3 : 15 - heartPhase * 2;
    drawHeart(64, 44, heartSize);

    display.display();
  } else {
    // Check if today is a birthday or anniversary celebration!
    checkDateCelebration();
    if (currentState != CELEBRATION_DISPLAY) {
      currentState = MENU;
      lastActivityTime = millis();
      updateDisplay();
    }
  }
}

// ============================================================================
//  GRAPHICS HELPERS
// ============================================================================
void printCentered(const char* text, int y, uint8_t maxSize) {
  int len = strlen(text);
  if (len == 0) return;

  uint8_t size = 1;
  if (maxSize >= 4 && len <= 4) {
    size = 4;
  } else if (maxSize >= 3 && len <= 6) {
    size = 3;
  } else if (maxSize >= 2 && len <= 10) {
    size = 2;
  } else {
    size = 1;
  }

  int charWidth = size * 6;
  int totalWidth = len * charWidth;
  int x = (128 - totalWidth) / 2;
  if (x < 0) x = 0;

  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
}

void printCentered(const String& str, int y, uint8_t maxSize) {
  printCentered(str.c_str(), y, maxSize);
}

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

void drawCake(int cx, int cy) {
  // 2-tier cute cake
  display.fillRect(cx - 16, cy + 2, 32, 14, WHITE);
  display.fillRect(cx - 12, cy - 8, 24, 10, WHITE);
  // Frosting line
  display.drawLine(cx - 16, cy + 2, cx + 16, cy + 2, BLACK);
  // 3 candles
  display.drawLine(cx - 8, cy - 12, cx - 8, cy - 8, WHITE);
  display.drawLine(cx,     cy - 12, cx,     cy - 8, WHITE);
  display.drawLine(cx + 8, cy - 12, cx + 8, cy - 8, WHITE);
  // Flames
  display.drawPixel(cx - 8, cy - 14, WHITE);
  display.drawPixel(cx,     cy - 14, WHITE);
  display.drawPixel(cx + 8, cy - 14, WHITE);
}

void drawPet(int cx, int cy, uint8_t animState, int bounceY) {
  int y = cy + bounceY;

  // Body: Round white creature
  display.fillCircle(cx, y, 13, WHITE);

  // Cute ears
  display.fillTriangle(cx - 9, y - 6, cx - 6, y - 18, cx - 3, y - 6, WHITE);
  display.fillTriangle(cx + 3, y - 6, cx + 6, y - 18, cx + 9, y - 6, WHITE);
  display.drawLine(cx - 6, y - 7, cx - 6, y - 14, BLACK);
  display.drawLine(cx + 6, y - 7, cx + 6, y - 14, BLACK);

  // Little paws
  display.fillCircle(cx - 8, y + 11, 2, WHITE);
  display.fillCircle(cx + 8, y + 11, 2, WHITE);

  if (animState == 0) {
    // Normal eyes ( • ◡ • )
    display.fillCircle(cx - 4, y - 2, 1, BLACK);
    display.fillCircle(cx + 4, y - 2, 1, BLACK);
    display.drawPixel(cx - 2, y + 3, BLACK);
    display.drawPixel(cx - 1, y + 4, BLACK);
    display.drawPixel(cx,     y + 4, BLACK);
    display.drawPixel(cx + 1, y + 4, BLACK);
    display.drawPixel(cx + 2, y + 3, BLACK);
  } else if (animState == 1) {
    // Petting: Joyful squint ( > ◡ < ) + hearts
    display.drawLine(cx - 6, y - 3, cx - 4, y - 1, BLACK);
    display.drawLine(cx - 4, y - 1, cx - 2, y - 3, BLACK);
    display.drawLine(cx + 2, y - 3, cx + 4, y - 1, BLACK);
    display.drawLine(cx + 4, y - 1, cx + 6, y - 3, BLACK);
    display.fillCircle(cx, y + 3, 2, BLACK);
    drawHeart(cx - 18, y - 10, 4);
    drawHeart(cx + 18, y - 10, 4);
    drawHeart(cx, y - 20, 5);
  } else if (animState == 2) {
    // Eating: Nom mouth + cookie
    display.drawLine(cx - 5, y - 2, cx - 2, y - 2, BLACK);
    display.drawLine(cx + 2, y - 2, cx + 5, y - 2, BLACK);
    display.fillCircle(cx, y + 3, 3, BLACK);
    display.fillCircle(cx - 14, y + 2, 3, WHITE);
    display.drawPixel(cx - 14, y + 2, BLACK);
    display.setTextSize(1);
    display.setCursor(cx + 15, y - 8);
    display.print(F("Yum!"));
  } else if (animState == 3) {
    // Playing: Winking + stars
    display.drawLine(cx - 5, y - 2, cx - 2, y - 2, BLACK);
    display.fillCircle(cx + 4, y - 2, 1, BLACK);
    display.drawLine(cx - 3, y + 2, cx + 3, y + 2, BLACK);
    display.drawPixel(cx, y + 4, BLACK);
    display.drawPixel(cx - 18, y - 6, WHITE);
    display.drawPixel(cx - 17, y - 7, WHITE);
    display.drawPixel(cx + 18, y - 6, WHITE);
    display.drawPixel(cx + 17, y - 7, WHITE);
  } else if (animState == 4) {
    // Sleeping (in IDLE): ( - . - )
    display.drawLine(cx - 5, y, cx - 2, y, BLACK);
    display.drawLine(cx + 2, y, cx + 5, y, BLACK);
    display.drawPixel(cx, y + 3, BLACK);
    display.setTextSize(1);
    display.setCursor(cx + 14, y - 10);
    display.print(F("zZ"));
  }
}

void drawMoodFace(uint8_t mood, int cx, int cy, int r) {
  display.drawCircle(cx, cy, r, WHITE);
  int eyeY = cy - r / 3;
  int eyeSpacing = r / 2;

  switch (mood) {
    case 0: // Happy (Curved smile UP at corners + blush)
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      display.drawPixel(cx - 7, cy + 1, WHITE);
      display.drawPixel(cx + 7, cy + 1, WHITE);
      for (int i = -5; i <= 5; i++) {
        display.drawPixel(cx + i, cy + 6 - (i * i) / 5, WHITE);
      }
      break;

    case 1: // Okay (Straight neutral line)
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      display.drawLine(cx - 4, cy + 4, cx + 4, cy + 4, WHITE);
      break;

    case 2: // Sad (Frown DOWN at corners + teardrop)
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      display.drawPixel(cx + 6, cy, WHITE);     // Teardrop
      display.drawPixel(cx + 6, cy + 1, WHITE);
      for (int i = -5; i <= 5; i++) {
        display.drawPixel(cx + i, cy + 2 + (i * i) / 5, WHITE);
      }
      break;

    case 3: // Angry (Slanted eyebrows + angry mouth)
      display.drawLine(cx - 7, eyeY - 4, cx - 2, eyeY - 2, WHITE);
      display.drawLine(cx + 7, eyeY - 4, cx + 2, eyeY - 2, WHITE);
      display.fillCircle(cx - 5, eyeY, 2, WHITE);
      display.fillCircle(cx + 5, eyeY, 2, WHITE);
      for (int i = -4; i <= 4; i++) {
        display.drawPixel(cx + i, cy + 3 + (i * i) / 4, WHITE);
      }
      break;

    case 4: // Love (Heart eyes + happy smile!)
      drawHeart(cx - eyeSpacing, eyeY - 1, 4);
      drawHeart(cx + eyeSpacing, eyeY - 1, 4);
      for (int i = -5; i <= 5; i++) {
        display.drawPixel(cx + i, cy + 5 - (i * i) / 5, WHITE);
      }
      display.drawPixel(cx, cy + 6, WHITE);
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
    case PET_VIEW:
      renderPet();
      break;
    case DAYS_VIEW:
      renderDays();
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
      break;
    case MOOD_SELECT:
      renderMoodSelect();
      break;
    case MOOD_RESPONSE:
      renderMoodResponse();
      break;
    case WIFI_PORTAL_VIEW:
      renderWiFiPortal();
      break;
    case GREET_SELECT:
      renderGreetSelect();
      break;
    case REMINDER_DISPLAY:
      renderReminder();
      break;
    case GREETING_TEASER:
      renderGreetingTeaser();
      break;
    case GREETING_WAKEUP:
      renderGreeting();
      break;
    case LIVE_NOTE_DISPLAY:
      renderLiveNote();
      break;
    case CELEBRATION_DISPLAY:
      renderCelebration();
      break;
    default:
      break;
  }

  display.display();
}

// ── Render: Main Menu ───────────────────────────────────────────────────────
void renderMenu() {
  display.setTextSize(1);
  String flowStr = (flowMinutes < 1000) ? "Flow:" + String(flowMinutes) + "m" : "F:" + String(flowMinutes / 60) + "h";
  display.setCursor(0, 4);
  display.print(flowStr);

  display.setCursor(84, 4);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(F("WiFi \x09")); // connected dot
  } else if (apModeActive) {
    display.print(F("AP Set"));
  } else {
    display.print(greetLabels[greetIntervalIdx]);
  }

  // Menu text: auto-fit in Size 2 (crisp & never wraps!)
  printCentered(mainMenuItems[menuIndex], 26, 2);

  // Bottom navigation dots
  display.setTextSize(1);
  int dotsWidth = MAIN_MENU_COUNT * 6;
  int dotsX = (128 - dotsWidth) / 2;
  for (uint8_t i = 0; i < MAIN_MENU_COUNT; i++) {
    if (i == menuIndex) {
      display.fillCircle(dotsX + i * 6 + 2, 58, 2, WHITE);
    } else {
      display.drawCircle(dotsX + i * 6 + 2, 58, 1, WHITE);
    }
  }
}

// ── Render: Idle ────────────────────────────────────────────────────────────
void renderIdle() {
  display.setTextSize(1);
  display.setCursor(16, 4);
  display.print(DEVICE_NAME);
  display.print(F(" is sleeping"));
  drawPet(64, 38, 4, 0);
}

// ── Render: Virtual Pet ─────────────────────────────────────────────────────
void renderPet() {
  display.setTextSize(1);
  display.setCursor(0, 4);
  display.print(DEVICE_NAME);

  display.setCursor(56, 4);
  display.print(F("Love:"));
  for (uint8_t i = 0; i < 5; i++) {
    if (i < petLoveLevel) {
      drawHeart(90 + i * 8, 7, 3);
    } else {
      display.drawPixel(90 + i * 8, 7, WHITE);
    }
  }

  int bounceY = (petAnimState == 3) ? ((millis() / 150) % 2 == 0 ? -4 : 0) : ((millis() / 700) % 2 == 0 ? 0 : 1);
  drawPet(64, 33, petAnimState, bounceY);

  if (petAnimState == 1) {
    display.setTextSize(1);
    display.setCursor(38, 55);
    display.print(F("<3 Loved! <3"));
  } else if (petAnimState == 2) {
    display.setTextSize(1);
    display.setCursor(34, 55);
    display.print(F("* Nom nom! *"));
  } else if (petAnimState == 3) {
    display.setTextSize(1);
    display.setCursor(40, 55);
    display.print(F("Wheee! :D"));
  } else {
    const int tabX[] = {8, 38, 72, 102};
    const int tabW[] = {18, 24, 24, 24};
    display.setTextSize(1);
    for (uint8_t i = 0; i < PET_ACTION_COUNT; i++) {
      display.setCursor(tabX[i], 55);
      display.print(petActionLabels[i]);
      if (i == petActionIndex) {
        display.drawLine(tabX[i], 63, tabX[i] + tabW[i], 63, WHITE);
      }
    }
  }
}

// ── Render: Feature 10: Days View (Countdowns & Clock) ───────────────────────
void renderDays() {
  display.setTextSize(1);

  if (daysSubIndex == 0) {
    // Anniversary countdown
    printCentered(F("Our Anniversary <3"), 4, 1);

    int days = getDaysUntil(cfgAnnivMonth, cfgAnnivDay);
    if (days == -1) {
      printCentered(F("Sync WiFi in menu"), 24, 1);
      printCentered(F("or set dates on phone"), 38, 1);
    } else if (days == 0) {
      printCentered(F("TODAY!! <3"), 28, 2);
    } else {
      printCentered("in " + String(days) + "d", 28, 3);
    }
  } else if (daysSubIndex == 1) {
    // Birthday countdown
    printCentered(String(cfgGfName) + F("'s Birthday <3"), 4, 1);

    int days = getDaysUntil(cfgBdayMonth, cfgBdayDay);
    if (days == -1) {
      printCentered(F("Sync WiFi in menu"), 24, 1);
      printCentered(F("or set dates on phone"), 38, 1);
    } else if (days == 0) {
      printCentered(F("TODAY!! <3"), 28, 2);
    } else {
      printCentered("in " + String(days) + "d", 28, 3);
    }
  } else {
    // Live Clock & Date
    time_t now = time(nullptr);
    if (now > 100000) {
      struct tm* t = localtime(&now);
      char dateBuf[20];
      strftime(dateBuf, sizeof(dateBuf), "%d %b %Y", t);
      printCentered(dateBuf, 4, 1);

      char timeBuf[10];
      strftime(timeBuf, sizeof(timeBuf), "%H:%M", t);
      printCentered(timeBuf, 26, 3);
    } else {
      printCentered(F("Internet Clock"), 4, 1);
      printCentered(F("Connecting WiFi..."), 30, 1);
    }
  }

  // Bottom dots indicator (3 pages)
  for (uint8_t i = 0; i < 3; i++) {
    if (i == daysSubIndex) display.fillCircle(56 + i * 8, 58, 2, WHITE);
    else display.drawCircle(56 + i * 8, 58, 1, WHITE);
  }
}

// ── Render: Feature 11: Celebration Display ─────────────────────────────────
void renderCelebration() {
  if (celebrationType == 1) {
    // Birthday!
    printCentered(F("HAPPY BIRTHDAY! <3"), 4, 1);
    drawCake(64, 34);
    printCentered(String(cfgGfName) + "!", 48, 2);
  } else {
    // Anniversary!
    printCentered(F("HAPPY ANNIVERSARY! <3"), 4, 1);
    drawHeart(64, 32, 14);
    printCentered(F("Forever & Always <3"), 50, 1);
  }
}

// ── Render: Live Love Note Sent From Phone ───────────────────────────────────
void renderLiveNote() {
  printCentered(String(F("Note for ")) + cfgGfName + F(" <3"), 4, 1);

  printCentered(liveNoteLine1, 20, 2);
  printCentered(liveNoteLine2, 38, 2);

  printCentered(F("[Click to Dismiss]"), 55, 1);
}

// ── Render: Wi-Fi Portal Status ─────────────────────────────────────────────
void renderWiFiPortal() {
  printCentered(F("Phone Web Setup"), 4, 1);

  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(4, 18);
    display.print(F("Connected to:"));
    display.setCursor(4, 28);
    display.print(cfgWifiSSID);
    display.setCursor(4, 42);
    display.print(F("Open on phone:"));
    display.setCursor(4, 52);
    display.print(F("http://iglet.local"));
  } else {
    display.setCursor(4, 18);
    display.print(F("1. Join Hotspot:"));
    printCentered(F("Iglet-Setup"), 28, 1);
    display.setCursor(4, 42);
    display.print(F("2. Open in browser:"));
    printCentered(F("http://192.168.4.1"), 52, 1);
  }
}

// ── Render: Timer Up ────────────────────────────────────────────────────────
void renderTimerUp() {
  printCentered(F("Focus! ^"), 4, 1);
  printCentered(String(elapsedMinutes), 24, 3);
}

// ── Render: Timer Down ──────────────────────────────────────────────────────
void renderTimerDown() {
  printCentered((currentState == SELECTING_DOWN_DURATION) ? F("Set Time (min)") : F("Focus! v"), 4, 1);
  printCentered(String(countdownValue), 24, 3);
}

// ── Render: Pomodoro Select ─────────────────────────────────────────────────
void renderPomoSelect(bool isFocus) {
  printCentered(isFocus ? F("Set Focus (min)") : F("Set Rest (min)"), 4, 1);
  printCentered(String(countdownValue), 24, 3);
}

// ── Render: Pomodoro Running ────────────────────────────────────────────────
void renderPomoRunning(bool isFocus) {
  printCentered(isFocus ? F("Focus! v") : F("Rest! <3"), 4, 1);
  printCentered(String(countdownValue), 24, 3);
}

// ── Render: Love Notes ──────────────────────────────────────────────────────
void renderLoveNote() {
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.print(F("To "));
  display.print(cfgGfName);
  display.print(F(" <3"));

  display.setCursor(96, 4);
  display.print(currentNoteIndex + 1);
  display.print(F("/"));
  display.print(LOVE_NOTE_COUNT);

  uint8_t idx = (currentNoteIndex % LOVE_NOTE_COUNT) * 3;

  printCentered(loveNotes[idx], 20, 2);
  printCentered(loveNotes[idx + 1], 38, 2);
}

// ── Render: Compliment ──────────────────────────────────────────────────────
void renderCompliment() {
  if (currentState == COMPLIMENT_RESULT) {
    printCentered(F("You're"), 6, 1);
  } else {
    printCentered(F("Spinning... *"), 6, 1);
  }

  const char* comp = compliments[rouletteIndex % COMPLIMENT_COUNT];
  printCentered(comp, 26, 3);

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
  printCentered(F("Ask me!"), 4, 1);

  if (currentState == DECISION_RESULT) {
    const char* answer = oracleAnswers[decisionIndex % ORACLE_COUNT];
    printCentered(answer, 26, 3);
  } else {
    printCentered(F("?"), 24, 4);
  }
}

// ── Render: Breathing Select ────────────────────────────────────────────────
void renderBreathSelect() {
  printCentered(F("Breathe"), 4, 1);
  printCentered(F("Cycles:"), 20, 1);
  printCentered(String(breathCycles), 34, 3);
}

// ── Render: Mood Select ─────────────────────────────────────────────────────
void renderMoodSelect() {
  printCentered(F("How are you?"), 4, 1);
  drawMoodFace(moodSelection, 64, 34, 12);
  printCentered(moodLabels[moodSelection], 52, 1);
}

// ── Render: Mood Response ───────────────────────────────────────────────────
void renderMoodResponse() {
  printCentered(moodResponses[moodSelection][0], 18, 2);
  printCentered(moodResponses[moodSelection][1], 38, 2);
}

// ── Render: Auto-Wake Interval Setting ──────────────────────────────────────
void renderGreetSelect() {
  printCentered(F("Auto Wake"), 4, 1);
  printCentered(F("Wake every:"), 20, 1);
  printCentered(greetLabels[greetIntervalIdx], 34, 3);
}

// ── Render: Gentle Reminder ─────────────────────────────────────────────────
void renderReminder() {
  printCentered(F("Gentle Reminder <3"), 4, 1);

  uint8_t rIdx = (currentReminderIndex % REMINDER_COUNT) * 3;
  printCentered(reminderMsgs[rIdx], 20, 2);
  printCentered(reminderMsgs[rIdx + 1], 38, 2);
}

// ── Render: Greeting ────────────────────────────────────────────────────────
void renderGreeting() {
  renderLoveNote();
}

// ── Render: Auto-Wake Teaser / Celebration ──────────────────────────────────
void renderGreetingTeaser() {
  if (teaserType == 1) {
    // Birthday today!
    printCentered(F("HAPPY BIRTHDAY! <3"), 4, 1);
    drawCake(64, 34);
    printCentered(String(cfgGfName) + "!", 48, 2);
  } else if (teaserType == 2) {
    // Anniversary today!
    printCentered(F("HAPPY ANNIVERSARY! <3"), 4, 1);
    drawHeart(64, 32, 14);
    printCentered(F("Forever & Always <3"), 50, 1);
  } else if (teaserType == 3) {
    // Birthday in next week!
    printCentered(F("Who's Birthday is"), 8, 1);
    if (teaserDays == 1) {
      printCentered(F("TOMORROW!"), 24, 2);
    } else {
      String dStr = "in " + String(teaserDays) + " days?!";
      printCentered(dStr, 24, 2);
    }
    printCentered(String(cfgGfName) + F("! <3"), 46, 2);
  } else if (teaserType == 4) {
    // Anniversary in next week!
    printCentered(F("Our Anniversary is"), 8, 1);
    if (teaserDays == 1) {
      printCentered(F("TOMORROW! <3"), 24, 2);
    } else {
      String dStr = "in " + String(teaserDays) + " days!";
      printCentered(dStr, 24, 2);
    }
    printCentered(F("Counting down! <3"), 48, 1);
  }
}

// ============================================================================
//  ANIMATION HANDLERS
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
      printCentered(F("Thinking..."), 4, 1);

      display.setTextSize(4);
      display.setCursor(52, 24);
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

  int minR = 5, maxR = 21;
  int radius = (breathPhase == 0) ? (minR + (int)((maxR - minR) * progress)) :
               (breathPhase == 1) ? maxR : (maxR - (int)((maxR - minR) * progress));

  display.drawCircle(64, 42, radius, WHITE);
  display.drawCircle(64, 42, radius - 1, WHITE);
  display.fillCircle(64, 42, 2, WHITE);

  display.display();
}

// ============================================================================
//  AUTO GREETINGS & REMINDERS
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

    // Check if Anniversary or Birthday is TODAY or in the next week (<= 7 days)!
    int bDays = getDaysUntil(cfgBdayMonth, cfgBdayDay);
    int aDays = getDaysUntil(cfgAnnivMonth, cfgAnnivDay);

    if (bDays == 0) {
      teaserType = 1; // Birthday today!
      teaserDays = 0;
      teaserStart = now;
      currentState = GREETING_TEASER;
    } else if (aDays == 0) {
      teaserType = 2; // Anniversary today!
      teaserDays = 0;
      teaserStart = now;
      currentState = GREETING_TEASER;
    } else if (bDays >= 1 && bDays <= 7 && (aDays < 1 || bDays <= aDays)) {
      teaserType = 3; // Birthday in the next week!
      teaserDays = bDays;
      teaserStart = now;
      currentState = GREETING_TEASER;
    } else if (aDays >= 1 && aDays <= 7) {
      teaserType = 4; // Anniversary in the next week!
      teaserDays = aDays;
      teaserStart = now;
      currentState = GREETING_TEASER;
    } else {
      currentState = GREETING_WAKEUP;
    }

    lastActivityTime = now;
    updateDisplay();
  }
}

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
  }
}

// ============================================================================
//  INPUT HANDLING (Hardware Interrupts + Polling Fallback)
// ============================================================================
ICACHE_RAM_ATTR void handleEncoderInterrupt() {
  static uint8_t prev = 0xFF;
  static int8_t count = 0;
  static unsigned long lastTurnTime = 0;

  uint8_t clk = (uint8_t)digitalRead(CLK);
  uint8_t dt  = (uint8_t)digitalRead(DT);
  uint8_t curr = (clk << 1) | dt;

  if (prev == 0xFF) {
    prev = curr;
    return;
  }

  if (curr != prev) {
    uint8_t idx = (prev << 2) | curr;
    prev = curr;

    static const int8_t encTable[16] = {
       0, -1,  1,  0,
       1,  0,  0, -1,
      -1,  0,  0,  1,
       0,  1, -1,  0
    };

    int8_t step = encTable[idx];
    if (step != 0) {
      count += step;
      if ((curr == 3 || curr == 0) && (count >= 2 || count <= -2)) {
        int result = (count > 0) ? 1 : -1;
        count = 0;
        unsigned long now = millis();
        if (now - lastTurnTime > 35) {
          lastTurnTime = now;
          rotaryDelta += result;
        }
      }
    }
  }
}

ICACHE_RAM_ATTR void handleButtonInterrupt() {
  static unsigned long lastPressTime = 0;
  unsigned long now = millis();
  if (now - lastPressTime > 300) {
    buttonWasClicked = true;
    lastPressTime = now;
  }
}

int getRotation() {
  if (rotaryDelta != 0) {
    noInterrupts();
    int delta = rotaryDelta;
    rotaryDelta = 0;
    interrupts();
    return (delta > 0) ? 1 : -1;
  }
  return 0;
}

bool buttonPressed(unsigned long now) {
  if (buttonIgnoreUntil != 0) {
    if ((long)(now - buttonIgnoreUntil) < 0) {
      buttonWasClicked = false;
      return false;
    }
    buttonIgnoreUntil = 0;
  }

  if (buttonWasClicked) {
    buttonWasClicked = false;
    buttonDebounceTime = now;
    lastActivityTime = now;
    return true;
  }

  static int lastButtonReading = HIGH;
  int reading = digitalRead(SW);

  // Detect falling edge (HIGH -> LOW)
  if (lastButtonReading == HIGH && reading == LOW) {
    if (now - buttonDebounceTime > buttonDebounceDelay) {
      buttonDebounceTime = now;
      lastActivityTime = now;
      lastButtonReading = reading;
      return true;
    }
  }

  lastButtonReading = reading;
  return false;
}

void handleRotaryInput(unsigned long now) {
  int rotation = getRotation();
  if (rotation == 0) return;

  lastActivityTime = now;

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

    case PET_VIEW:
      if (petAnimState == 0) {
        petActionIndex = (petActionIndex + rotation + PET_ACTION_COUNT) % PET_ACTION_COUNT;
        updateDisplay();
      }
      break;

    case DAYS_VIEW:
      daysSubIndex = (daysSubIndex + rotation + 3) % 3;
      updateDisplay();
      break;

    case COMPLIMENT_ROULETTE:
    case COMPLIMENT_RESULT:
      if (!rouletteSpinning) {
        rngState ^= (uint16_t)millis() ^ (uint16_t)analogRead(A0);
        rouletteSpinning = true;
        rouletteLastSpin = millis();
        rouletteIndex = cheapRandom(COMPLIMENT_COUNT);
        currentState = COMPLIMENT_ROULETTE;
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
  if (!buttonPressed(now)) return;

  switch (currentState) {
    case MENU:
      handleMenuSelection(now);
      break;

    case GREETING_TEASER:
      currentState = GREETING_WAKEUP;
      lastGreetingTime = now;
      if (displayOff) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        displayOff = false;
      }
      break;

    case IDLE:
    case GREETING_WAKEUP:
    case REMINDER_DISPLAY:
    case COMPLIMENT_RESULT:
    case DECISION_RESULT:
    case MOOD_RESPONSE:
    case WIFI_PORTAL_VIEW:
    case DAYS_VIEW:
      currentState = MENU;
      if (displayOff) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        displayOff = false;
      }
      break;

    case LIVE_NOTE_DISPLAY:
      if (now - liveNoteStart >= 1200) {
        currentState = MENU;
        if (displayOff) {
          display.ssd1306_command(SSD1306_DISPLAYON);
          displayOff = false;
        }
      }
      break;

    case LOVE_NOTE_DISPLAY:
      currentState = MENU;
      break;

    case PET_VIEW:
      if (petAnimState != 0) {
        petAnimState = 0;
        updateDisplay();
        return;
      }
      if (petActionIndex == 0) {        // Pet
        petAnimState = 1;
        petAnimStart = now;
        if (petLoveLevel < 5) petLoveLevel++;
        saveSettings();
      } else if (petActionIndex == 1) { // Feed
        petAnimState = 2;
        petAnimStart = now;
        if (petLoveLevel < 5) petLoveLevel++;
        saveSettings();
      } else if (petActionIndex == 2) { // Play
        petAnimState = 3;
        petAnimStart = now;
        if (petLoveLevel < 5) petLoveLevel++;
        saveSettings();
      } else if (petActionIndex == 3) { // Exit
        currentState = MENU;
      }
      updateDisplay();
      return;

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
      updateDisplay();
      sendTelegramMood(moodSelection);
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
    case 0:  // Pet — Virtual Pet
      currentState = PET_VIEW;
      petActionIndex = 0;
      petAnimState = 0;
      break;

    case 1:  // Days — Event Countdowns (Anniversary, Birthday, Clock)
      daysSubIndex = 0;
      currentState = DAYS_VIEW;
      break;

    case 2:  // Spin! — Compliment roulette
      rngState ^= (uint16_t)millis() ^ (uint16_t)analogRead(A0);
      rouletteSpinning = true;
      rouletteLastSpin = now;
      rouletteIndex = cheapRandom(COMPLIMENT_COUNT);
      currentState = COMPLIMENT_ROULETTE;
      break;

    case 3:  // Decide — Decision maker
      currentState = DECISION_ORACLE;
      decisionAnimStart = now;
      decisionIndex = 0;
      break;

    case 4:  // Breathe — Breathing exercise
      breathCycles = 3;
      currentState = BREATHING_SELECT;
      break;

    case 5:  // Mood — Mood check-in
      moodSelection = 0;
      currentState = MOOD_SELECT;
      break;

    case 6:  // WiFi — Status / Setup Hotspot
      if (WiFi.status() != WL_CONNECTED && !apModeActive) {
        startAPMode();
      }
      currentState = WIFI_PORTAL_VIEW;
      break;

    case 7:  // Wake — Auto-wake greeting interval
      currentState = GREET_SELECT;
      break;

    case 8:  // UP — Count up focus timer
      currentState = COUNTING_UP;
      elapsedMinutes = 0;
      isCounting = true;
      previousMillis = now;
      break;

    case 9:  // DOWN — Countdown focus timer
      currentState = SELECTING_DOWN_DURATION;
      countdownValue = 20;
      break;

    case 10: // POMO — Pomodoro focus timer
      currentState = SELECTING_POMO_FOCUS;
      pomodoroFocusMinutes = 25;
      pomodoroRestMinutes = 5;
      countdownValue = pomodoroFocusMinutes;
      break;

    case 11: // Reset — Reset total focus minutes
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
                  currentState == BREATHING_SELECT || currentState == GREET_SELECT ||
                  currentState == PET_VIEW || currentState == DAYS_VIEW ||
                  currentState == WIFI_PORTAL_VIEW);

  if (mayIdle && (now >= lastActivityTime) && (now - lastActivityTime > inactivityLimit)) {
    if (currentState != IDLE) {
      currentState = IDLE;
      idleStartTime = now;
      updateDisplay();
      Serial.println(F("Entered IDLE state."));
    }
  }

  if (currentState == IDLE && !displayOff && (now >= idleStartTime) &&
      (now - idleStartTime > displayOffTimeLimit)) {
    displayOff = true;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    Serial.println(F("Display turned off."));
  }
}
