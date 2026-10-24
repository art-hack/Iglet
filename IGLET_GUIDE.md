# 🎀 IGLET — A Cute Desk Companion & Focus Partner Gift

> **Based on Project IGOR by Urban Circles**  
> 3D Model & Original Build: [MakerWorld Model #1000048](https://makerworld.com/en/models/1000048-project-igor-open-source-offline-focus-partner)  
> Custom Firmware: `iglet_sketch/iglet_sketch.ino`

---

## 📖 What is Iglet?

**Iglet** transforms the minimalist 3D-printed focus partner into an adorable, thoughtful desk companion gift for your girlfriend. It preserves the original focus tools (UP timer, DOWN timer, Pomodoro sessions, and flow minutes tracking) while adding sweet interactive companion features:

1. 💌 **Love Notes (`Notes`)**: 20 personal notes that she can browse with the rotary dial (previous/next note) or read on auto-wake.
2. 🐾 **Virtual Pet (`Pet`)**: A cute animated desktop creature! Choose between **`Pet`** (joyful wiggles + floating hearts), **`Feed`** (nom nom mouth + treat), **`Play`** (hopping + sparkles), and **`Exit`**. Tracks a 5-heart love level in EEPROM!
3. 📅 **Event Countdowns & NTP Clock (`Days`)**: Live countdown to her Birthday and your Anniversary! Synchronizes accurate real-world time automatically via Wi-Fi NTP (`pool.ntp.org`).
4. 🎂 **Special Day Celebrations**: When the calendar hits her Birthday or your Anniversary, Iglet automatically plays a surprise celebration screen (birthday cake with glowing candles or pulsing hearts)!
5. 📱 **Phone Setup Portal (`WiFi`)**: Zero-code wireless configuration! Connect your phone to `Iglet-Setup` to update her name, birthday, anniversary, and Wi-Fi without ever touching code or reflashing.
6. 🚀 **Live Desk Note Sender (`http://iglet.local`)**: Send surprise real-time love notes directly to her desk screen from your phone or laptop whenever you are on the same Wi-Fi!
7. 🎰 **Compliment Roulette (`Spin!`)**: Spin the dial or click to roll a slot-machine-style compliment with deceleration and sparkle hearts.
8. 🔮 **Decision Maker (`Decide`)**: Magic 8-Ball decider for dates, food, or silly dilemmas ("Should we get bubble tea?"). Rapid animated thinking then reveals an answer.
9. 🫁 **Calm Breathing Exercise (`Breathe`)**: 4-2-6 calming breathing session with a smooth pulsing circle animation. Configurable cycles (1 to 20).
10. 😊 **Mood Check-in (`Mood`)**: Rotate through 5 cute pixel faces (Happy, Okay, Sad, Angry, Love) to receive a warm, tailored comforting message.
11. ⏰ **Auto-Wake Greeting (`Wake`)**: Configurable interval (`Off`, `30m`, `1 hr`, `2 hr`, `4 hr`) that wakes up the screen, shows a random love note, and sleeps again. Persists in EEPROM!
12. 💧 **Gentle Reminders**: Periodic reminders every 45 min ("Drink water!", "Stretch break!", "Deep breath in", "You are amazing <3", "Look up & blink", "Stand up & shake it!").
13. 💤 **Sleepy Pet Idle**: When inactive for 2 minutes, your pet curls up asleep `(-.-)zzZ` on screen! Display turns off after 30 minutes to conserve power.
14. ⏱️ **Focus Partner Timers (`UP`, `DOWN`, `POMO`, `Reset`)**: Original count-up stopwatch, countdown timer with rotary duration selection, Pomodoro focus/rest cycles, and total flow minutes tracking saved to EEPROM.

---

## 📱 Wireless Phone Setup & Live Desk Notes (No Flashing Needed!)

Iglet comes with a built-in mobile web portal and Wi-Fi hotspot. You do **not** need to edit code or reflash Arduino to change her name, set dates, or configure Wi-Fi!

```
               ┌───────────────────────────────────────────┐
               │         1. FIRST TIME SETUP (Hotspot)     │
               │  Connect phone to Wi-Fi: "Iglet-Setup"    │
               │  Captive portal opens: 192.168.4.1        │
               │  Set: Her Name, Birthday, Anniversary,   │
               │       Timezone & Home Wi-Fi credentials   │
               └─────────────────────┬─────────────────────┘
                                     │ (Saves to EEPROM & connects)
                                     ▼
               ┌───────────────────────────────────────────┐
               │         2. EVERYDAY DESK DASHBOARD        │
               │  On home Wi-Fi, open: http://iglet.local  │
               │  💌 Send surprise live notes to her desk  │
               │  ⏰ Iglet syncs time with NTP pool        │
               │  🎂 Celebrates birthday & anniversary     │
               └───────────────────────────────────────────┘
```

### Step 1: Connect to Setup Hotspot
1. Turn on Iglet. If no Wi-Fi credentials are saved (or if you click **`WiFi`** in the menu), Iglet broadcasts an open Wi-Fi network:
   - **Network Name (SSID):** `Iglet-Setup`
   - **Password:** None (Open)
2. On your phone, connect to **`Iglet-Setup`**.
3. A captive portal popup will appear automatically. (If not, open your mobile browser and visit `http://192.168.4.1`).

### Step 2: Configure Settings
Fill out the mobile-friendly form:
- **Her Name or Nickname**: (e.g. `Cutie`, `Maya`, `Emma`)
- **🎂 Birthday Month & Day**: Used for countdown and birthday celebration!
- **💍 Anniversary Month & Day**: Used for anniversary countdown!
- **⏰ Timezone Offset**: (e.g. `5.5` for IST +05:30, `-5.0` for EST, `1.0` for CET)
- **📶 Home Wi-Fi SSID & Password**: Iglet uses this to fetch online time.
- Tap **Save & Connect ✨**.

All settings are written to **non-volatile EEPROM memory**, so they are remembered even when unplugged!

### Step 3: Send Live Notes to Her Screen!
Once Iglet connects to your home Wi-Fi:
1. Open your phone or laptop browser and go to:
   - **`http://iglet.local`** (or the IP address shown on the Iglet screen in the `WiFi` menu).
2. You will see the **💌 Send Live Note to Her Desk!** form.
3. Enter:
   - **Top Line (Yellow zone)**: e.g. `Thinking of`
   - **Bottom Line (Blue zone)**: e.g. `you always <3`
4. Tap **Send to Her Screen 🚀**!
5. Iglet instantly wakes up from sleep, plays a floating hearts animation, and displays your note for 15 seconds!

---

## 🛠️ Bill of Materials (BOM)

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **Wemos D1 Mini (ESP8266)** | 1 | USB-C recommended (USB-Micro also works) |
| **0.96" SSD1306 I2C OLED** | 1 | Dual color (Yellow top 16px, Blue bottom 48px) |
| **KY-040 Rotary Encoder** | 1 | With built-in push button switch |
| **3D Printed Case** | 1 set | Press-fit, no screws or glue needed ([MakerWorld](https://makerworld.com/en/models/1000048)) |
| **USB Cable** | 1 | Connects to laptop / phone charger for power |

---

## 🔌 Pinout & Wiring Diagram

Connect the components to the D1 Mini as follows:

| Component Pin | D1 Mini Pin | GPIO | Function |
| :--- | :--- | :--- | :--- |
| **OLED SDA** | `D2` | GPIO4 | I2C Data |
| **OLED SCL** | `D1` | GPIO5 | I2C Clock |
| **OLED VCC** | `3V3` (or `5V`) | — | Power |
| **OLED GND** | `G` | — | Ground |
| **Encoder CLK** | `D6` | GPIO12 | Rotary Clock |
| **Encoder DT** | `D7` | GPIO13 | Rotary Direction |
| **Encoder SW** | `D4` | GPIO2 | Push Button |
| **Encoder + (VCC)**| `3V3` | — | Power |
| **Encoder GND** | `G` | — | Ground |

> [!NOTE]  
> If the rotary encoder turns the opposite direction of what you expect, simply swap the `D6` (CLK) and `D7` (DT) wires, or swap the pin definitions in the code!

---

## 💻 Arduino IDE Setup & Flashing

### Step 1: Install ESP8266 Board Support
1. Open **Arduino IDE**.
2. Go to `File` → `Preferences`.
3. In **Additional boards manager URLs**, add:  
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
4. Go to `Tools` → `Board` → `Boards Manager...`, search for `esp8266`, and click **Install**.

### Step 2: Install Libraries
In Arduino IDE, go to `Tools` → `Manage Libraries...` and install:
- **Adafruit SSD1306** by Adafruit
- **Adafruit GFX Library** by Adafruit

*(The Wi-Fi, WebServer, DNS, mDNS, and EEPROM libraries are built directly into the ESP8266 board core package!)*

### Step 3: Open and Flash the Sketch
1. Open `iglet_sketch/iglet_sketch.ino`.
2. Select Board: `Tools` → `Board` → `ESP8266 Boards` → **LOLIN(WEMOS) D1 R2 & mini**.
3. Select Port: `Tools` → `Port` → Select your D1 Mini COM port.
4. Click **Upload** (Arrow icon).

---

## 🎨 How to Personalize (100% Private!)

There are two ways to personalize Iglet:

### Method A: Wirelessly via Phone Portal (Easiest & No Code!)
Simply power Iglet, connect to the `Iglet-Setup` Wi-Fi, and type her name, birthday, anniversary, and home Wi-Fi into the form. It saves directly to EEPROM.

### Method B: Private `config.h` (Pre-flashed Secrets)
If you want to bake her name and 20 custom love notes into the firmware before gifting it to her:
1. In the `iglet_sketch/` folder, copy **`config.example.h`** and name the copy **`config.h`**:
   - `config.h` is strictly ignored by Git (`.gitignore`), so **`git push` can NEVER upload it to GitHub!**
   - The sketch will automatically detect and load `config.h`. If `config.h` is omitted, it safely falls back to cute generic defaults.
2. Open `iglet_sketch/config.h` and customize:

```cpp
#define GF_NAME             "Cutie"          // Her real name or nickname
#define DEVICE_NAME         "Iglet"          // Name of her little companion
#define TIMEZONE_OFFSET_HRS 5.5              // e.g. 5.5 for IST (+05:30)
#define ANNIVERSARY_MONTH   11               // Month (1-12)
#define ANNIVERSARY_DAY     15               // Day (1-31)
#define GF_BIRTHDAY_MONTH   8                // Month (1-12)
#define GF_BIRTHDAY_DAY     24               // Day (1-31)
```

You can also customize the 20 love notes, roulette compliments, and decision maker answers in `config.h`.

---

## 🎮 How to Use Iglet

| Action | Control | Result |
| :--- | :--- | :--- |
| **Navigate Menu** | Rotate Dial | Cycles through: `Pet` → `Days` → `Spin!` → `Decide` → `Breathe` → `Mood` → `WiFi` → `Wake` → `UP` → `DOWN` → `POMO` → `Reset` |
| **Select / Enter** | Click Dial | Opens selected feature |
| **Virtual Pet** | Rotate in `Pet` | Select `Pet`, `Feed`, `Play`, or `Exit`; Click to interact & raise Love Level |
| **Days / Clock** | Rotate in `Days` | Toggle between Anniversary countdown, Birthday countdown, and live NTP clock |
| **Spin Roulette** | Rotate or Click in `Spin!` | Triggers spin animation, reveals compliment |
| **Ask Decider** | Click in `Decide` | Plays thinking animation (`?`, `!`, `*`) and reveals answer |
| **Breathing** | Rotate in `Breathe` | Sets cycles (1 to 20), Click starts 4-2-6 calming breathing session |
| **Check Mood** | Rotate in `Mood` | Pick Happy, Okay, Sad, Angry, or Love face; Click for response |
| **WiFi Status / Portal**| Click `WiFi` | Shows connection status & IP address. If offline, activates `Iglet-Setup` AP |
| **Auto-Wake** | Rotate in `Wake` | Pick interval: `Off`, `30m`, `1 hr`, `2 hr`, `4 hr`; Click to save |
| **Focus Timers** | Standard Igor flow | UP (stopwatch), DOWN (countdown), POMO (Pomodoro) |
| **Wake from Sleep** | Rotate or Click | Instantly wakes screen back to menu |

---

## 💡 Gift Presentation Ideas

- **The Mystery Hotspot**: Hand her the little device plugged into a portable power bank. Ask her to check her phone's Wi-Fi list for `Iglet-Setup` and watch her smile when the setup page asks for her name!
- **Cute Box & Note**: Place Iglet inside a small gift box with a handwritten card: *"Turn the knob to spin, breathe, or hear how much I love you!"*
- **Cute Colored Cable**: Pair with a pastel braided USB-C cable (pink, lavender, or cream).
- **Custom 3D Print**: Print the front shell in her favorite color and the back cover in white/ivory for a dual-tone aesthetic!
