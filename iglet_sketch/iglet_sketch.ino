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
  LIVE_NOTE_DISPLAY,     // Note sent live from phone!
  CELEBRATION_DISPLAY    // Birthday or Anniversary celebration!
};
State currentState = BOOT_ANIM;

// ── Main Menu ───────────────────────────────────────────────────────────────
const char* const mainMenuItems[] = {
  "Notes",       // 0: Love notes
  "Pet",         // 1: Virtual Pet (Tamagotchi-lite)
  "Days",        // 2: Event Countdowns (Anniversary, Birthday, Clock)
  "Spin!",       // 3: Compliment roulette
  "Decide",      // 4: Decision maker
  "Breathe",     // 5: Breathing exercise
  "Mood",        // 6: Mood check-in
  "WiFi",        // 7: Wi-Fi status / Web Portal
  "Wake",        // 8: Auto-wake greeting interval
  "UP",          // 9: Count up focus timer
  "DOWN",        // 10: Countdown focus timer
  "POMO",        // 11: Pomodoro focus timer
  "Reset"        // 12: Reset total flow minutes
};
const uint8_t MAIN_MENU_COUNT = 13;

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
unsigned long lastRotaryTime = 0;
const unsigned long rotaryDebounceDelay = 120;

// ── Inactivity & Sleep ──────────────────────────────────────────────────────
const unsigned long inactivityLimit = 2UL * 60000UL;      // 2 min to sleep
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
#define EEPROM_SSID_ADDR   48    // 64 bytes (char string)
#define EEPROM_PASS_ADDR   112   // 64 bytes (char string)
#define EEPROM_MAGIC_A     0xAC
#define EEPROM_MAGIC_B     0xE3

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
void renderLiveNote();
void renderCelebration();

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("\nStarting Iglet..."));

  EEPROM.begin(EEPROM_SIZE);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring (0x3C/0x3D)"));
    for (;;);
  }

  display.clearDisplay();
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

  // Auto-dismiss live note sent from phone
  if (currentState == LIVE_NOTE_DISPLAY && (now - liveNoteStart > 15000)) {
    currentState = MENU;
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
  if (server.hasArg("tz")) cfgTzOffset = server.arg("tz").toFloat();

  if (server.hasArg("ssid") && server.arg("ssid").length() > 0) {
    strncpy(cfgWifiSSID, server.arg("ssid").c_str(), sizeof(cfgWifiSSID) - 1);
  }
  if (server.hasArg("pass") && server.arg("pass").length() > 0) {
    strncpy(cfgWifiPass, server.arg("pass").c_str(), sizeof(cfgWifiPass) - 1);
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
    updateDisplay();

    String res = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                   "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#ff9a9e;}"
                   ".c{background:#fff;padding:30px;border-radius:15px;max-width:360px;margin:auto;}"
                   "h2{color:#e63946;}a{color:#e63946;font-weight:700;}</style></head><body>"
                   "<div class='c'><h2>Sent with Love! 💖</h2><p>Your note is on ");
    res += String(cfgGfName);
    res += F("'s desk right now!</p><p><a href='/'>Send another note</a></p></div></body></html>");
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
  int curYear = t->tm_year + 1900;

  struct tm target = *t;
  target.tm_year = curYear - 1900;
  target.tm_mon  = targetMonth - 1;
  target.tm_mday = targetDay;
  target.tm_hour = 0;
  target.tm_min  = 0;
  target.tm_sec  = 0;

  time_t targetTime = mktime(&target);
  double diffSec = difftime(targetTime, now);

  if (diffSec < -86400.0) {
    target.tm_year = curYear - 1900 + 1;
    targetTime = mktime(&target);
    diffSec = difftime(targetTime, now);
  }

  int days = (int)(diffSec / 86400.0) + 1;
  if (days < 0) days = 0;
  return days;
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
    String greeting = "Hi " + String(cfgGfName) + "!";
    int len = greeting.length();
    int tSize = (len > 9) ? 1 : 2;
    display.setTextSize(tSize);
    int charW = (tSize == 2) ? 12 : 6;
    int w = len * charW;
    display.setCursor((128 - w) / 2, (tSize == 2) ? 0 : 4);
    display.print(greeting);

    int heartPhase = ((elapsed - 600) / 250) % 4;
    int heartSize = (heartPhase < 2) ? 9 + heartPhase * 3 : 15 - heartPhase * 2;
    drawHeart(64, 44, heartSize);

    display.display();
  } else {
    // Check if today is a birthday or anniversary celebration!
    checkDateCelebration();
    if (currentState != CELEBRATION_DISPLAY) {
      currentState = MENU;
      updateDisplay();
    }
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
    case 0: // Happy
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        display.drawPixel(cx + i, cy + r / 4 + (i * i) / (r * 2), WHITE);
      }
      break;
    case 1: // Okay
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      display.drawLine(cx - eyeSpacing, cy + r / 4, cx + eyeSpacing, cy + r / 4, WHITE);
      break;
    case 2: // Sad
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        display.drawPixel(cx + i, cy + r / 2 - (i * i) / (r * 2), WHITE);
      }
      break;
    case 3: // Angry
      display.drawLine(cx - eyeSpacing - 2, eyeY - 4, cx - eyeSpacing + 2, eyeY - 2, WHITE);
      display.drawLine(cx + eyeSpacing + 2, eyeY - 4, cx + eyeSpacing - 2, eyeY - 2, WHITE);
      display.fillCircle(cx - eyeSpacing, eyeY, 2, WHITE);
      display.fillCircle(cx + eyeSpacing, eyeY, 2, WHITE);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        display.drawPixel(cx + i, cy + r / 2 - (i * i) / (r * 2), WHITE);
      }
      break;
    case 4: // Love
      drawHeart(cx - eyeSpacing, eyeY, 4);
      drawHeart(cx + eyeSpacing, eyeY, 4);
      for (int i = -eyeSpacing; i <= eyeSpacing; i++) {
        display.drawPixel(cx + i, cy + r / 4 + (i * i) / r, WHITE);
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

  // Large menu text
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
    display.setCursor(20, 4);
    display.print(F("Our Anniversary <3"));

    int days = getDaysUntil(cfgAnnivMonth, cfgAnnivDay);
    if (days == -1) {
      display.setTextSize(1);
      display.setCursor(8, 28);
      display.print(F("Sync WiFi in menu"));
      display.setCursor(8, 42);
      display.print(F("or set dates on phone"));
    } else if (days == 0) {
      display.setTextSize(2);
      display.setCursor(20, 32);
      display.print(F("TODAY!! <3"));
    } else {
      display.setTextSize(3);
      String s = "in " + String(days) + "d";
      int w = s.length() * 18;
      display.setCursor((128 - w) / 2, 28);
      display.print(s);
    }
  } else if (daysSubIndex == 1) {
    // Birthday countdown
    display.setCursor(16, 4);
    display.print(String(cfgGfName) + "'s Birthday *");

    int days = getDaysUntil(cfgBdayMonth, cfgBdayDay);
    if (days == -1) {
      display.setTextSize(1);
      display.setCursor(8, 28);
      display.print(F("Sync WiFi in menu"));
      display.setCursor(8, 42);
      display.print(F("or set dates on phone"));
    } else if (days == 0) {
      display.setTextSize(2);
      display.setCursor(20, 32);
      display.print(F("TODAY!! 🎂"));
    } else {
      display.setTextSize(3);
      String s = "in " + String(days) + "d";
      int w = s.length() * 18;
      display.setCursor((128 - w) / 2, 28);
      display.print(s);
    }
  } else {
    // Live Clock & Date
    time_t now = time(nullptr);
    if (now > 100000) {
      struct tm* t = localtime(&now);
      char dateBuf[20];
      strftime(dateBuf, sizeof(dateBuf), "%d %b %Y", t);
      display.setCursor((128 - strlen(dateBuf) * 6) / 2, 4);
      display.print(dateBuf);

      char timeBuf[10];
      strftime(timeBuf, sizeof(timeBuf), "%H:%M", t);
      display.setTextSize(3);
      display.setCursor((128 - 5 * 18) / 2, 28);
      display.print(timeBuf);
    } else {
      display.setCursor(24, 4);
      display.print(F("Internet Clock"));
      display.setTextSize(1);
      display.setCursor(14, 32);
      display.print(F("Connecting WiFi..."));
    }
  }

  // Bottom dots indicator (3 pages)
  for (uint8_t i = 0; i < 3; i++) {
    if (i == daysSubIndex) display.fillCircle(56 + i * 8, 60, 2, WHITE);
    else display.drawCircle(56 + i * 8, 60, 1, WHITE);
  }
}

