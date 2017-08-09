#include <mutex>
#include <cstdint>
#include <vector>
#include <queue>

#include "TConfig.h"
#include "TReadoutBoard.h" // TBoardType is a typedef...

class TAlpide;

struct SingleEvent {
  SingleEvent(unsigned int length, uint64_t trigger_id, uint64_t timestamp, uint64_t timestamp_reference)
    : m_buffer(0), m_length(length), m_trigger_id(trigger_id),
      m_timestamp(timestamp), m_timestamp_reference(timestamp_reference) {
    m_buffer = new unsigned char[length];
  }
  ~SingleEvent() {
    delete[] m_buffer;
    m_buffer = 0;
  }
  unsigned char* m_buffer;
  unsigned int m_length;
  uint64_t m_trigger_id;
  uint64_t m_timestamp;
  uint64_t m_timestamp_reference;
};

class SimpleLock {
public:
  SimpleLock(std::mutex &mutex) : m_mutex(mutex) { mutex.lock(); }
  ~SimpleLock() { m_mutex.unlock(); }

protected:
  std::mutex &m_mutex;
};

class ALPIDESetup {
public:
  ALPIDESetup(int id, int debuglevel, TConfig* config,
        std::vector<TReadoutBoard *> * boards, TBoardType* boardType,
        std::vector<TAlpide *> * chips,
        std::vector<unsigned char>* raw_data=0x0);
  ALPIDESetup(int id, int debuglevel, std::string configFile,
        std::vector<unsigned char>* raw_data=0x0);
  ~ALPIDESetup();

  void StartDAQ();
  void StopDAQ();
  void PrintStatus() {}
  std::string GetConfigurationDump(std::string full_config_template) {}
  std::string GetSoftwareVersion() { return m_config->GetSoftwareVersion(); }
  std::string GetFirmwareVersion() { return "NOT IMPLEMENTED"; }
  int ReadEvents();
  void DeleteNextEvent();
  SingleEvent* PopNextEvent();
  void PrintQueueStatus();
  int GetQueueLength() {
    return m_queue.size();
  }
  bool CheckBusy();

protected:
  void Print(int level, const char* text, uint64_t value1 = -1,
             uint64_t value2 = -1, uint64_t value3 = -1, uint64_t value4 = -1);

  void Push(SingleEvent* ev);
  int  Configure(std::string configFile);


  // configuration
  int m_id;
  int m_debuglevel;
  TConfig* m_config;
  std::vector<TReadoutBoard *> * m_boards;
  TBoardType* m_boardType;
  std::vector<TAlpide *> * m_chips;

  std::queue<SingleEvent* > m_queue;
  std::vector<unsigned char>* m_raw_data;

  uint64_t m_last_trigger_id;
  uint64_t m_timestamp_reference;
  uint64_t m_timestamp_full;
};
