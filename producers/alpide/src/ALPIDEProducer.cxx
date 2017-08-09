#include "ALPIDEProducer.hh"
#include "ALPIDEUtils.hh"

#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include <iostream>
#include <ostream>
#include <cctype>
#include <cstdlib>
#include <list>
#include <climits>
#include <cmath>
#include <algorithm>

#include "config.h"

using eudaq::StringEvent;
using eudaq::RawDataEvent;

static const std::string EVENT_TYPE = "ALPIDE";

void ALPIDEProducer::OnInitialise(const eudaq::Configuration &init) {
  try {
    std::cout << "Reading: " << init.Name() << std::endl;

    //m_exampleInitParam = init.Get("InitParameter", 0);
    //std::cout << "Initialise with parameter = " << m_exampleInitParam << std::endl;
    //...

    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
  }
  catch(...) {
    std::cout << "Initialisation Error - Unknown Exception" << std::endl;
    EUDAQ_ERROR("Initialisation Error - Unknown Exception");
    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
  }
}

void ALPIDEProducer::OnConfigure(const eudaq::Configuration &param) {
  try {
    std::cout << "Configuring (" << param.Name() << ") ..." << std::endl;
    EUDAQ_INFO("Configuring (" + param.Name() + ")");


    if (&param!=&m_param) m_param = param; // store configuration (needed for reconfiguration)

    m_status_interval = param.Get("StatusInterval", -1);
    m_full_config     = param.Get("FullConfig", "");

    const int nSetups = param.Get("Setups", 1); // number of setups
    if (m_nSetups != 0 && m_nSetups != nSetups) {
      if (m_setups) {
        for (int iSetup = 0; iSetup < nSetups; ++iSetup) {
          delete m_setups[iSetup];
          m_setups[iSetup] = 0x0;
        }
        delete[] m_setups;
        m_setups = 0x0;
      }
      if (!m_next_event) {
        for (int iSetup = 0; iSetup < nSetups; ++iSetup) {
          delete m_next_event[iSetup];
          m_next_event[iSetup] = 0x0;
        }
        delete[] m_next_event;
        m_next_event = 0x0;
      }
    }
    m_nSetups = nSetups;

    if (!m_next_event) {
      m_next_event = new SingleEvent*[m_nSetups];
      for (int i = 0; i < m_nSetups; ++i) {
        m_next_event[i] = 0x0;
      }
    }
    if (!m_setups) {
      m_setups = new ALPIDESetup*[m_nSetups];
      for (int i = 0; i < m_nSetups; ++i) {
        m_setups[i] = 0x0;
      }
    }
    if (!m_raw_data) {
      m_raw_data = new std::vector<unsigned char>*[m_nSetups];
      for (int i = 0; i < m_nSetups; ++i) {
#ifdef DEBUG_USB
        m_raw_data[i] = new std::vector<unsigned char>();
#else
        m_raw_data[i] = 0x0;
#endif
      }
    }

    // Control external devices
    SetBackBiasVoltage(param);
    ControlLinearStage(param);
    ControlRotaryStages(param);
    ConfigurePulser(param);

    const size_t buffer_length = 1000;
    char buffer[buffer_length];

    for (int i = 0; i < m_nSetups; ++i) {
      if (m_setups[i]) {
        delete m_setups[i];
        m_setups[i] = 0x0;
      }
      // configuration
      snprintf(buffer, buffer_length, "Config_File_%d", i);
      std::string configFile = param.Get(buffer, "");

      m_setups[i] = new ALPIDESetup(i, m_debuglevel, configFile, m_raw_data[i]);

      std::cout << "Setup " << i << " configured." << std::endl;

      eudaq::mSleep(10);
    }

    if (m_debuglevel > 3) {
      for (int i = 0; i < m_nSetups; ++i) {
        std::cout << "Setup " << i << ":" << std::endl;
        m_setups[i]->PrintStatus();
      }
    }

    if (!m_configured) {
      m_configured = true;
      m_firstevent = true;
    }


    EUDAQ_INFO("Configured (" + param.Name() + ")");
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
  }
  catch(...) {
    std::cout << "Initialisation Error - Unknown Exception" << std::endl;
    EUDAQ_ERROR("Initialisation Error - Unknown Exception");
    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
  }
}

