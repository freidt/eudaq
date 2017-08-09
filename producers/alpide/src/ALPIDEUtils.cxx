#include "ALPIDEUtils.hh"

// EUDAQ
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"


// new-alpide-software
#include "TConfig.h"
#include "SetupHelpers.h"


ALPIDESetup::ALPIDESetup(int id, int debuglevel, TConfig* config,
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
  , m_raw_data(raw_data)
  , m_last_trigger_id((uint64_t)-1)
  , m_timestamp_reference((uint64_t)-1)
  , m_timestamp_full((uint64_t)-1)
{
  //m_last_trigger_id = m_daq_board->GetNextEventId();
  //Print(0, "Starting with last event id: %lu", m_last_trigger_id);
}

ALPIDESetup::ALPIDESetup(int id, int debuglevel, std::string configFile,
                         std::vector<unsigned char>* raw_data/*=0x0*/)
  : m_id(id)
  , m_debuglevel(debuglevel)
  , m_config(0x0)
  , m_boards(0x0)
  , m_boardType(0x0)
  , m_chips(0x0)
  , m_queue()
  , m_raw_data(raw_data)
  , m_last_trigger_id((uint64_t)-1)
  , m_timestamp_reference((uint64_t)-1)
  , m_timestamp_full((uint64_t)-1)
{
  if (Configure(configFile) != 0) throw "Failed to configure the Setup";
}



ALPIDESetup::~ALPIDESetup() {
  eudaq::mSleep(5000);

  TReadoutBoardDAQ* daq_board = dynamic_cast<TReadoutBoardDAQ*>(m_boards->at(0));
  switch((unsigned long)m_boardType) {
  case boardDAQ:
    daq_board->StartTrigger();
    eudaq::mSleep(100);
    daq_board->WriteBusyOverrideReg(true);
    break;
  case boardMOSAIC:

    break;
  case boardRU:

    break;
  default:
    return;
  }
}

void ALPIDESetup::StartDAQ() {
  TReadoutBoardDAQ* daq_board = dynamic_cast<TReadoutBoardDAQ*>(m_boards->at(0));
  switch((unsigned long)m_boardType) {
  case boardDAQ:
    daq_board->StartTrigger();
    eudaq::mSleep(100);
    daq_board->WriteBusyOverrideReg(true);
    break;
  case boardMOSAIC:

    break;
  case boardRU:

    break;
  default:
    return;
  }
}

void ALPIDESetup::StopDAQ() {
  TReadoutBoardDAQ* daq_board = dynamic_cast<TReadoutBoardDAQ*>(m_boards->at(0));
  switch((unsigned long)m_boardType) {
  case boardDAQ:
    daq_board->WriteBusyOverrideReg(false);
    eudaq::mSleep(100);
    daq_board->StopTrigger();
    break;
  case boardMOSAIC:

    break;
  case boardRU:

    break;
  default:
    return;
  }
}

void ALPIDESetup::DeleteNextEvent() {
  //SimpleLock lock(m_mutex);
  if (m_queue.size() == 0)
    return;
  SingleEvent* ev = m_queue.front();
  m_queue.pop();
  delete ev;
}

SingleEvent* ALPIDESetup::PopNextEvent() {
  //SimpleLock lock(m_mutex);
  if (m_queue.size() > 0) {
    SingleEvent* ev = m_queue.front();
    m_queue.pop();
    if (m_debuglevel > 3)
      Print(0, "Returning event. Current queue status: %lu elements", m_queue.size());
    return ev;
  }
  if (m_debuglevel > 3)
    Print(0, "PopNextEvent: No event available");
  return 0;
}

void ALPIDESetup::PrintQueueStatus() {
  //SimpleLock lock(m_mutex);
  Print(0, "Queue status: %lu elements.", m_queue.size());
}

bool ALPIDESetup::CheckBusy() {
  TReadoutBoardDAQ* daq_board = dynamic_cast<TReadoutBoardDAQ*>(m_boards->at(0));
  uint32_t tmp_value = 0;
  switch((unsigned long)m_boardType) {
  case boardDAQ:
    daq_board->ReadRegister((uint16_t)0x207, tmp_value);
    tmp_value &= 0xFFC000; // keep only bits not transmitted in the header (47:38)
    m_timestamp_full = (uint64_t)tmp_value << 24;
    daq_board->ReadRegister((uint16_t)0x307, tmp_value);
    if ((tmp_value >> 26) & 0x1) return true;
    break;
  case boardMOSAIC:

    break;
  case boardRU:

    break;
  default:
    return true;
  }
  return true;
}


void ALPIDESetup::Print(int level, const char* text, uint64_t value1,
                         uint64_t value2, uint64_t value3, uint64_t value4) {
  // level:
  //   0 -> printout
  //   1 -> INFO
  //   2 -> WARN
  //   3 -> ERROR

  std::string tmp("ALPIDESetup %d: ");
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

void ALPIDESetup::Push(SingleEvent* ev) {
  //SimpleLock lock(m_mutex);
  m_queue.push(ev);
  if (m_debuglevel > 2)
    Print(0, "Pushed events. Current queue size: %lu", m_queue.size());
}

int ALPIDESetup::Configure(std::string configFile) {
  return initSetup(m_config, m_boards, m_boardType, m_chips, configFile.data());
}


int ALPIDESetup::ReadEvents() {
  TReadoutBoardDAQ* daq_board = dynamic_cast<TReadoutBoardDAQ*>(m_boards->at(0));
  uint32_t tmp_value = 0;
  switch((unsigned long)m_boardType) {
  case boardDAQ:
    daq_board->ReadRegister((uint16_t)0x207, tmp_value);
    tmp_value &= 0xFFC000; // keep only bits not transmitted in the header (47:38)
    m_timestamp_full = (uint64_t)tmp_value << 24;
    break;
  case boardMOSAIC:

    break;
  case boardRU:

    break;
  default:
    return -1;
  }
  return -1;
}
