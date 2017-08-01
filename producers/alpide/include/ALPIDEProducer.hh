#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include <mutex>
#include <thread>
#include <queue>

#include <tinyxml.h>


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

class Setup {
public:
  Setup(int id, int debuglevel, TConfig* config,
        std::vector<TReadoutBoard *> * boards, TBoardType* boardType,
        std::vector<TAlpide *> * chip,
        std::vector<unsigned char>* raw_data=0x0);
  ~Setup();

  void SetMaxQueueSize(unsigned long size) { m_max_queue_size = size; }
  void SetQueueFullDelay(int delay) { m_queuefull_delay = delay; }

  void Configure();
  void StartDAQ();
  void StopDAQ();
  void DeleteNextEvent();
  SingleEvent* PopNextEvent();
  void PrintQueueStatus();
  int GetQueueLength() {
    SimpleLock lock(m_mutex);
    return m_queue.size();
  }

protected:
  void Loop();
  void Print(int level, const char* text, uint64_t value1 = -1,
             uint64_t value2 = -1, uint64_t value3 = -1, uint64_t value4 = -1);

  void Push(SingleEvent* ev);

  std::queue<SingleEvent* > m_queue;
  unsigned long m_queue_size;
  std::vector<unsigned char>* m_raw_data;

  int m_id;
  int m_boardid; // id of the DAQ board as used by TTestSetup::GetDAQBoard()...
  int m_debuglevel;
  uint64_t m_last_trigger_id;
  uint64_t m_timestamp_reference;

  TConfig* fConfig;
  std::vector<TReadoutBoard *> * fBoards;
  std::vector<TAlpide *> * fChips;

  // config
  int m_queuefull_delay;          // milliseconds
  unsigned long m_max_queue_size; // queue size in B
};

class ALPIDEProducer : public eudaq::Producer {
public:
  ALPIDEProducer(const std::string &name, const std::string &runcontrol,
                    int debuglevel = 0)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_good_ev(0),
      m_oos_ev(0), m_last_oos_ev(0), m_timestamp_last(0x0), m_timestamp_full(0x0),
      m_done(false), m_running(false), m_stopping(false), m_flushing(false),
      m_configured(false), m_firstevent(false), m_setups(0), m_next_event(0),
      m_debuglevel(debuglevel), m_testsetup(0), m_raw_data(0x0), m_mutex(), m_param(), m_nDevices(0),
      m_status_interval(-1), m_full_config_v4(), m_ignore_trigger_ids(true),
      m_recover_outofsync(true), m_chip_type(0x0),
      m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0),
      m_readout_delay(0x0), m_chip_readoutmode(0x0),
      m_monitor_PSU(false), m_back_bias_voltage(-1),
      m_dut_pos(-1.), m_dut_angle1(-1.), m_dut_angle2(-1.) {}
  ~ALPIDEProducer() { PowerOffTestSetup(); }

  virtual void OnConfigure(const eudaq::Configuration &param);
  virtual void OnStartRun(unsigned param);
  virtual void OnStopRun();
  virtual void OnTerminate();
  virtual void OnReset();
  virtual void OnStatus() {}
  virtual void OnUnrecognised(const std::string &cmd, const std::string &param);

  void Loop();

protected:
  bool InitialiseTestSetup(const eudaq::Configuration &param);
  bool PowerOffTestSetup();
  void SetBackBiasVoltage(const eudaq::Configuration &param);
  void ControlLinearStage(const eudaq::Configuration &param);
  void ControlRotaryStages(const eudaq::Configuration &param);
  bool ConfigChip(int id, TpAlpidefs *dut, std::string configFile);
  int BuildEvent();
  void SendEOR();
  void SendStatusEvent();
  void PrintQueueStatus();

  unsigned m_run, m_ev, m_good_ev, m_oos_ev, m_last_oos_ev;
  uint64_t *m_timestamp_last;
  uint64_t *m_timestamp_full;
  bool m_done;
  bool m_running;
  bool m_stopping;
  bool m_flushing;
  bool m_configuring;
  bool m_configured;
  bool m_firstevent;
  Setup** m_setups;
  SingleEvent** m_next_event;
  int m_debuglevel;


  std::mutex m_mutex;

  std::vector<unsigned char>** m_raw_data;

  // config
  eudaq::Configuration m_param;
  int m_nDevices;
  int m_status_interval;
  std::string m_full_config_v4;
  bool m_ignore_trigger_ids;
  bool m_recover_outofsync;
  int* m_chip_type;
  int* m_strobe_length;
  int* m_strobeb_length;
  int* m_trigger_delay;
  int* m_readout_delay;
  int* m_chip_readoutmode;
  float m_back_bias_voltage;
  float m_dut_pos;
  float m_dut_angle1;
  float m_dut_angle2;

  int m_n_trig;
  float m_period;

};