void ALPIDEProducer::SetBackBiasVoltage(const eudaq::Configuration &param) {
  m_back_bias_voltage = param.Get("BackBiasVoltage", -1.);
  if (m_back_bias_voltage >= 0.) {
    std::cout << "Setting back-bias voltage..." << std::endl;
    system("if [ -f ${SCRIPT_DIR}/meas-pid.txt ]; then kill -2 $(cat "
           "${SCRIPT_DIR}/meas-pid.txt); fi");
    const size_t buffer_size = 100;
    char buffer[buffer_size];
    snprintf(buffer, buffer_size, "${SCRIPT_DIR}/change_back_bias.py %f",
             m_back_bias_voltage);
    if (system(buffer) != 0) {
      const char* error_msg = "Failed to configure the back-bias voltage";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, error_msg);
      m_back_bias_voltage = -2.;
    } else {
      std::cout << "Back-bias voltage set!" << std::endl;
    }
  }
}

void ALPIDEProducer::ControlRotaryStages(const eudaq::Configuration &param) {
#ifndef SIMULATION
  // rotary stage
  // step size: 1 step = 4.091 urad

  m_dut_angle1 = param.Get("DUTangle1", -1.);
  std::cout << "angle 1: " << m_dut_angle1 << " deg" << std::endl;

  int nsteps1 = (int)(m_dut_angle1/180.*3.14159/(4.091/1e6));
  std::cout << "   nsteps1: " << nsteps1 << std::endl;

  m_dut_angle2 = param.Get("DUTangle2", -1.);
  std::cout << "angle 2: " << m_dut_angle2 << " deg" << std::endl;

  int nsteps2 = (int)(m_dut_angle2/180.*3.14159/(4.091/1e6));
  std::cout << "   nsteps2: " << nsteps2 << std::endl;

  if (m_dut_angle1 >= 0. && m_dut_angle2 >= 0.) {
    std::cout << "Rotating DUT..." << std::endl;
    bool move_failed = false;
    if (system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 0") == 0) { // init of stage 1 (only one stage sufficient)
      // rotate both stages to home position
      //system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 p1")
      //system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 2 1")

      // rotate stages
      const size_t buffer_size = 100;
      char buffer[buffer_size];
      // rotate stage 1
      std::cout << "   stage 1..." << std::endl;
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 3 %i", nsteps1);
      if (system(buffer) != 0)
        move_failed = true;
      // rotate stage 2
      std::cout << "   stage 2..." << std::endl;
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 2 3 %i", nsteps2);
      if (system(buffer) != 0)
        move_failed = true;

    } else
      move_failed = true;

    if (move_failed) {
      const char *error_msg = "Failed to rotate one of the stages";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, error_msg);
      m_dut_angle1 = -2.;
      m_dut_angle2 = -2.;
    } else {
      std::cout << "DUT in position!" << std::endl;
    }
  }
#endif
}

void ALPIDEProducer::ControlLinearStage(const eudaq::Configuration &param) {
  // linear stage
  m_dut_pos = param.Get("DUTposition", -1.);
  if (m_dut_pos >= 0.) {
    std::cout << "Moving DUT to position..." << std::endl;
    bool move_failed = false;
    if (system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 0") == 0) {
      const size_t buffer_size = 100;
      char buffer[buffer_size];
      snprintf(buffer, buffer_size,
               "${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 5 %f", m_dut_pos);
      if (system(buffer) != 0)
        move_failed = true;
    } else
      move_failed = true;
    if (move_failed) {
      const char* error_msg = "Failed to move the linear stage";
      std::cout << error_msg << std::endl;
      EUDAQ_ERROR(error_msg);
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, error_msg);
      m_dut_pos = -2.;
    } else {
      std::cout << "DUT in position!" << std::endl;
    }
  }
}

