#ifndef _ADMXRC3DRIVER_H
#define _ADMXRC3DRIVER_H

extern "C" {
#include <asm/byteorder.h>
#include <admxrc3.h>
#include <admxrc3ut.h>
#include <unistd.h>
}

class Admxrc3Driver
{
  public:
    ADMXRC3_HANDLE hDevice;
    const uint32_t hlsBaseAddr = 0x0;
    const uint32_t hlsChannel = 0;
    const unsigned int inBaseAddr = 0x0;
    const unsigned int outBaseAddr = 0x0;
    const unsigned int inChannel = 1;
    const unsigned int outChannel = 2;
    const unsigned int w1BaseAddr = 0x0;
    const unsigned int w2BaseAddr = 0x0;
    const unsigned int w1Channel = 1;
    const unsigned int w2Channel = 2;

    Admxrc3Driver() {
      ADMXRC3UT_OpenByIndex(0, false, true, 0, &hDevice);
    }

    ~Admxrc3Driver() {
      ADMXRC3_Close(hDevice);
      hDevice = ADMXRC3_HANDLE_INVALID_VALUE;
    }

    void attach(const char * name) {}
    void detach() {}

    void accelWrite(void* buffer, size_t size, uint64_t address, uint32_t channel) {
      ADMXRC3_BUFFER_HANDLE hBuffer;
      ADMXRC3_Lock(hDevice, buffer, size, &hBuffer);
      ADMXRC3_WriteDMALockedEx(hDevice, channel, 0, hBuffer, 0, size, address);
      ADMXRC3_Unlock(hDevice, hBuffer);
    }

    void accelRead(void* buffer, size_t size, uint64_t address, uint32_t channel) {
      ADMXRC3_BUFFER_HANDLE hBuffer;
      ADMXRC3_Lock(hDevice, buffer, size, &hBuffer);
      ADMXRC3_ReadDMALockedEx(hDevice, channel, 0, hBuffer, 0, size, address);
      ADMXRC3_Unlock(hDevice, hBuffer);
    }

    void writeJamRegAddr(uint32_t address, uint32_t control) {
      accelWrite(&control, sizeof(uint32_t), hlsBaseAddr + address, hlsChannel);
    }

    uint32_t readJamRegAddr(uint32_t address) {
      uint32_t control;
      accelRead(&control, sizeof(uint32_t), hlsBaseAddr + address, hlsChannel);
      return control;
    }
};

#endif
