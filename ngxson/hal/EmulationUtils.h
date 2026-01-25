#pragma once

#include <Arduino.h>
#include <vector>
#include <SerialMutex.h>

namespace EmulationUtils {

// Packet format (from device to host):
// $$CMD_(COMMAND):[(ARG0)][:(ARG1)][:(ARG2)]$$\n
// Host response with either a base64-encoded payload or int64, terminated by newline:
// (BASE64_ENCODED_PAYLOAD)\n
// 123\n

static const char* CMD_PING     = "PING";     // arg0: dummy -- return int64_t: 123456
static const char* CMD_DISPLAY  = "DISPLAY";  // arg0: base64-encoded buffer -- returns 0=success
static const char* CMD_FS_LIST  = "FS_LIST";  // arg0: path, arg1: max files -- returns list of filenames, one per line, terminated by empty line
static const char* CMD_FS_READ  = "FS_READ";  // arg0: path, arg1: offset, arg2: length (-1 means read all) -- returns base64-encoded file contents
static const char* CMD_FS_STAT  = "FS_STAT";  // arg0: path -- return file size int64_t; -1 means not found; -2 means is a directory
static const char* CMD_FS_WRITE = "FS_WRITE"; // arg0: path, arg1: base64-encoded data, arg2: offset, arg3: is inplace (0/1) -- return int64_t bytes written
static const char* CMD_FS_MKDIR = "FS_MKDIR"; // arg0: path -- return int64_t: 0=success
static const char* CMD_FS_RM    = "FS_RM";    // arg0: path -- return int64_t: 0=success
static const char* CMD_BUTTON   = "BUTTON";   // arg0: action ("read") -- return int64_t: HalInput state bitmask

std::string base64_encode(const char* buf, unsigned int bufLen);
std::vector<uint8_t> base64_decode(const char* encoded_string, unsigned int in_len);

// IMPORTANT: Must use lock to ensure only one EmulationUtils operation is active at a time
class Lock {
public:
  Lock();
  ~Lock();
};

static void sendCmd(const char* cmd, const char* arg0 = nullptr, const char* arg1 = nullptr, const char* arg2 = nullptr, const char* arg3 = nullptr) {
  // Drain any stale data in the RX buffer before sending command
  while (UnwrappedSerial.available()) {
    UnwrappedSerial.read();
  }
  if (cmd != CMD_BUTTON) {
    UnwrappedSerial.printf("[%lu] [EMU] Sending command: %s\n", millis(), cmd);
  }
  UnwrappedSerial.print("$$CMD_");
  UnwrappedSerial.print(cmd);
  if (arg0 != nullptr) {
    UnwrappedSerial.print(":");
    UnwrappedSerial.print(arg0);
  } else {
    UnwrappedSerial.print(":"); // ensure at least one colon
  }
  if (arg1 != nullptr) {
    UnwrappedSerial.print(":");
    UnwrappedSerial.print(arg1);
  }
  if (arg2 != nullptr) {
    UnwrappedSerial.print(":");
    UnwrappedSerial.print(arg2);
  }
  if (arg3 != nullptr) {
    UnwrappedSerial.print(":");
    UnwrappedSerial.print(arg3);
  }
  UnwrappedSerial.print("$$\n");
  UnwrappedSerial.flush();  // Ensure command is fully transmitted before waiting for response
}

// used by display command (to avoid alloc overhead)
void sendDisplayData(const char* buf, size_t bufLen);

#define DEFAULT_TIMEOUT_MS 10000

static String recvRespStr(uint32_t timeoutMs = DEFAULT_TIMEOUT_MS) {
  unsigned long startMillis = millis();
  String line;
  line.reserve(1024);
  while (millis() - startMillis < (unsigned long)timeoutMs) {
    if (UnwrappedSerial.available()) {
      char c = UnwrappedSerial.read();
      if (c == '\n') {
        return line;
      } else if (c != '\r') {  // Ignore carriage returns
        line += c;
      }
    } else {
      // Small yield to prevent tight spinning and allow USB CDC to process
      delay(1);
    }
  }
  UnwrappedSerial.printf("[%lu] [EMU] FATAL: Timeout waiting for response (received so far: %d bytes)\n", millis(), line.length());
  assert(false && "FATAL: Timeout waiting for response");
  // should never reach here
  return String();
}

static std::vector<uint8_t> recvRespBuf(uint32_t timeoutMs = DEFAULT_TIMEOUT_MS) {
  String respStr = recvRespStr(timeoutMs);
  return base64_decode(respStr.c_str(), respStr.length());
}

static int64_t recvRespInt64(uint32_t timeoutMs = DEFAULT_TIMEOUT_MS) {
  String respStr = recvRespStr(timeoutMs);
  return respStr.toInt();
}

} // namespace EmulationUtils