void ALPIDEProducer::ConfigurePulser(const eudaq::Configuration &param) {
  m_n_trig = param.Get("NTrig", -1);
  m_period = param.Get("Period", -1.);

  // Control pulser for continuous integration testing
  if (m_n_trig>0 && m_period>0) {
    char cmd[100];
    snprintf(cmd, 100, "${SCRIPT_DIR}/pulser.py 1 %e %d", m_period, m_n_trig);
    int attempts = 0;
    bool success = false;
    while (!success && attempts++<5) {
      success = (system(cmd)==0);
    }
    if (!success) {
      std::string msg = "Failed to configure the pulser!";
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
    }
  }
}

void ALPIDEProducer::OnStartRun(unsigned param) {
  m_run = param;
  m_ev = 0;
  m_good_ev = 0;
  m_oos_ev = 0;
  m_last_oos_ev = (unsigned)-1;

  const size_t str_len = 100;
  char tmp[str_len];

  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
  bore.SetTag("Setups", m_nSetups);
  bore.SetTag("DataVersion", 0); // complete DAQ board event

  // versions
  bore.SetTag("Driver_GITVersion", m_setups[0]->GetSoftwareVersion());
  bore.SetTag("EUDAQ_GITVersion", PACKAGE_VERSION);

  // read configuration, dump to XML string
  for (int i = 0; i < m_nSetups; ++i) {
    try {
      snprintf(tmp, str_len, "Config_%d", i);
      bore.SetTag(tmp, m_setups[i]->GetConfigurationDump(m_full_config));

    }
    catch(...) {
      std::string msg = "Failed to load config file: ";
      std::cerr << msg.data() << std::endl;
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
      return;
    }
    try {
      // firmware version
      snprintf(tmp, str_len, "FirmwareVersion_%d", i);
      bore.SetTag(tmp, m_setups[i]->GetFirmwareVersion());
    }
    catch(...) {
      std::string msg = "Could not determine firmware version";
      std::cerr << msg.data() << std::endl;
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
      return;
    }
  }

  // back-bias voltage
  bore.SetTag("BackBiasVoltage", m_back_bias_voltage);
  bore.SetTag("DUTposition", m_dut_pos);
  bore.SetTag("DUTangle1", m_dut_angle1);
  bore.SetTag("DUTangle2", m_dut_angle2);

  // pulser configuration
  bore.SetTag("PulserPeriod", m_period);
  bore.SetTag("PulserNTriggers", m_n_trig);

  //Configuration is done
  for (int i = 0; i < m_nSetups; ++i) {
    m_timestamp_last[i]      = 0;
  }

  SendEvent(bore);

  for (int i = m_nSetups-1; i >= 0; --i) {
    m_setups[i]->StartDAQ();
  }


  SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
  EUDAQ_INFO("Running");
}

void ALPIDEProducer::OnStopRun() {
  for (int i = m_nSetups-1; i >= 0; --i) { // stop the event polling loop

#ifdef DEBUG_USB
    char tmp[50];
    snprintf(tmp, 50, "debug_data_%d_%d.dat", m_run, i);
    std::ofstream outfile(tmp, std::ios::out | std::ios::binary);
    if (m_raw_data[i] && m_raw_data[i]->size()>0) {
      outfile.write(reinterpret_cast<const char*>(&m_raw_data[i]), m_raw_data[i]->size());
      m_raw_data[i]->clear();
    }
    outfile.close();
#endif

    m_setups[i]->StopDAQ();
  }

  try {
    SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
    char msg[100];
    snprintf(msg, 100, "Sending EOR, Run %d, Event %d", m_run, m_ev);
    std::cout << msg << std::endl;
    EUDAQ_INFO(msg);
  }
  catch (...) {
    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stopping Error");
  }

  // Control pulser for continuous integration testing
  if (m_n_trig>0 && m_period>0) {
    char cmd[100];
    snprintf(cmd, 100, "${SCRIPT_DIR}/pulser.py 0"); // turn the pulser off

    int attempts = 0;
    bool success = false;
    while (!success && attempts++<5) {
      success = (system(cmd)==0);
    }
    if (!success) {
      std::string msg = "Failed to configure the pulser!";
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
    }
  }

  if (m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
    SetConnectionState(eudaq::ConnectionState::STATE_CONF);

  if (m_setups) {
    for (int i = m_nSetups-1; i >= 0; --i) { // destroy the setups
      delete m_setups[i];
      m_setups[i] = 0x0;
    }
    delete[] m_setups;
  }
}

void ALPIDEProducer::OnTerminate() {
  std::cout << "Terminating" << std::endl;
  m_done = true;
}

void ALPIDEProducer::OnReset() {
  std::cout << "Reset" << std::endl;
  SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Reset");
}

void ALPIDEProducer::OnUnrecognised(const std::string &cmd,
                                    const std::string &param) {
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0)
    std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Error");
}

