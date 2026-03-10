# Classroom Computer

An Arduino-based educational computer that demonstrates algorithms and provides interactive utilities. Features a 16x2 LCD display, potentiometer control, and audio feedback.

![Classroom Computer](link-to-photo-placeholder)

## What It Does

The Classroom Computer runs three interactive programs:

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
- Breadboard (optional, for prototyping)

### Enclosure
- Laser-cut acrylic or wood panels ([CAD files](link-placeholder))
- M3 screws and standoffs
- USB cable (Type-C for R4 Minima)

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
6. Assemble panels and secure with screws

## How to Use

### Starting Up
Power on the Arduino via USB or external power supply. The welcome screen appears for 3 seconds, then the program selection menu loads.

### Selecting a Program
Move the potentiometer slider:
- **Left third**: Sort Test
- **Middle third**: Prime Finder
- **Right third**: Calculator

Hold the slider in position for 0.5 seconds to lock in your selection.

### Program Controls

**Sort Test:**
1. Select array size (1-50) with slider
2. Watch the sort visualization
3. See timing results
4. Returns to menu automatically

**Prime Finder:**
1. Select a number (1-1000) with slider
2. Computer determines if it's prime
3. If composite, shows smallest factor
4. Returns to menu automatically

**Calculator:**
1. Select first number A (1-1000) with slider
2. Select second number B (1-1000) with slider
3. Select operation with slider (quartiles: +, -, ×, ÷)
4. See the result with celebration animation
5. Returns to menu automatically

## Customization

Want to add your own programs? See [CLAUDE.md](CLAUDE.md) for development guidelines and architecture details.

## Files

- **Firmware**: `ClassroomComputer/` directory
- **CAD Files**: [link-placeholder] (Fusion 360, STEP format)
- **Laser Cut Files**: [link-placeholder] (SVG, DXF format)

## License

This project is open source. [License details TBD]

## Credits

Built with Arduino and the Grove RGB LCD library.

## Questions?

[Contact info or issue tracker placeholder]