// ── Render: Feature 11: Celebration Display ─────────────────────────────────
void renderCelebration() {
  if (celebrationType == 1) {
    // Birthday!
    display.setTextSize(1);
    display.setCursor(14, 4);
    display.print(F("HAPPY BIRTHDAY! <3"));
    drawCake(64, 38);
    display.setTextSize(2);
    String n = String(cfgGfName) + "!";
    display.setCursor((128 - n.length() * 12) / 2, 48);
    display.print(n);
  } else {
    // Anniversary!
    display.setTextSize(1);
    display.setCursor(8, 4);
    display.print(F("HAPPY ANNIVERSARY! <3"));
    drawHeart(64, 36, 16);
    display.setTextSize(1);
    display.setCursor(24, 52);
    display.print(F("Forever & Always"));
  }
}

// ── Render: Live Love Note Sent From Phone ───────────────────────────────────
void renderLiveNote() {
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.print(F("Live Note for "));
  display.print(cfgGfName);
  display.print(F(" <3"));

  int len1 = strlen(liveNoteLine1);
  int sz1 = (len1 > 7) ? 2 : 3;
  display.setTextSize(sz1);
  int w1 = len1 * ((sz1 == 3) ? 18 : 12);
  display.setCursor((128 - w1) / 2, 22);
  display.print(liveNoteLine1);

  int len2 = strlen(liveNoteLine2);
  int sz2 = (len2 > 7) ? 2 : 3;
  display.setTextSize(sz2);
  int w2 = len2 * ((sz2 == 3) ? 18 : 12);
  display.setCursor((128 - w2) / 2, 42);
  display.print(liveNoteLine2);

  drawHeart(118, 56, 4);
}

