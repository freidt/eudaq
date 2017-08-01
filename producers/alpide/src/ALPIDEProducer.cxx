#include "ALPIDEProducer.hh"

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

static const std::string EVENT_TYPE = "pALPIDEfsRAW";


Setup::Setup(int id, int debuglevel, TConfig* config,
             std::vector<TReadoutBoard *> * boards, TBoardType* boardType,
             std::vector<TAlpide *> * chips,
             std::vector<unsigned char>* raw_data/*=0x0*/)
  : m_id(id)
  , m_debuglevel(debuglevel)
  , m_config(config)
  , m_boards(boards)
  , m_boardType(boardType)
  , m_chips(chips)
  , m_queue()
  , m_queue_size(0)
  , m_raw_data(raw_data)
  , m_last_trigger_id((uint64_t)-1)
  , m_timestamp_reference((uint64_t)-1)
{
  //m_last_trigger_id = m_daq_board->GetNextEventId();
  Print(0, "Starting with last event id: %lu", m_last_trigger_id);
}

Setup::~Setup() {
  eudaq::mSleep(5000);
}

void Setup::StartDAQ() {
  //m_daq_board->StartTrigger();
  //m_daq_board->WriteBusyOverrideReg(true);
}

void Setup::StopDAQ() {
  //m_daq_board->WriteBusyOverrideReg(false);
  //eudaq::mSleep(100);
  //m_daq_board->StopTrigger();
}

void Setup::DeleteNextEvent() {
  //SimpleLock lock(m_mutex);
  if (m_queue.size() == 0)
    return;
  SingleEvent* ev = m_queue.front();
  m_queue.pop();
  m_queue_size -= ev->m_length;
  delete ev;
}

SingleEvent* Setup::PopNextEvent() {
  //SimpleLock lock(m_mutex);
  if (m_queue.size() > 0) {
    SingleEvent* ev = m_queue.front();
    m_queue.pop();
    m_queue_size -= ev->m_length;
    if (m_debuglevel > 3)
      Print(0, "Returning event. Current queue status: %lu elements. Total "
            "size: %lu",
            m_queue.size(), m_queue_size);
    return ev;
  }
  if (m_debuglevel > 3)
    Print(0, "PopNextEvent: No event available");
  return 0;
}

void Setup::PrintQueueStatus() {
  //SimpleLock lock(m_mutex);
  Print(0, "Queue status: %lu elements. Total size: %lu", m_queue.size(),
        m_queue_size);
}

void Setup::Loop() {
  Print(0, "Loop starting...");
  while (1) {

    if (m_debuglevel > 10)
      eudaq::mSleep(1000);

    // no event waiting

  }
  Print(0, "ThreadRunner stopping...");
}



void Setup::Print(int level, const char* text, uint64_t value1,
                         uint64_t value2, uint64_t value3, uint64_t value4) {
  // level:
  //   0 -> printout
  //   1 -> INFO
  //   2 -> WARN
  //   3 -> ERROR

  std::string tmp("Setup %d: ");
  tmp += text;

  const int maxLength = 100000;
  char msg[maxLength] = { 0 };
  snprintf(msg, maxLength, tmp.data(), m_id, value1, value2, value3, value4);

  std::cout << msg << std::endl;
  if (level == 1) {
    EUDAQ_INFO(msg);
    //     SetStatus(eudaq::Status::LVL_OK, msg);
  } else if (level == 2) {
    EUDAQ_WARN(msg);
    //     SetStatus(eudaq::Status::LVL_WARN, msg);
  } else if (level == 3) {
    EUDAQ_ERROR(msg);
    //    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
  }
}

void Setup::Push(SingleEvent* ev) {
  //SimpleLock lock(m_mutex);
  m_queue.push(ev);
  m_queue_size += ev->m_length;
  if (m_debuglevel > 2)
    Print(0, "Pushed events. Current queue size: %lu", m_queue.size());
}

