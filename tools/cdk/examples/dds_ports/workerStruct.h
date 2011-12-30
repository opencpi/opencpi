
const char* s_msg = "  Test message from consumer   ";
#define s_msg_size 31
#define long_seq_size 5
#define double_seq_size 12
#define short_seq_size 4
#define uchar_seq_size 8
#define char_seq_size 12


typedef struct {

  int32_t       userId;
  uint32_t      u1;
  int64_t       v_longlong1;
  int16_t       v_short;
  char pad0_[2];
  int32_t       v_l_array[3];
  int32_t       v_l_array1[3][4];
  RCCBoolean    v_bool;
  char pad1_[7];
  RCCDouble     v_double;
  uint8_t       v_oct_array[48];
  int32_t       v_long2;
  char pad2_[4];


} WorkerTestMsg;


// We do the alignment this way for debug
uint8_t * align( int n, uint8_t * p )
{
  uint8_t * tmp  = (uint8_t *)(((uintptr_t)p + (n - 1)) & ~((uintptr_t)(n)-1));
  return tmp;
}

