# ClassroomComputer Project

## Overview
Arduino-based educational computer with LCD display and interactive programs.
Built for classroom demonstrations of algorithms and utilities.

## Hardware
- **Arduino Uno R4 Minima**
- **Grove RGB LCD** (I2C) - 16x2 character display
- **Linear potentiometer** - analog input for user interaction
- **Small speaker** (2-lead) - audio feedback

## Architecture
- **Main sketch**: `ClassroomComputer/ClassroomComputer.ino`
- **Program modules**: Individual `.h` files (e.g., `sort_program.h`, `primes_program.h`, `calculator_program.h`)
- **State machine pattern**: Each program implements isolated state machine
- **Pot debouncing**: Use `potLastMovedAt` pattern with 500ms delay before lock-in

## Current Programs
1. **Sort Test** - Bubble sort visualization with timing
2. **Primes** - Prime number finder (1-1000 range)
3. **Calculator** - Four-operation calculator with float support

## Development Guidelines

### Code Style
- Follow existing state machine patterns in program files
- Keep all display output within 16x2 LCD constraints
- Use descriptive state enum names (e.g., `CALC_SELECT_A`, `PRIMES_COMPUTING`)

### Display Management
- Clear lines only when necessary (avoid flicker)
- Pad strings to 16 chars to overwrite previous content
- Use `lcd.setCursor(col, row)` before each `lcd.print()`

### Pot Input Handling
- Read pot value and detect movement (threshold-based)
- Wait for `potLastMovedAt + 500ms` before accepting selection
- Remap pot range as needed per program (e.g., 0-1023 → 1-1000)

### Git Workflow
- Branch naming: `claude/description-XXXXX`
- Commit messages include session URL
- Push to feature branch, never directly to main

## Memory Constraints
Arduino Uno R4 Minima has limited RAM. Avoid:
- Large string concatenations
- Deep recursion
- Excessive global arrays

## File Structure
```
ClassroomComputer/
├── CLAUDE.md                    # This file
├── ClassroomComputer/
│   ├── ClassroomComputer.ino    # Main sketch
│   ├── sort_program.h           # Sort program
│   ├── primes_program.h         # Primes program
│   ├── calculator_program.h     # Calculator program
│   └── rgb_lcd.h                # LCD library
```

## Adding New Programs

1. Create `new_program.h` with state machine
2. Add include to main `.ino` file
3. Add enum value to `AppState`
4. Update `enterAppState()` to initialize your program
5. Add case to main loop dispatcher
6. Update program selection display and pot logic