// -------------------------------------------------------------------------------------------------
/*
bool ALPIDEProducer::ConfigChip(int id, TpAlpidefs* dut,
                                std::string configFile) {

  // TODO maybe add this functionality to TDAQBoard?
  char buffer[1000];
  sprintf(buffer, "Device %d: Reading chip configuration from file %s", id,
          configFile.data());
  EUDAQ_INFO(buffer);

  TiXmlDocument doc(configFile.c_str());
  if (!doc.LoadFile()) {
    std::string msg = "Unable to open file ";
    msg += configFile;
    std::cerr << msg.data() << std::endl;
    EUDAQ_ERROR(msg.data());
    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
    return false;
  }
  // TODO return code?
  ParseXML(dut, doc.FirstChild("root")->ToElement(), -1, -1, false);
  return true;
}
*/
void ALPIDEProducer::OnConfigure(const eudaq::Configuration &param) {
  {
    SimpleLock lock(m_mutex);
    m_configuring = true;
  }
  std::cout << "Configuring..." << std::endl;
  if (&param!=&m_param) m_param = param; // store configuration
/*
  long wait_cnt = 0;
  while (IsRunning() || IsStopping()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still running, waiting to configure";
      std::cout << msg << std::endl;
      EUDAQ_ERROR(msg.data());
    }
  }
  EUDAQ_INFO("Configuring (" + param.Name() + ")");
  //SetStatus(eudaq::Status::LVL_OK, "Configuring (" + param.Name() + ")");

  m_status_interval = param.Get("StatusInterval", -1);

  const int delay = param.Get("QueueFullDelay", 0);
  const unsigned long queue_size = param.Get("QueueSize", 0) * 1024 * 1024;

  if (param.Get("CheckTriggerIDs", 0) == 1)
    m_ignore_trigger_ids = false;

  if (param.Get("DisableRecoveryOutOfSync", 0) == 1)
    m_recover_outofsync = false;

  if (param.Get("MonitorPSU", 0) == 1)
    m_monitor_PSU = true;

  m_n_trig = param.Get("NTrig", -1);
  m_period = param.Get("Period", -1.);

  const int nDevices = param.Get("Devices", 1);
  if (m_nDevices == 0 || m_nDevices == nDevices)
    m_nDevices = nDevices;
  else {
    const char* error_msg =
      "Unsupported attempt to change the number of devices!";
    std::cout << error_msg << std::endl;
    EUDAQ_ERROR(error_msg);
    SetConnectionState(eudaq::ConnectionState::STATE_ERROR, error_msg);
    return;
  }

  m_full_config_v1 = param.Get("FullConfigV1", param.Get("FullConfig", ""));
  m_full_config_v2 = param.Get("FullConfigV2", "");
  m_full_config_v3 = param.Get("FullConfigV3", "");
  m_full_config_v4 = param.Get("FullConfigV4", "");

  if (!m_next_event) {
    m_next_event = new SingleEvent*[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_next_event[i] = 0x0;
    }
  }
  if (!m_chip_type)
    m_chip_type = new int[m_nDevices];
  if (!m_strobe_length)
    m_strobe_length = new int[m_nDevices];
  if (!m_strobeb_length)
    m_strobeb_length = new int[m_nDevices];
  if (!m_trigger_delay)
    m_trigger_delay = new int[m_nDevices];
  if (!m_readout_delay)
    m_readout_delay = new int[m_nDevices];
  if (!m_timestamp_last)
    m_timestamp_last = new uint64_t[m_nDevices];
  if (!m_timestamp_full)
    m_timestamp_full = new uint64_t[m_nDevices];
  if (!m_chip_readoutmode)
    m_chip_readoutmode = new int[m_nDevices];
  if (!m_setups) {
    m_setups = new Setup*[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      m_setups[i] = 0x0;
    }
  }
  if (!m_raw_data) {
    m_raw_data = new std::vector<unsigned char>*[m_nDevices];
    for (int i = 0; i < m_nDevices; i++) {
      #ifdef DEBUG_USB
      m_raw_data[i] = new std::vector<unsigned char>();
      #else
      m_raw_data[i] = 0x0;
      #endif
    }
  }

  // Set back-bias voltage
  SetBackBiasVoltage(param);
  // Power supply monitoring
  if (m_monitor_PSU) {
    system("${SCRIPT_DIR}/meas.sh ${SCRIPT_DIR} ${LOG_DIR}/$(date "
           "+%s)-meas-tab ${LOG_DIR}/$(date +%s)-meas-log "
           "${SCRIPT_DIR}/meas-pid.txt");
  }
  // Move the linear stage
  //ControlLinearStage(param);
  ControlRotaryStages(param);

  if (!InitialiseTestSetup(param))
    return;

  for (int i = 0; i < m_nDevices; i++) {
    TDAQBoard* daq_board = m_setups[i]->GetDAQBoard();
    daq_board->WriteBusyOverrideReg(false);
  }

  if (!DoSCurveScan(param))
    return;

  for (int i = 0; i < m_nDevices; i++) {
    TpAlpidefs* dut = m_setups[i]->GetDUT();
    TDAQBoard* daq_board = m_setups[i]->GetDAQBoard();
    const size_t buffer_size = 100;
    char buffer[buffer_size];

    daq_board->WriteBusyOverrideReg(false);

    // configuration
    sprintf(buffer, "Config_File_%d", i);
    std::string configFile = param.Get(buffer, "");
    if (configFile.length() > 0)
      if (!ConfigChip(i, dut, configFile))
        return;

    // noisy and broken pixels
    dut->SetMaskAllPixels(false); // unmask all pixels
    dut->ClearNoisyPixels();
    bool mask_pixels = false;
    sprintf(buffer, "Noisy_Pixel_File_%d", i);
    std::string noisyPixels = param.Get(buffer, "");
    if (noisyPixels.length() > 0) {
      sprintf(buffer, "Device %d: Reading noisy pixels from file %s", i,
              noisyPixels.data());
      EUDAQ_INFO(buffer);
      dut->ReadNoisyPixelFile(noisyPixels.data());
      mask_pixels = true;
    }
    sprintf(buffer, "Broken_Pixel_File_%d", i);
    std::string brokenPixels = param.Get(buffer, "");
    if (brokenPixels.length() > 0) {
      sprintf(buffer, "Device %d: Reading broken pixels from file %s", i,
              brokenPixels.data());
      std::cout << buffer << std::endl;
      EUDAQ_INFO(buffer);
      dut->ReadNoisyPixelFile(brokenPixels.data(), true);
      mask_pixels = true;
    }

    if (mask_pixels)
      dut->MaskNoisyPixels();

    // triggering configuration per layer
    sprintf(buffer, "StrobeLength_%d", i);
    m_strobe_length[i] = param.Get(buffer, param.Get("StrobeLength", 10));
    sprintf(buffer, "StrobeBLength_%d", i);
    m_strobeb_length[i] = param.Get(buffer, param.Get("StrobeBLength", 20));
    sprintf(buffer, "ReadoutDelay_%d", i);
    m_readout_delay[i] = param.Get(buffer, param.Get("ReadoutDelay", 10));
    sprintf(buffer, "TriggerDelay_%d", i);
    m_trigger_delay[i] = param.Get(buffer, param.Get("TriggerDelay", 75));
    sprintf(buffer, "ChipReadoutMode_%d", i);
    m_chip_readoutmode[i] = param.Get(buffer, param.Get("ChipReadoutMode", 2));

    // data taking configuration
    // PrepareEmptyReadout

    if (!(strcmp(dut->GetClassName(), "TpAlpidefs3")) || !(strcmp(dut->GetClassName(), "TpAlpidefs4")) ) {
      //std::cout << "This is " << dut->GetClassName() << std::endl;
      daq_board->ConfigureReadout(3, true, true);
      // buffer depth = 3, 'sampling on rising edge (changed for pALPIDE3)', packet-based mode
    }
    else daq_board->ConfigureReadout(1, false, true); //buffer depth = 1, sampling on rising edge, packet-based mode
    daq_board->ConfigureTrigger(0, m_strobe_length[i], 2, 0,
                                m_trigger_delay[i]);
    // PrepareChipReadout
    dut->PrepareReadout(m_strobeb_length[i], m_readout_delay[i],
                        (TAlpideMode)m_chip_readoutmode[i]);       // chip_readoutmode = 1 : triggered ; 2 : continuous mode;

    if (delay > 0)
      m_setups[i]->SetQueueFullDelay(delay);
    if (queue_size > 0)
      m_setups[i]->SetMaxQueueSize(queue_size);

    int address = 0x207;
    uint32_t tmp_value = 0;
    m_setups[i]->GetDAQBoard()->ReadRegister(address, &tmp_value);
    //tmp_value &= 0xFFFFFF; // narrow down to 24 bits
    tmp_value &= 0xFFC000; // keep only bits not transmitted in the header (47:38)
    m_timestamp_full[i] = (m_chip_type[i] >= 3) ? (uint64_t)tmp_value << 24 : 0x0;

    std::cout << "Device " << i << " configured." << std::endl;

    eudaq::mSleep(10);
  }


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


  if (m_debuglevel > 3) {
    for (int i = 0; i < m_nDevices; i++) {
      std::cout << "Reader " << i << ":" << std::endl;
      m_setups[i]->PrintDAQboardStatus();
    }
  }

  if (!m_configured) {
    m_configured = true;
    m_firstevent = true;
  }

*/

  EUDAQ_INFO("Configured (" + param.Name() + ")");
  SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
  {
    SimpleLock lock(m_mutex);
    m_configuring = false;
  }
}