// ── Render: Wi-Fi Portal Status ─────────────────────────────────────────────
void renderWiFiPortal() {
  display.setTextSize(1);
  display.setCursor(20, 4);
  display.print(F("Phone Web Setup"));

  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(4, 22);
    display.print(F("Connected to:"));
    display.setCursor(4, 32);
    display.print(cfgWifiSSID);
    display.setCursor(4, 44);
    display.print(F("Open on phone:"));
    display.setCursor(4, 54);
    display.print(F("http://iglet.local"));
  } else {
    display.setCursor(4, 20);
    display.print(F("1. Join Wi-Fi:"));
    display.setTextSize(2);
    display.setCursor(4, 30);
    display.print(F("Iglet-Setup"));
    display.setTextSize(1);
    display.setCursor(4, 48);
    display.print(F("2. Open: 192.168.4.1"));
  }
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

// ── Render: Timer Down ──────────────────────────────────────────────────────
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
  display.print(cfgGfName);
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

// ── Render: Greeting ────────────────────────────────────────────────────────
void renderGreeting() {
  renderLoveNote();
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

    currentState = GREETING_WAKEUP;
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
    case LIVE_NOTE_DISPLAY:
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
    case LIVE_NOTE_DISPLAY:
    case WIFI_PORTAL_VIEW:
    case DAYS_VIEW:
      currentState = MENU;
      if (displayOff) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        displayOff = false;
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

    case 1:  // Pet — Virtual Pet
      currentState = PET_VIEW;
      petActionIndex = 0;
      petAnimState = 0;
      break;

    case 2:  // Days — Event Countdowns (Anniversary, Birthday, Clock)
      daysSubIndex = 0;
      currentState = DAYS_VIEW;
      break;

    case 3:  // Spin! — Compliment roulette
      rouletteSpinning = false;
      rouletteIndex = 0;
      currentState = COMPLIMENT_ROULETTE;
      break;

    case 4:  // Decide — Decision maker
      currentState = DECISION_ORACLE;
      decisionAnimStart = now;
      decisionIndex = 0;
      break;

    case 5:  // Breathe — Breathing exercise
      breathCycles = 3;
      currentState = BREATHING_SELECT;
      break;

    case 6:  // Mood — Mood check-in
      moodSelection = 0;
      currentState = MOOD_SELECT;
      break;

    case 7:  // WiFi — Status / Setup Hotspot
      if (WiFi.status() != WL_CONNECTED && !apModeActive) {
        startAPMode();
      }
      currentState = WIFI_PORTAL_VIEW;
      break;

    case 8:  // Wake — Auto-wake greeting interval
      currentState = GREET_SELECT;
      break;

    case 9:  // UP — Count up focus timer
      currentState = COUNTING_UP;
      elapsedMinutes = 0;
      isCounting = true;
      previousMillis = now;
      break;

    case 10: // DOWN — Countdown focus timer
      currentState = SELECTING_DOWN_DURATION;
      countdownValue = 20;
      break;

    case 11: // POMO — Pomodoro focus timer
      currentState = SELECTING_POMO_FOCUS;
      pomodoroFocusMinutes = 25;
      pomodoroRestMinutes = 5;
      countdownValue = pomodoroFocusMinutes;
      break;

    case 12: // Reset — Reset total focus minutes
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