void ALPIDEProducer::Loop() {
  unsigned long count = 0;
  unsigned long reconfigure_count = 0;
  unsigned long busy_count = 0;
  unsigned long out_of_sync_count = 0;
  time_t last_status = time(0);
  bool reconfigure = false;
  do {
    eudaq::mSleep(20);

    if (reconfigure_count>5) {
      std::string str = "Reconfigured more than 5 times without taking any new events - halting!";
      EUDAQ_ERROR(str);
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, str);
      while (1) {}
    }

    if (!m_running) {
      count = 0;
      reconfigure_count = 0;
      reconfigure = false;
    }
    // build events
    while (m_running) {
      int events_built = BuildEvent();
      count += events_built;

      if (events_built > 0) {
        reconfigure_count = 0;
        out_of_sync_count = 0;
      }
      else if (events_built == -1) { // auto-of-sync, which won't be recovered
        reconfigure = true;
        break;
      }
      else if (events_built == 0 || events_built == -2) {
        if (events_built == -2) {
          ++out_of_sync_count;
        }
        if (m_status_interval > 0 &&
            time(0) - last_status > m_status_interval) {
          if (m_running) {
            SendStatusEvent();
            uint32_t tmp_value = 0;
            uint64_t timestamp = 0;
            int address = 0x207; // timestamp upper 24bit
            bool found_busy = false;
            for (int i = 0; i < m_nSetups; ++i) {
              // check busy status
              if (m_setups[i]->CheckBusy())
                found_busy = true;
              if (found_busy) { // busy
                std::cout << "DAQ board " << i << " is busy." << std::endl;
              }
            }
            if (found_busy) ++busy_count;
          }
          PrintQueueStatus();
          last_status = time(0);

          std::vector<int> queueLengths;
          for (int i = 0; i < m_nSetups; ++i) {
            queueLengths.push_back(m_setups[i]->GetQueueLength());
          }
          std::sort(queueLengths.begin(), queueLengths.end());
          int diff = queueLengths[queueLengths.size()-1] - queueLengths[0];
          if (diff > 1000) {
            std::cout << "Queue difference: " << diff << std::endl;
            std::string str = "Queues differ by more than 1000 events";
            EUDAQ_ERROR(str);
            for (int i = 0; i < m_nSetups; ++i) {
              std::cout << "Reader " << i << ":" << std::endl;
              m_setups[i]->PrintStatus();
              reconfigure = true;
            }
          }
        }
        break;
      }
      else {
        busy_count = 0;
      }
      if (count % 20000 == 0)
        std::cout << "Sending event " << count << std::endl;
    }
    if (busy_count>5) {
      std::string str = "DAQ boards stay busy";
      EUDAQ_ERROR(str);
      for (int i = 0; i < m_nSetups; ++i) {
        std::cout << "Reader " << i << ":" << std::endl;
        m_setups[i]->PrintStatus();
      }
      busy_count = 0;
      reconfigure = true;
    }
    if (out_of_sync_count>10) {
      std::string str = "Out-of-sync recovery fails";
      EUDAQ_ERROR(str);
      for (int i = 0; i < m_nSetups; ++i) {
        std::cout << "Reader " << i << ":" << std::endl;
        m_setups[i]->PrintStatus();
      }
      out_of_sync_count = 0;
      reconfigure = true;
    }

    if (reconfigure) {
      std::string msg = "Reconfiguring ...";
      std::cout << msg << std::endl;
      EUDAQ_WARN(msg);
      for (int i = 0; i < m_nSetups; ++i) { // stop the event polling loop
        m_setups[i]->StopDAQ();
      }
      {
        SimpleLock lock(m_mutex);
        m_running = false;
      }

      // re-run configuration
      OnConfigure(m_param);

      // Print queue status and clear them
      PrintQueueStatus();
      for (int i = 0; i < m_nSetups; ++i) {
        while (m_setups[i]->GetQueueLength() > 0) {
          m_setups[i]->DeleteNextEvent();
        }
      }
      {
        SimpleLock lock(m_mutex);
        m_running = true;
      }
      for (int i = 0; i < m_nSetups; ++i) {
        m_setups[i]->StartDAQ();
      }
      EUDAQ_INFO("Reconfiguration done.");
      ++reconfigure_count;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
      EUDAQ_INFO("Running");
      reconfigure = false;
    }
  } while (!m_done);
}

