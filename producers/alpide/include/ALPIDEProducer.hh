#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include <mutex>
#include <thread>
#include <queue>

class ALPIDESetup;
class SingleEvent;

class ALPIDEProducer : public eudaq::Producer {
public:
  ALPIDEProducer(const std::string &name, const std::string &runcontrol,
                    int debuglevel = 0)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_good_ev(0),
      m_oos_ev(0), m_last_oos_ev(0), m_timestamp_last(0x0), m_timestamp_full(0x0),
      m_done(false), m_running(false), m_stopping(false), m_flushing(false),
      m_configured(false), m_firstevent(false), m_setups(0), m_next_event(0),
      m_debuglevel(debuglevel), m_raw_data(0x0), m_mutex(), m_param(), m_nSetups(0),
      m_status_interval(-1), m_full_config(), m_ignore_trigger_ids(true),
      m_recover_outofsync(true), m_chip_type(0x0),
      m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0),
      m_readout_delay(0x0), m_chip_readoutmode(0x0), m_back_bias_voltage(-1),
      m_dut_pos(-1.), m_dut_angle1(-1.), m_dut_angle2(-1.) {}
  ~ALPIDEProducer() { PowerOff(); }

  virtual void OnInitialise(const eudaq::Configuration &init);
  virtual void OnConfigure(const eudaq::Configuration &param);
  virtual void OnStartRun(unsigned param);
  virtual void OnStopRun();
  virtual void OnTerminate();
  virtual void OnReset();
  virtual void OnStatus() {}
  virtual void OnUnrecognised(const std::string &cmd, const std::string &param);

  void Loop();

protected:
  bool Initialise(const eudaq::Configuration &param);
  bool PowerOff();
  void SetBackBiasVoltage(const eudaq::Configuration &param);
  void ControlLinearStage(const eudaq::Configuration &param);
  void ControlRotaryStages(const eudaq::Configuration &param);
  void ConfigurePulser(const eudaq::Configuration &param);
  //bool ConfigChip(int id, TpAlpidefs *dut, std::string configFile);
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
  ALPIDESetup** m_setups;
  SingleEvent** m_next_event;
  int m_debuglevel;


  std::mutex m_mutex;

  std::vector<unsigned char>** m_raw_data;

  // config
  eudaq::Configuration m_param;
  int m_nSetups;
  int m_status_interval;
  std::string m_full_config;
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
