# 🎀 IGLET — A Cute Desk Companion & Focus Partner Gift

> **Based on Project IGOR by Urban Circles**  
> 3D Model & Original Build: [MakerWorld Model #1000048](https://makerworld.com/en/models/1000048-project-igor-open-source-offline-focus-partner)  
> Custom Firmware: `iglet_sketch/iglet_sketch.ino`

---

## 📖 What is Iglet?

**Iglet** transforms the minimalist 3D-printed focus partner into an adorable, thoughtful desk companion gift for your girlfriend. It preserves the original focus tools (UP timer, DOWN timer, Pomodoro sessions, and flow minutes tracking) while adding **7 sweet, interactive companion features**:

1. 💌 **Love Notes (`Notes`)**: 20 personal notes that she can browse with the rotary dial (previous/next note) or read on auto-wake.
2. 🎰 **Compliment Roulette (`Spin!`)**: Spin the dial or click to roll a slot-machine-style compliment with deceleration and sparkle hearts.
3. 🔮 **Decision Maker (`Decide`)**: Magic 8-Ball decider for dates, food, or silly dilemmas ("Should we get bubble tea?"). Rapid animated thinking then reveals an answer.
4. 🫁 **Calm Breathing Exercise (`Breathe`)**: 4-2-6 calming breathing session with a smooth pulsing circle animation. Configurable cycles (1 to 20).
5. 😊 **Mood Check-in (`Mood`)**: Rotate through 5 cute pixel faces (Happy, Okay, Sad, Angry, Love) to receive a warm, tailored comforting message.
6. ⏰ **Auto-Wake Greeting (`Wake`)**: Configurable interval (`Off`, `30m`, `1 hr`, `2 hr`, `4 hr`) that wakes up the screen, shows a random love note, and sleeps again. Persists in EEPROM!
7. 💧 **Gentle Reminders**: Periodic reminders every 45 min ("Drink water!", "Stretch break!", "Deep breath in", "You are amazing <3", "Look up & blink", "Stand up & shake it!").
8. 💖 **Cute Boot Animation**: Shows `"Hi [Her Name]!"` with a beating heart animation every time it turns on.
9. 💤 **Sleepy Idle Face**: Shows `(-_-)zzZ` when inactive for 2 minutes, then powers down OLED after 30 minutes to save power and prevent burn-in.
10. ⏱️ **Focus Partner Timers (`UP`, `DOWN`, `POMO`, `Reset`)**: Original count-up stopwatch, countdown timer with rotary duration selection, Pomodoro focus/rest cycles, and total flow minutes tracking saved to EEPROM.

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

### Step 3: Open the Sketch
1. Open `iglet_sketch/iglet_sketch.ino`.
2. Select Board: `Tools` → `Board` → `ESP8266 Boards` → **LOLIN(WEMOS) D1 R2 & mini**.
3. Select Port: `Tools` → `Port` → Select your D1 Mini COM port.
4. Click **Upload** (Arrow icon).

---

## 🎨 How to Personalize for Your Girlfriend

Open `iglet_sketch/iglet_sketch.ino` and customize the top sections:

### 1. Her Name & Device Name
```cpp
#define GF_NAME       "Cutie"          // Her name or nickname (e.g. "Maya", "Babe", "Emma")
#define DEVICE_NAME   "Iglet"          // Name of her little companion
```

### 2. Love Notes (20 messages)
Stored as pairs of lines (`line1` in yellow zone, `line2` in blue zone):
```cpp
const char* const loveNotes[] = {
  "You are my",    "sunshine!",    "---",
  "I love you",    "3000 <3",      "---",
  "You make me",   "so happy!",    "---",
  // Add your inside jokes, anniversary dates, or favorite memories!
};
```

### 3. Compliments for Roulette
Add personalized compliments:
```cpp
const char* const compliments[] = {
  "Amazing!",
  "Stunning!",
  "10/10 !!",
  "Gorgeous!",
  "Brilliant",
  // etc.
};
```

### 4. Decision Maker Answers
Tailor them for fun decisions:
```cpp
const char* const oracleAnswers[] = {
  "Yes!",
  "Nope!",
  "Do it!",
  "Boba time!",
  "Pizza date!",
  "100%!",
  // etc.
};
```

---

## 🎮 How to Use Iglet

| Action | Control | Result |
| :--- | :--- | :--- |
| **Navigate Menu** | Rotate Dial | Cycles through: `Notes` → `Spin!` → `Decide` → `Breathe` → `Mood` → `Wake` → `UP` → `DOWN` → `POMO` → `Reset` |
| **Select / Enter** | Click Dial | Opens selected feature |
| **Love Notes** | Rotate in `Notes` | Browses through all 20 notes (`1/20`, `2/20`...) |
| **Exit Note** | Click in `Notes` | Returns to main menu |
| **Spin Roulette** | Rotate or Click in `Spin!` | Triggers spin animation, reveals compliment |
| **Ask Decider** | Click in `Decide` | Plays thinking animation (`?`, `!`, `*`) and reveals answer |
| **Breathing** | Rotate in `Breathe` | Sets cycles (1 to 20), Click starts 4-2-6 breathing |
| **Check Mood** | Rotate in `Mood` | Pick Happy, Okay, Sad, Angry, or Love face; Click for response |
| **Auto-Wake** | Rotate in `Wake` | Pick interval: `Off`, `30m`, `1 hr`, `2 hr`, `4 hr`; Click to save |
| **Focus Timers** | Standard Igor flow | UP (stopwatch), DOWN (countdown), POMO (Pomodoro) |
| **Wake from Sleep** | Rotate or Click | Instantly wakes screen back to menu |

---

## 💡 Gift Presentation Ideas

- **Cute Box & Note**: Place Iglet inside a small gift box with a handwritten card: *"Turn the knob to spin, breathe, or hear how much I love you!"*
- **Cute Colored Cable**: Pair with a pastel braided USB-C cable (pink, lavender, or cream).
- **Custom 3D Print**: Print the front shell in her favorite color and the back cover in white/ivory for a dual-tone aesthetic!
