int crc16(const char *msg, int size)
{
  unsigned short poly = 0x1021;
  unsigned short crc = 0xffff;
  unsigned short byte = 0;

  while (--size >= 0) {
    byte = ((unsigned short)*msg++) << 8;
    crc = crc ^ byte;

    int i;
    for(i = 0; i < 8; i++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ poly;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}
