// Megaprocessor Definition File
// Hardware constants and memory map for the Megaprocessor
// Reference: http://www.megaprocessor.com/

// ===============================================
// MEMORY MAP
// ===============================================

// Internal RAM (Display Memory)
INT_RAM_START         EQU 0x8000;
INT_RAM_LEN           EQU 0x4B00;  // 19200 bytes (64x75 words)
INT_RAM_BYTES_ACROSS  EQU 4;       // 4 bytes per display line (2 words)

// External RAM
EXT_RAM_START         EQU 0x0000;
EXT_RAM_LEN           EQU 0x8000;

// ===============================================
// I/O ADDRESSES
// ===============================================

// General I/O Port
GEN_IO_INPUT          EQU 0xD000;  // Input switches and joystick
GEN_IO_OUTPUT         EQU 0xD001;  // LED outputs

// ===============================================
// I/O BIT MASKS
// ===============================================

// Joystick/Button Input Masks (GEN_IO_INPUT)
IO_SWITCH_FLAG_UP         EQU 0x0001;  // Joystick Up
IO_SWITCH_FLAG_DOWN       EQU 0x0002;  // Joystick Down
IO_SWITCH_FLAG_LEFT       EQU 0x0004;  // Joystick Left
IO_SWITCH_FLAG_RIGHT      EQU 0x0008;  // Joystick Right
IO_SWITCH_FLAG_R1         EQU 0x0010;  // Button R1 (Fire)
IO_SWITCH_FLAG_R2         EQU 0x0020;  // Button R2
IO_SWITCH_FLAG_L1         EQU 0x0040;  // Button L1
IO_SWITCH_FLAG_L2         EQU 0x0080;  // Button L2
IO_SWITCH_FLAG_SELECT     EQU 0x0100;  // Select button
IO_SWITCH_FLAG_START      EQU 0x0200;  // Start button
IO_SWITCH_FLAG_SQUARE     EQU 0x0400;  // Square button
IO_SWITCH_FLAG_TRIANGLE   EQU 0x0800;  // Triangle button
IO_SWITCH_FLAG_CIRCLE     EQU 0x1000;  // Circle button
IO_SWITCH_FLAG_CROSS      EQU 0x2000;  // Cross button

// ===============================================
// PROCESSOR STATUS FLAGS
// ===============================================
FLAG_CARRY            EQU 0x0001;
FLAG_ZERO             EQU 0x0002;
FLAG_NEGATIVE         EQU 0x0004;
FLAG_OVERFLOW         EQU 0x0008;
FLAG_INTERRUPT_ENABLE EQU 0x0010;

// ===============================================
// DISPLAY CONSTANTS
// ===============================================
DISPLAY_WIDTH         EQU 64;      // Display width in words
DISPLAY_HEIGHT        EQU 75;      // Display height in lines
DISPLAY_BYTES_PER_LINE EQU 4;      // 4 bytes per line (2 words)

// ===============================================
// COMMON COLORS (16-bit RGB565 format)
// ===============================================
COLOR_BLACK           EQU 0x0000;
COLOR_WHITE           EQU 0xFFFF;
COLOR_RED             EQU 0xF800;
COLOR_GREEN           EQU 0x07E0;
COLOR_BLUE            EQU 0x001F;
COLOR_YELLOW          EQU 0xFFE0;
COLOR_CYAN            EQU 0x07FF;
COLOR_MAGENTA         EQU 0xF81F;

// ===============================================
// STACK CONFIGURATION
// ===============================================
STACK_START           EQU 0x7FFF;  // Stack grows downward
STACK_SIZE            EQU 0x1000;  // 4KB stack

// ===============================================
// INTERRUPT VECTORS (at address 0x0000)
// ===============================================
VECTOR_RESET          EQU 0x0000;  // Reset vector
VECTOR_EXT_INT        EQU 0x0004;  // External interrupt
VECTOR_DIV_ZERO       EQU 0x0008;  // Division by zero
VECTOR_ILLEGAL        EQU 0x000C;  // Illegal instruction

// ===============================================
// USEFUL CONSTANTS
// ===============================================
TRUE                  EQU 1;
FALSE                 EQU 0;
NULL                  EQU 0;

// ===============================================
// CHARACTER CODES (ASCII subset)
// ===============================================
CHAR_NUL              EQU 0x00;   // Null
CHAR_LF               EQU 0x0A;   // Line Feed
CHAR_CR               EQU 0x0D;   // Carriage Return
CHAR_SPACE            EQU 0x20;   // Space
CHAR_ZERO             EQU 0x30;   // '0'
CHAR_A                EQU 0x41;   // 'A'
CHAR_Z                EQU 0x5A;   // 'Z'
CHAR_a                EQU 0x61;   // 'a'
CHAR_z                EQU 0x7A;   // 'z'

// ===============================================
// TIMING CONSTANTS (assuming ~1MHz clock)
// ===============================================
DELAY_1MS             EQU 1000;   // ~1ms delay loop count
DELAY_1S              EQU 1000000; // ~1s delay loop count

// ===============================================
// END OF DEFINITIONS
// ===============================================