int ALPIDEProducer::BuildEvent() {
  // returns:
  // - Number of events built
  // - -1 for an out-of-sync which is not recovered
  // - -2 for an out-of-sync recovery attempt


  if (m_debuglevel > 3)
    std::cout << "BuildEvent..." << std::endl;

  if (m_debuglevel > 10)
    eudaq::mSleep(2000);

  // fill next event and check if all event fragments are there
  // NOTE a partial _last_ event is lost in this way (can be improved based on
  // flush)
  uint64_t trigger_id = ULONG_MAX;
  int64_t timestamp = LONG_MAX;
  int64_t timestamp_mask = 0xFFFFFFFFFFFF;//0x7FFFFFFF;

  // local copies
  std::vector<int> planes;
  std::vector<uint64_t> trigger_ids;
  std::vector<int64_t> timestamps;
  std::vector<int64_t> timestamps_last;

  for (int i = 0; i < m_nSetups; ++i) {
    if (m_next_event[i] == 0)
      m_next_event[i] = m_setups[i]->PopNextEvent();
    if (m_next_event[i] == 0)
      return 0;
  }

  for (int i = 0; i < m_nSetups; ++i) {
    int64_t timestamp_tmp =
      (int64_t)m_next_event[i]->m_timestamp-(int64_t)m_next_event[i]->m_timestamp_reference;
    timestamp_tmp &= timestamp_mask;

    //std::cout << i << '\t' << m_next_event[i]->m_timestamp << '\t' << m_timestamp_full[i] << '\t' << m_next_event[i]->m_timestamp_reference << '\t' << timestamp_tmp << std::endl;
    if (m_firstevent) // set last timestamp if first event
      m_timestamp_last[i] = (int64_t)timestamp_tmp;

    planes.push_back(i);
    trigger_ids.push_back(m_next_event[i]->m_trigger_id);
    timestamps.push_back(timestamp_tmp);
    timestamps_last.push_back(m_timestamp_last[i]);
  }


  // sort the plane data, plane with largest timestamp first
  for (int i = 0; i < m_nSetups-1; ++i) {
    for (int j = 0; j < m_nSetups-1; j++) {
      //if ((timestamps[j]-timestamps_last[j])&timestamp_mask<(timestamps[j+1]-timestamps_last[j+1])&timestamp_mask) {
      if (trigger_ids[j]<trigger_ids[j+1]) {
        std::iter_swap(planes.begin()+j,          planes.begin()+j+1);
        std::iter_swap(trigger_ids.begin()+j,     trigger_ids.begin()+j+1);
        std::iter_swap(timestamps.begin()+j,      timestamps.begin()+j+1);
        std::iter_swap(timestamps_last.begin()+j, timestamps_last.begin()+j+1);
      }
    }
  }

#if 0
  std::cout << "m_ev\ti\tplanes[i]\ttrigger_ids[i]\tts[i]\tts_last[i]\tm_ts_last[i]\tm_next_event[planes[i]]->m_ts\tm_next_event[planes[i]]->m_timestamp_reference" << std::endl;

  for (int i = 0; i < m_nSetups; ++i) {
    std::cout << m_ev << '\t' << i << '\t' << planes[i] << '\t' << trigger_ids[i] << '\t' << timestamps[i] << '\t' << timestamps_last[i] << '\t' << m_timestamp_last[i] << '\t'
              << std::hex << "0x" << m_next_event[planes[i]]->m_timestamp << "\t0x" << m_next_event[planes[i]]->m_timestamp_reference << std::dec << std::endl;
  }
#endif

  // use the largest timestamp and the correponding trigger id
  trigger_id = trigger_ids[0];
  timestamp  = timestamps[0];

  if (m_debuglevel > 2)
    std::cout << "Sending event with trigger id " << trigger_id << std::endl;

  // detect inconsistency in timestamp
  bool* bad_plane = new bool[m_nSetups];
  for (int i = 0; i < m_nSetups; ++i)
    bad_plane[i] = false;

  bool timestamp_error_zero = false;
  bool timestamp_error_ref  = false;
  bool timestamp_error_last = false;
  for (int i = 1; i < m_nSetups; ++i) {
    if ((timestamps[i] == 0 && timestamps[0]!=0) || (timestamps[i] == 0 && timestamps[0]!=0)) {
      std::cout << "Found plane with timestamp equal to zero, while others aren't zero!" << std::endl;
      timestamp_error_zero = true;
      if (timestamps[i]==0) bad_plane[planes[i]] = true;
      else                  bad_plane[planes[0]] = true;
      break;
    }
    double abs_diff_ref = (double)(std::labs(timestamps[i]-timestamps[0])&timestamp_mask);
    double rel_diff_ref = abs_diff_ref/(double)(timestamps[0]&timestamp_mask);
    if (rel_diff_ref > 0.0001 && abs_diff_ref> 10) {
      std::cout << "Relative difference to reference timestamp larger than 1.e-4 and 10 clock cycles: " << rel_diff_ref <<" / " << abs_diff_ref << " in planes " << planes[0] << " and " << planes[i] <<std::endl;
      timestamp_error_ref = true;
      bad_plane[planes[i]] = true;
    }
    double abs_diff_last = (double)(std::labs(timestamps[i]-timestamps_last[i]-(timestamps[0]-timestamps_last[0]))&timestamp_mask);
    double rel_diff_last = abs_diff_last/(double)((timestamps[0]-timestamps_last[0])&timestamp_mask);
    if (rel_diff_last > 0.0001 && abs_diff_last>10 ) {
      std::cout << "Relative difference to last timestamp larger than 1.e-4 and 10 clock cycles: " << rel_diff_last << " / " << abs_diff_last << " in planes " << planes[0] << " and " << planes[i] << std::endl;
      timestamp_error_last = true;
      bad_plane[planes[i]] = true;
    }
  }
  if (timestamp_error_zero || timestamp_error_ref || timestamp_error_last) { // timestamps suspicious
    if (m_last_oos_ev!=m_ev) {
      m_last_oos_ev=m_ev;
      ++m_oos_ev;
      m_good_ev = 0;

      char msg[200];
      sprintf(msg, "Event %d. Out of sync", m_ev);
      std::string str(msg);
      if (m_oos_ev>5) {
        EUDAQ_WARN(str);
      }
      else  {
        EUDAQ_INFO(str);
      }
    }
    for (int i = 0; i < m_nSetups; ++i) {
      int64_t diff = (int64_t)timestamps[i] -(int64_t)timestamps[0];
      diff&=timestamp_mask;

      std::cout << i << '\t' << m_ev << '\t' << trigger_ids[i] << '\t' << timestamps[i]
                << '\t' << m_timestamp_last[i] << '\t'
                << (((int64_t)timestamps[i]-(int64_t)m_timestamp_last[i])&timestamp_mask) << '\t'
                << m_next_event[i]->m_timestamp_reference << '\t'
                << diff << '\t' << (double)diff/(double)timestamps[0] << std::endl;
    }
  }
  else {
    ++m_good_ev;
    if (m_good_ev>5) {
      m_oos_ev = 0;
    }
  }

  bool timestamp_error = (timestamp_error_zero || timestamp_error_ref || timestamp_error_last);

  if (!timestamp_error) { // store timestamps
    for (int i = 0; i < m_nSetups; ++i) {
      m_timestamp_last[i] = m_next_event[i]->m_timestamp - m_next_event[i]->m_timestamp_reference;
    }
  }

  if (timestamp_error) {
    int result = 0;
    if (m_recover_outofsync) {
      for (int i = 0; i < m_nSetups; ++i) {
        if (bad_plane[i] && m_next_event[i]) {
          delete m_next_event[i];
          m_next_event[i] = 0x0;
        }
      }
      result = -2; // recovery attempt
    }
    else {
      result = -1; // no recovery
    }
    delete[] bad_plane;
    bad_plane = 0x0;
    return result;
  }
  delete[] bad_plane;
  bad_plane = 0x0;

  // send event with trigger id trigger_id
  // send all layers in one block
  // also adding reference timestamp
  unsigned long total_size = 0;
  for (int i = 0; i < m_nSetups; ++i) {
    total_size += 2 + sizeof(uint16_t); //  0 - 3
    total_size += 3 * sizeof(uint64_t); //  4 - 27 (28 bytes)
    total_size += m_next_event[i]->m_length; // X
  }

  char* buffer = new char[total_size];
  unsigned long pos = 0;
  for (int i = 0; i < m_nSetups; ++i) {
    buffer[pos++] = 0xff; // 0
    buffer[pos++] = i;    // 1

    SingleEvent* single_ev = m_next_event[i];

    // data length
    uint16_t length = sizeof(uint64_t) * 3 + single_ev->m_length; // 3x64bit: trigger_id, timestamp, timestamp_reference
    memcpy(buffer + pos, &length, sizeof(uint16_t)); // 2, 3
    pos += sizeof(uint16_t);

    // event id and timestamp per layer
    memcpy(buffer + pos, &(single_ev->m_trigger_id), sizeof(uint64_t)); //  4 - 11
    pos += sizeof(uint64_t);
    memcpy(buffer + pos, &(single_ev->m_timestamp), sizeof(uint64_t));  // 12 - 19
    pos += sizeof(uint64_t);
    memcpy(buffer + pos, &(single_ev->m_timestamp_reference), sizeof(uint64_t)); // 20 - 27
    pos += sizeof(uint64_t);

    memcpy(buffer + pos, single_ev->m_buffer, single_ev->m_length);
    pos += single_ev->m_length;
    //printf("Event %d, reference_timestamp : %lu ; current_timestamp : %llu \n" ,m_ev, m_timestamp_reference[i], timestamp); // just for debugging
  }


  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.AddBlock(0, buffer, total_size);
  SendEvent(ev);
  delete[] buffer;
  m_firstevent = false;

  // clean up
  for (int i = 0; i < m_nSetups; ++i) {
    if (m_next_event[i]) {
      delete m_next_event[i];
      m_next_event[i] = 0;
    }
  }

  return 1;
}

void ALPIDEProducer::SendStatusEvent() {
  std::cout << "Sending status event" << std::endl;
  // temperature readout
  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.SetTag("pALPIDEfs_Type", 1);
  for (int i = 0; i < m_nSetups; ++i) {

  }
  SendEvent(ev);
}

void ALPIDEProducer::PrintQueueStatus() {
  for (int i = 0; i < m_nSetups; ++i)
    m_setups[i]->PrintQueueStatus();
}

// -------------------------------------------------------------------------------------------------

int main(int /*argc*/, const char** argv) {
  eudaq::OptionParser op("EUDAQ Producer", "1.0", "The pALPIDEfs Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(
    op, "l", "log-level", "NONE", "level",
    "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "pALPIDEfs", "string",
                                  "The name of this Producer");
  eudaq::Option<int> debug_level(op, "d", "debug-level", 0, "int",
                                 "Debug level for the producer");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    ALPIDEProducer producer(name.Value(), rctrl.Value(),
                            debug_level.Value());
    producer.Loop();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
