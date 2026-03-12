# Classroom Computer

The Classroom Computer is very nearly the simplest computer that a person can build. For information processing, it's got a basic Arduino. For output, it's got a basic two-line LCD display. And for input, there's a simple knob. But due to the unreasonable power of digital logic, you can do a lot with a setup this limited. As I told the students for whom this computer was built, it still has over an order of magnitude more processing power than the Apollo 11 computer that landed men on the moon. 

In any event, I would encourage you to make a version of it yourself for two primary reasons. First, it gives one a palpable sense of what a computer is and does. Second, it provides a great platform, or proving ground, for you to code your own programs. If they can work effectively in this context, they will surely work in more featureful environments. 

![Classroom Computer](https://uc5df8b3b54bc1d7273e9425a4cd.previews.dropboxusercontent.com/p/thumb/AC8SyaFIEgAePWFLLuwEgHm_0AQT2P_FjcyzJuh_DlHKhGnGiyFk46KQP_qjZ0N4cTIN_lorPlz1fzkbFt_1EBnZWybzuRTsxDbI3p9Nh1AxWpLGEv0rXAQJN6jfbj-L1BCZGO7m5h77KPTDFT60sFd3v43G-RYDQe6VJ0KQzcJQn2sl-aDsAbnrPpQxopl5RQL_ARVUcpXmLTWmUjmCH4ltBBQAgx-q-unNPv4KJB0HwuLTVRHqFDWai-Wu5lCCzEMqfxJBX4Cg3cRU-uASxFJ8P8wNwyIUrfEHgmQ7rlituA2jNS3iJ5CjP9ZLtJQVxqKyDimv4yvOv62g6FI8VACVI9eUO0QgOO0P2K9x20bMGtOO1KLnZfjqP1aZxJVmLKunYntaTw706Y5Z0252xGKD/p.jpeg?is_prewarmed=true)

## What It Does

The Classroom Computer runs some number of interactive programs. Among the programs created so far are:

1. **Sort Test** - Visualizes bubble sort algorithm with timing display
2. **Prime Finder** - Finds prime numbers in the range 1-1000
3. **Calculator** - Four-operation calculator (+, -, ×, ÷) with decimal support

Navigate between programs using the potentiometer slider, then use the same slider to input values and make selections within each program.

## Bill of Materials

### Electronics
- Arduino Uno R4 Minima
- Grove RGB LCD (16x2, I2C)
- Linear potentiometer (10kΩ recommended)
- Small speaker (2-lead piezo or magnetic)
- Jumper wires

### Enclosure
- 1/8 in (3 mm) laser-cut acrylic or wood panels 
- 4x M3 screws and nuts
- USB cable (type-C for R4 Minima)
- USB Li-Ion power nank with 5V Outputs @ 2.1A - 5000mAh (from Adafruit)
- Select finger joints were bonded with weld-on acrylic

## Build Instructions

### 1. Hardware Assembly

**Connect the LCD (I2C):**
- LCD SDA → Arduino SDA
- LCD SCL → Arduino SCL
- LCD VCC → Arduino 5V
- LCD GND → Arduino GND

**Connect the Potentiometer:**
- Pot pin 1 → Arduino GND
- Pot pin 2 (wiper) → Arduino A0
- Pot pin 3 → Arduino 5V

**Connect the Speaker:**
- Speaker (+) → Arduino D8 (or through 100Ω resistor)
- Speaker (-) → Arduino GND

### 2. Software Setup

**Install Arduino IDE:**
1. Download Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Install and open the IDE

**Install RGB LCD Library:**
1. In Arduino IDE: Sketch → Include Library → Manage Libraries
2. Search for "Grove LCD RGB Backlight"
3. Install the library

**Upload the Code:**
1. Clone or download this repository
2. Open `ClassroomComputer/ClassroomComputer.ino` in Arduino IDE
3. Select board: Tools → Board → Arduino Uno R4 Minima
4. Select port: Tools → Port → (your Arduino's port)
5. Click Upload button (→)

### 3. Enclosure Assembly

1. Laser cut panels using provided files ([download](link-placeholder))
2. Mount Arduino to base panel with M3 standoffs
3. Mount LCD in front panel cutout
4. Mount potentiometer in side panel
5. Secure speaker inside enclosure
6. Assemble panels and secure with screws and weld-on (sparingly)

## How to Use

### Starting Up
Power on the Arduino via USB or external power supply. The welcome screen appears for ~3 seconds, then the program selection menu loads.

### Selecting a Program
Move the potentiometer slider to select program. Note that if write your own programs, you will have to manually specify which percent of the slider range maps to which program.

## Files

- **Firmware**: `ClassroomComputer/` directory
- **CAD**: [Onshape project](https://cad.onshape.com/documents/2c616913ed35852bc2abee61/w/cb143e35db52d72bd43e4b08/e/0127d00eb2454f0f71ede743?renderMode=0&uiState=69b31ef32c020ddc21038ab5)
- **Laser Cut Files**: [The Adobe Illustrator files in this folder](https://www.dropbox.com/scl/fo/buwry2dpekuhzvbac7raz/AEnpvI6yAWcVBEK0AMFHpDo?rlkey=oy1yj3bhf98fbxmac3mu35kar&st=710h4esm&dl=0)

## License

This project is licensed under the MIT License. Put simnply, you may use it and adapt it as you wish, but you must credit this project. 

