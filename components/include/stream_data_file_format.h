
/*
  This file contains the common data structures for the signal data file readers/writes
*/

typedef struct {
  uint8_t     endian;  // 1 = little
  uint8_t     pad;
  uint16_t    opcode;
  uint32_t    length;  // data length in bytes
} FileHeader;

