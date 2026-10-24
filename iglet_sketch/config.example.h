#ifndef IGLET_CONFIG_H
#define IGLET_CONFIG_H

// ============================================================================
//  IGLET — Private Personalization Settings
//  Instructions:
//    1. Copy this file and save it as "config.h" in the same folder.
//    2. Edit your girlfriend's real name, private love notes, and inside jokes below.
//    3. "config.h" is gitignored, so it will NEVER be uploaded to GitHub!
// ============================================================================

// ── Her Name & Companion Name ───────────────────────────────────────────────
#define GF_NAME       "Cutie"          // Her real name or nickname (e.g. "Maya", "Emma", "Babe")
#define DEVICE_NAME   "Iglet"          // Name of her little companion

// ── Optional Wi-Fi & Special Dates ──────────────────────────────────────────
// Tip: You can also set these wirelessly from your phone via the web portal!
#define WIFI_SSID           ""         // Your home Wi-Fi network name
#define WIFI_PASS           ""         // Your home Wi-Fi password
#define TIMEZONE_OFFSET_HRS 5.5        // e.g. 5.5 for IST (+05:30), -5 for EST, 1 for CET
#define ANNIVERSARY_MONTH   0          // Month (1-12, or 0 if not set yet)
#define ANNIVERSARY_DAY     0          // Day (1-31)
#define GF_BIRTHDAY_MONTH   0          // Month (1-12)
#define GF_BIRTHDAY_DAY     0          // Day (1-31)

// ── Love Notes: 20 personal messages ────────────────────────────────────────
// Stored as triplets: line1 (yellow zone), line2 (blue zone), separator "---"
// Tip: Keep line1 and line2 under ~10 characters each for large crisp text.
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

// ── Decision Maker (Magic 8-Ball / Fun date & food dilemmas) ────────────────
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

#endif // IGLET_CONFIG_H
