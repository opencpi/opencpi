typedef unsigned int uint32_t;

typedef struct {
  uint32_t length;
  uint32_t operation_or_exception_ordinal;
  uint32_t maxLength;
} OCLBufferInfo;

#define START (0)

typedef struct {
  __global void *data;
  uint32_t maxLength;
} OCLBuffer;

typedef struct {
  OCLBuffer current;
  struct {
    uint32_t length;
    union {
      uint32_t operation;
      uint32_t exception;
    } u;
  } input;
  struct {
    uint32_t length;
    union {
      uint32_t operation;
      uint32_t exception;
    } u;
  } output;
} OCLPort;