bool ALPIDEProducer::InitialiseTestSetup(const eudaq::Configuration &param) {
  /*
  if (!m_configured) {
    m_nDevices = param.Get("Devices", 1);

    const int delay = param.Get("QueueFullDelay", 0);
    const unsigned long queue_size = param.Get("QueueSize", 0) * 1024 * 1024;

    for (int idev = 0; idev < m_nDevices; idev++) {
      sprintf(mybuffer, "BoardAddress_%d", idev);
      int board_address = param.Get(mybuffer, -1);

      sprintf(mybuffer, "ChipType_%d", idev);
      std::string ChipType = param.Get(mybuffer, "");

      config->GetBoardConfig(idev)->GeoAdd = board_address;

      if (!ChipType.compare("ALPIDE1")) {
        m_chip_type[idev] = 1;
        config->GetBoardConfig(idev)->BoardType = 1;
        config->GetChipConfig(idev)->ChipType = DUT_ALPIDE1;
      } else if (!ChipType.compare("ALPIDE2")) {
        m_chip_type[idev] = 2;
        config->GetBoardConfig(idev)->BoardType = 2;
        config->GetChipConfig(idev)->ChipType = DUT_ALPIDE2;
      } else if (!ChipType.compare("ALPIDE3")) {
        m_chip_type[idev] = 3;
        config->GetBoardConfig(idev)->BoardType = 2;
        config->GetBoardConfig(idev)->EnableDDR = false;
        config->GetChipConfig(idev)->ChipType = DUT_ALPIDE3;
      } else if (!ChipType.compare("ALPIDE4") || !ChipType.compare("ALPIDE")) {
        m_chip_type[idev] = 4;
        config->GetBoardConfig(idev)->BoardType = 2;
        config->GetBoardConfig(idev)->EnableDDR = false;
        config->GetChipConfig(idev)->ChipType = DUT_ALPIDE4;
      }
    }

    std::cout << "Searching for DAQ boards " << std::endl;
    m_testsetup->FindDAQBoards(config);
    std::cout << "Found " << m_testsetup->GetNDAQBoards() << " DAQ boards."
              << std::endl;

    if (m_testsetup->GetNDAQBoards() < m_nDevices) {
      char msg[100];
      sprintf(msg, "Not enough devices connected. Configuration requires %d "
              "devices, but only %d present.",
              m_nDevices, m_testsetup->GetNDAQBoards());
      std::cerr << msg << std::endl;
      EUDAQ_ERROR(msg);
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg);
      return false;
    }

    m_testsetup->AddDUTs(config);
    for (int i = 0; i < m_nDevices; i++) {
      TpAlpidefs* dut = 0;
      TDAQBoard* daq_board = 0;

      const size_t buffer_size = 100;
      char buffer[buffer_size];

      if (!m_configured) {
        sprintf(buffer, "BoardAddress_%d", i);
        int board_address = param.Get(buffer, i);

        // find board
        int board_no = -1;
        for (int j = 0; j < m_testsetup->GetNDAQBoards(); j++) {
          if (m_testsetup->GetDAQBoard(j)->GetBoardAddress() == board_address) {
            board_no = j;
            break;
          }
        }

        if (board_no == -1) {
          char msg[100];
          sprintf(msg, "Device with board address %d not found", board_address);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
	  SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg);
          return false;
        }
        std::cout << "Enabling device " << board_no << std::endl;
        dut = (TpAlpidefs*)m_testsetup->GetDUT(board_no);
        daq_board = m_testsetup->GetDAQBoard(board_no);
        int overflow = 0x0;
        if (!m_testsetup->PowerOnBoard(board_no, overflow)) {
          char msg[100];
          sprintf(msg,
                  "Powering device with board address %d failed, overflow=0x%x",
                  board_address, overflow);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
          SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg);
          return false;
        }

        if (!m_testsetup->InitialiseChip(board_no, overflow)) {
          char msg[100];
          sprintf(
            msg,
            "Initialising device with board address %d failed, overflow=0x%x",
            board_address, overflow);
          std::cerr << msg << std::endl;
          EUDAQ_ERROR(msg);
          SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg);
          return false;
        }

        std::cout << "Device " << i << " with board address " << board_address
                  << " (delay " << delay << " - queue size " << queue_size
                  << ") powered." << std::endl;

        m_setups[i] = new Setup(i, m_debuglevel, m_testsetup, board_no,
                                daq_board, dut, m_raw_data[i]);
        if (m_next_event[i])
          delete m_next_event[i];
        m_next_event[i] = 0;
      } else {
        std::cout
          << "Already initialized and powered. Doing only reconfiguration..."
          << std::endl;
      }
    }
  }
  */
  return true;
}

bool ALPIDEProducer::PowerOffTestSetup() {
  /*
  std::cout << "Powering off test setup" << std::endl;
  for (int i = 0; i < m_nDevices; i++) {
    if (m_setups[i]) {
      TDAQBoard* daq_board = m_setups[i]->GetDAQBoard();
      m_setups[i]->Stop();
      delete m_setups[i];
      m_setups[i] = 0x0;
      // power of the DAQboard
      std::vector<SFieldReg> ADCConfigReg0 =
        daq_board
        ->GetADCConfigReg0(); // Get Descriptor Register ADCConfigReg0
      daq_board->SetPowerOnSequence(1, 0, 0, 0);
      daq_board->SendADCControlReg(ADCConfigReg0, 1,
                                   0); // register 0, self shutdown = 1, off = 0
      daq_board->ResetBoardFPGA(10);
      daq_board->ResetBoardFX3(10);
    }
  }
  if (m_testsetup) {
    struct libusb_context* context = m_testsetup->GetContext();
    delete m_testsetup;
    m_testsetup = 0x0;
    libusb_exit(context);
    m_configured = false;
  }

  eudaq::mSleep(5000);
  system("${SCRIPT_DIR}/fx3/program.sh");
  return true;
  */
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
      //system("${SCRIPT_DIR}/zaber.py /dev/ttyZABER0 1 1")
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

void ALPIDEProducer::OnStartRun(unsigned param) {
  /*
  long wait_cnt = 0;

  while (IsStopping()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still configuring, waiting to run";
      std::cout << msg << std::endl;
      EUDAQ_ERROR(msg.data());
    }
  }

  if (!m_configured) {
    EUDAQ_ERROR("No configuration file loaded and pALPIDE chips not powered on. Failed to start the run!");
    return;
  }

  while (IsConfiguring()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 100 == 0) {
      std::string msg = "Still configuring, waiting to run";
      std::cout << msg << std::endl;
      EUDAQ_ERROR(msg.data());
    }
  }
  m_run = param;
  m_ev = 0;
  m_good_ev = 0;
  m_oos_ev = 0;
  m_last_oos_ev = (unsigned)-1;

  // the queues should be empty at this stage, if not flush them
  PrintQueueStatus();
  bool queues_empty = true;
  for (int i = 0; i < m_nDevices; i++) {
    while (m_setups[i]->GetQueueLength() > 0) {
      m_setups[i]->DeleteNextEvent();
      queues_empty = false;
    }
  }
  if (!queues_empty) {
    EUDAQ_INFO( "Queues not empty on SOR, queues were flushed.");
  }
  eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
  bore.SetTag("Devices", m_nDevices);
  bore.SetTag("DataVersion", 3); // complete DAQ board event

  // driver version
  bore.SetTag("Driver_GITVersion", m_testsetup->GetGITVersion());
  bore.SetTag("EUDAQ_GITVersion", PACKAGE_VERSION);

  std::vector<char> SCS_data_block;
  std::vector<char> SCS_points_block;

  // read configuration, dump to XML string
  for (int i = 0; i < m_nDevices; i++) {
    std::string configstr;

    if (m_chip_type[i] == 4) configstr = m_full_config_v4;
    else if (m_chip_type[i] == 3) configstr = m_full_config_v3;
    else if (m_chip_type[i] == 2) configstr = m_full_config_v2;
    else configstr = m_full_config_v1;

    TiXmlDocument doc(configstr.c_str());
    if (!doc.LoadFile()) {
      std::string msg = "Failed to load config file: ";

      if (m_chip_type[i] == 4) msg += m_full_config_v4;
      else if (m_chip_type[i] == 3) msg += m_full_config_v3;
      else msg += (m_chip_type[i] == 2) ? m_full_config_v2 : m_full_config_v1;
      std::cerr << msg.data() << std::endl;
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
      return;
    }
    // 	std::cout << "PARSEXML READTRUE" << std::endl;
    m_setups[i]->ParseXML(doc.FirstChild("root")->ToElement(), -1, -1, true);

    std::string configStr;
    configStr << doc;

    char tmp[100];
    sprintf(tmp, "Config_%d", i);
    bore.SetTag(tmp, configStr);

    // store masked pixels
    const std::vector<TPixHit> pixels = m_setups[i]->GetDUT()->GetNoisyPixels();

    std::string pixelStr;
    for (int j = 0; j < pixels.size(); j++) {
      char buffer[50];
      sprintf(buffer, "%d %d %d\n", pixels.at(j).region, pixels.at(j).doublecol,
              pixels.at(j).address);
      pixelStr += buffer;
    }

    sprintf(tmp, "MaskedPixels_%d", i);
    bore.SetTag(tmp, pixelStr);

    // firmware version
    sprintf(tmp, "FirmwareVersion_%d", i);
    bore.SetTag(tmp, m_setups[i]->GetDAQBoard()->GetFirmwareName());

    sprintf(tmp, "ChipType_%d", i);
    bore.SetTag(tmp, m_chip_type[i]);
    // readout / triggering settings
    sprintf(tmp, "StrobeLength_%d", i);
    bore.SetTag(tmp, m_strobe_length[i]);
    sprintf(tmp, "StrobeBLength_%d", i);
    bore.SetTag(tmp, m_strobeb_length[i]);
    sprintf(tmp, "ReadoutDelay_%d", i);
    bore.SetTag(tmp, m_readout_delay[i]);
    sprintf(tmp, "TriggerDelay_%d", i);
    bore.SetTag(tmp, m_trigger_delay[i]);

    // S-curve scan data
    SCS_data_block.clear();
    SCS_data_block.reserve(512 * 1024 * m_SCS_n_steps);
    SCS_points_block.clear();
    SCS_points_block.reserve(m_SCS_n_steps);
    sprintf(tmp, "SCS_%d", i);
    if (m_do_SCS[i]) {
      for (int j = 0; j < 512; ++j) {
        for (int k = 0; k < 1024; ++k) {
          for (int l = 0; l < m_SCS_n_steps; ++l) {
            SCS_data_block.push_back(m_SCS_data[i][j][k][l]);
          }
        }
      }
      for (int j = 0; j < m_SCS_n_steps; ++j) {
        SCS_points_block.push_back(m_SCS_points[i][j]);
      }
      bore.SetTag(tmp, (int)1);
    } else {
      bore.SetTag(tmp, (int)0);
    }
    bore.AddBlock(2 * i, SCS_data_block);
    bore.AddBlock(2 * i + 1, SCS_points_block);
  }

  // general S-curve scan configuration
  bore.SetTag("SCSchargeStart", m_SCS_charge_start);
  bore.SetTag("SCSchargeStop", m_SCS_charge_stop);
  bore.SetTag("SCSchargeStep", m_SCS_charge_step);
  bore.SetTag("SCSnEvents", m_SCS_n_events);
  bore.SetTag("SCSnMaskStages", m_SCS_n_mask_stages);

  // back-bias voltage
  bore.SetTag("BackBiasVoltage", m_back_bias_voltage);
  bore.SetTag("DUTposition", m_dut_pos);

  // pulser configuration
  bore.SetTag("PulserPeriod", m_period);
  bore.SetTag("PulserNTriggers", m_n_trig);

  //Configuration is done, Read DAC Values and send to log

  for (int i = 0; i < m_nDevices; i++) {
    m_timestamp_last[i]      = 0;
  }


  for (int i = 0; i < m_nDevices; i++) {
    TDAQBoard* daq_board = m_setups[i]->GetDAQBoard();
    std::cout << "Reading Device:" << i << " ADCs" <<  std::endl;
    daq_board->ReadAllADCs();
  }

  SendEvent(bore);

  {
    SimpleLock lock(m_mutex);
    m_running = true;
  }
  for (int i = m_nDevices-1; i >= 0; --i) {
    m_setups[i]->SetRunning(true);
    m_setups[i]->StartDAQ();
  }


  SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
  EUDAQ_INFO("Running");
}

void ALPIDEProducer::OnStopRun() {
  std::cout << "Stop Run" << std::endl;
  {
    SimpleLock lock(m_mutex);
    m_running = false;
    m_stopping = true;
    m_flushing = true;
  }
  for (int i = m_nDevices-1; i >= 0; --i) { // stop the event polling loop

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
    m_setups[i]->SetRunning(false);
  }
  std::cout << "Set Running to False completed" << std::endl;
  SendEOR();

  unsigned long count = 0;
  bool resetRequired = false;
  for (int i = 0; i < m_nDevices; i++) { // wait until all read transactions are done
    while ((m_setups[i]->IsReading() || m_setups[i]->IsFlushing()) && count < 1000) {
      if (count++ % 250 == 0) {
        std::cout << "Reader is still reading, waiting.. " << std::endl;
      }
      eudaq::mSleep(20);
    }
    if (count==1000) {
      std::string msg = "Failed to receive the EOT marker!";
      EUDAQ_ERROR(msg.data());
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, msg.data());
      m_setups[i]->PrintDAQboardStatus();
      resetRequired = true;
    }
  }
  {
    SimpleLock lock(m_mutex);
    m_flushing = false;
  }

  long wait_cnt = 0;
  eudaq::mSleep(100);

  while (IsStopping()) {
    eudaq::mSleep(10);
    ++wait_cnt;
    if (wait_cnt % 1000 == 0) {
      std::string msg = "Still stopping...";
      std::cout << msg << std::endl;
    }
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

  if (resetRequired) {
    for (int i = m_nDevices-1; i >= 0; --i) { // stop the event polling loop
      m_setups[i]->StopDAQ();
      m_setups[i]->SetRunning(false);
    }
    eudaq::mSleep(1000);
    PowerOffTestSetup();
  }

  SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Run Stopped");
}

void ALPIDEProducer::OnTerminate() {
  std::cout << "Terminate" << std::endl;
  PowerOffTestSetup();
  std::cout << "All boards powered off" << std::endl;
  {
    SimpleLock lock(m_mutex);
    m_done = true;
  }
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

    if (!IsRunning()) {
      count = 0;
      reconfigure_count = 0;
      reconfigure = false;
    }
    // build events
    while (IsRunning()) {
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
          if (IsRunning()) {
            SendStatusEvent();
            uint32_t tmp_value = 0;
            uint64_t timestamp = 0;
            int address = 0x207; // timestamp upper 24bit
            bool found_busy = false;
            for (int i = 0; i < m_nDevices; i++) {
              if (m_debuglevel > 3){
                std::cout << "Reader " << i << ":" << std::endl;
                m_setups[i]->PrintDAQboardStatus();
              }

              // read the timestamp
              address = 0x207; // upper 24bit
              m_setups[i]->GetDAQBoard()->ReadRegister(address, &tmp_value);
              timestamp = (tmp_value & 0xFFFFFF) << 24;
              tmp_value &= 0xFFC000; // keep only bits not transmitted in the header (47:38)
              m_timestamp_full[i] = (m_chip_type[i] >= 3) ? (uint64_t)tmp_value << 24 : 0x0;

              address = 0x206; // lower 24bit
              m_setups[i]->GetDAQBoard()->ReadRegister(address, &tmp_value);
              timestamp |= (tmp_value & 0xFFFFFF);


              // check busy status
              address = 0x307;
              m_setups[i]->GetDAQBoard()->ReadRegister(address, &tmp_value);
              if ((tmp_value >> 26) & 0x1) found_busy = true;
              if (found_busy) { // busy
                std::cout << "DAQ board " << i << " is busy." << std::endl;
              }

              // read the trigger counter
              address = 0x202; // event ID lower 24bit
              m_setups[i]->GetDAQBoard()->ReadRegister(address, &tmp_value);
              std::cout << "Event ID " << i << ": 0x" << std::hex << tmp_value
                        << "\tTimestamp: 0x" << timestamp << std::dec << std::endl;
            }
            if (found_busy) ++busy_count;
          }
          PrintQueueStatus();
          last_status = time(0);

          std::vector<int> queueLengths;
          for (int i = 0; i < m_nDevices; i++) {
            queueLengths.push_back(m_setups[i]->GetQueueLength());
          }
          std::sort(queueLengths.begin(), queueLengths.end());
          int diff = queueLengths[queueLengths.size()-1] - queueLengths[0];
          if (diff > 1000) {
            std::cout << "Queue difference: " << diff << std::endl;
            std::string str = "DAQ boards queues differ by more than 1000 events";
            EUDAQ_ERROR(str);
            //SetConnectionState(eudaq::ConnectionState::STATE_ERROR, str);
            for (int i = 0; i < m_nDevices; i++) {
              std::cout << "Reader " << i << ":" << std::endl;
              m_setups[i]->PrintDAQboardStatus();
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
      //SetConnectionState(eudaq::ConnectionState::STATE_ERROR, str);
      for (int i = 0; i < m_nDevices; i++) {
        std::cout << "Reader " << i << ":" << std::endl;
        m_setups[i]->PrintDAQboardStatus();
      }
      busy_count = 0;
      reconfigure = true;
    }
    if (out_of_sync_count>10) {
      std::string str = "Out-of-sync recovery fails";
      EUDAQ_ERROR(str);
      //SetConnectionState(eudaq::ConnectionState::STATE_ERROR, str);
      for (int i = 0; i < m_nDevices; i++) {
        std::cout << "Reader " << i << ":" << std::endl;
        m_setups[i]->PrintDAQboardStatus();
      }
      out_of_sync_count = 0;
      reconfigure = true;
    }

    if (reconfigure) {
      std::string msg = "Reconfiguring ...";
      std::cout << msg << std::endl;
      //SetStatus(eudaq::Status::LVL_WARN, msg.data());
      EUDAQ_WARN(msg);
      for (int i = 0; i < m_nDevices; i++) { // stop the event polling loop
        m_setups[i]->StopDAQ();
      }
      {
        SimpleLock lock(m_mutex);
        m_running = false;
      }
      // power down
      PowerOffTestSetup();

      // re-run configuration
      OnConfigure(m_param);

      // Print queue status and clear them
      PrintQueueStatus();
      for (int i = 0; i < m_nDevices; i++) {
        while (m_setups[i]->GetQueueLength() > 0) {
          m_setups[i]->DeleteNextEvent();
        }
      }
      {
        SimpleLock lock(m_mutex);
        m_running = true;
      }
      for (int i = 0; i < m_nDevices; i++) {
        m_setups[i]->SetRunning(true);
        m_setups[i]->StartDAQ();
      }
      EUDAQ_INFO("Reconfiguration done.");
      ++reconfigure_count;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
      EUDAQ_INFO("Running");
      reconfigure = false;
    }
  } while (!IsDone());
  */
}

int ALPIDEProducer::BuildEvent() {
  // returns:
  // - Number of events built
  // - -1 for an out-of-sync which is not recovered
  // - -2 for an out-of-sync recovery attempt

  /*
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

  for (int i = 0; i < m_nDevices; i++) {
    if (m_next_event[i] == 0)
      m_next_event[i] = m_setups[i]->PopNextEvent();
    if (m_next_event[i] == 0)
      return 0;
  }

  for (int i = 0; i < m_nDevices; i++) {
    int64_t timestamp_tmp = ((int64_t)m_next_event[i]->m_timestamp | (int64_t)m_timestamp_full[i])-(int64_t)m_next_event[i]->m_timestamp_reference;
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
  for (int i = 0; i < m_nDevices-1; i++) {
    for (int j = 0; j < m_nDevices-1; j++) {
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

  for (int i = 0; i < m_nDevices; i++) {
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
  bool* bad_plane = new bool[m_nDevices];
  for (int i = 0; i < m_nDevices; i++)
    bad_plane[i] = false;

  bool timestamp_error_zero = false;
  bool timestamp_error_ref  = false;
  bool timestamp_error_last = false;
  for (int i = 1; i < m_nDevices; i++) {
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
        //SetStatus(eudaq::Status::LVL_WARN, str);
        EUDAQ_WARN(str);
      }
      else  {
        EUDAQ_INFO(str);
      }
    }
    for (int i = 0; i < m_nDevices; i++) {
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
    for (int i = 0; i < m_nDevices; i++) {
      m_timestamp_last[i] = m_next_event[i]->m_timestamp - m_next_event[i]->m_timestamp_reference;
    }
  }

  if (timestamp_error) {
    int result = 0;
    if (m_recover_outofsync) {
      for (int i = 0; i < m_nDevices; i++) {
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
  for (int i = 0; i < m_nDevices; i++) {
    total_size += 2 + sizeof(uint16_t); //  0 - 3
    total_size += 3 * sizeof(uint64_t); //  4 - 27 (28 bytes)
    total_size += m_next_event[i]->m_length; // X
  }

  char* buffer = new char[total_size];
  unsigned long pos = 0;
  for (int i = 0; i < m_nDevices; i++) {
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
  if (IsRunning())
    SendEvent(ev);
  delete[] buffer;
  m_firstevent = false;

  // clean up
  for (int i = 0; i < m_nDevices; i++) {
    if (m_next_event[i]) {
      delete m_next_event[i];
      m_next_event[i] = 0;
    }
  }

  return 1;
  */
}

void ALPIDEProducer::SendEOR() {
  /*
  char msg[100];
  snprintf(msg, 100, "Sending EOR, Run %d, Event %d", m_run, m_ev);
  std::cout << msg << std::endl;
  EUDAQ_INFO(msg);
  SendEvent(RawDataEvent::EORE(EVENT_TYPE, m_run, m_ev++));
  {
    SimpleLock lock(m_mutex);
    m_stopping = false;
  }
  */
}

void ALPIDEProducer::SendStatusEvent() {
  /*
  std::cout << "Sending status event" << std::endl;
  // temperature readout
  RawDataEvent ev(EVENT_TYPE, m_run, m_ev++);
  ev.SetTag("pALPIDEfs_Type", 1);
  for (int i = 0; i < m_nDevices; i++) {
    float temp = m_setups[i]->GetTemperature();
    ev.AddBlock(i, &temp, sizeof(float));
  }
  if (IsRunning())
    SendEvent(ev);
  */
}

void ALPIDEProducer::PrintQueueStatus() {
  for (int i = 0; i < m_nDevices; i++)
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
