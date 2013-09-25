#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include "eudaq/Mutex.hh"
#include "DeviceExplorer.hh"

#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>

#define BUFFER 9000
using namespace std;

template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
  unsigned char * src = static_cast < unsigned char* >(static_cast < void * >(&data));
  dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (std::vector <unsigned char >& src, int index, T& data) {
  copy (&src[index], &src[index + sizeof (T)], &data);
}

typedef pair<string,string> config;

// A name to identify the raw data format of the events generate.

// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "EXPLORER1Raw";

// Declare a new class that inherits from eudaq::Producer
class ExplorerProducer : public eudaq::Producer {
public:

  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  ExplorerProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), running(false), done(false) {
    fExplorer1= new DeviceExplorer();
  }

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & param) {
    std::cout << "Configuring: " << param.Name() << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "Wait");

    printf("\n--------------------- Get Config --------------------\n");
    //read from config
    options[0].first="ExplorerNumber";		//SetOptionTags
    options[1].first="PedestalMeasurement";
    options[2].first="UsePedestal";
    options[3].first="PedestalFileToUse";
    options[4].first="UseThreshold";
    options[5].first="Threshold";

    //GetTags
    ExplorerNum	=   param.Get("ExplorerNumber", 0);
    stringstream ss;
    ss << ExplorerNum;
    options[0].second	=   ss.str();
    ss.str("");

    PedestalMeas 	=   param.Get("PedestalMeasurement", false);
    ss << PedestalMeas;
    options[1].second	=   ss.str();
    ss.str("");

    UsePedestal	=   param.Get("UsePedestal", false);
    ss << UsePedestal;
    options[2].second	=   ss.str();
    ss.str("");

    PedestalFile 	=   param.Get("PedestalFileToUse", "pedestal0.ped");
    PedestalFile	=   "../data/"+PedestalFile;
    //saved @end of if/else further down

    UseThreshold	=   param.Get("UseThreshold", true);
    ss << UseThreshold;
    options[4].second	=   ss.str();
    ss.str("");

    Threshold		=   param.Get("Threshold", 10);
    ss << Threshold;
    options[5].second	=   ss.str();
    ss.str("");

    //output for the xterm
    cout<<endl
	<<"Parameters:"<<endl<<"Explorer Chip:\t\t\t"<<ExplorerNum<<endl
	<<"Pedestal Measurement:\t\t"<<(PedestalMeas==1?"Yes":"No")<<endl
	<<"Use Pedestal?\t\t\t"<<(UsePedestal==1?"Yes":"No")<<endl
	<<"Pedestal Filename(.ped):\t"<<PedestalFile<<endl
	<<"Use Threshold?\t\t\t"<<(UseThreshold==1?"Yes":"No")<<endl
	<<"Threshold (ADC-Counts):\t\t"<<Threshold<<endl<<endl;
    //Checks if everything existing and Making sense
    int nfile=-1;
    std::string filename;
    bool good=true;

    //Check logic of input -->Re-Set Tags
    if(PedestalMeas){
      //check file existence
      do{
	nfile++;
	stringstream ss2;
	ss2 << nfile;
	std::string str = ss2.str();
	filename= "../data/pedestal"+str+".ped";
	std::ifstream in(filename.c_str());
	good=in.good();
      } while(good);
      //create new filename
      PedestalFile=filename;
      cout<<endl<<"New PedestalFile: "<<endl<<PedestalFile<<endl;
      //error catch and re-set
      if(UsePedestal){
	std::cout<<"Use of Pedestal not possible!"<<endl<<"This is a Pedestal Measurement!"
		 <<endl<<"Setting UsePedestal to false!"<<endl<<endl;
	UsePedestal=false;
      }

      if(UseThreshold){
	std::cout<<"Use of Threshold not possible!"<<endl<<"This is a Pedestal Measurement!"
		 <<endl<<"Setting UseThreshold to false!"<<endl<<endl;
	UseThreshold=false;
      }
    }

    else{
      //check for file existence
      if(UsePedestal){
	std::ifstream in(PedestalFile.c_str());
	if(!in.good()){
	  std::cout<<"No Pedestal File Found!"<<endl<<"Switching to Threshold"<<endl<<endl;
	  UsePedestal=false;
	  UseThreshold=true;
	}
      }
      else{
	//threshold case
	if(!UseThreshold){
	  std::cout<<"No measure method choosen!"<<endl<<"Switching to Threshold"<<endl<<endl;
	  UseThreshold=true;
	  Threshold=0;
	}
      }
    }
    options[3].second	=   PedestalFile;	//Save new name

    printf("\n--------------------- Start Servers --------------------\n");
    if (fExplorer1->get_SD(0) < 0) {
      fExplorer1->create_server_udp(0);		//Fec0(HLVDS)
    }
    else printf("Server 0 for FEC HLVDS has already been up\n");
    if (fExplorer1->get_SD(1) < 0) {
      fExplorer1->create_server_udp(1);		//Fec1(ADC)
    }
    else printf("Server 1 for FEC ADC has already been up\n");

    printf("\n--------------------- Configure Explorer --------------------\n");
    fExplorer1->Configure();		//calls python script ConfigureExplorer.py

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;

    printf("\n--------------------- Configure DAQ --------------------\n");
    fExplorer1->ConfigureDAQ();	//calls python script ConfigureDAQ.py

    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));

    //attache tags to bore
    for(int i=0;i<6;i++){
      bore.SetTag(options[i].first,options[i].second);
    }

    printf("\n--------------------- Start Run: %d --------------------\n\n",m_run);

    // Send the event to the Data Collector
    SendEvent(bore);
    std::cout << "[ExplorerProducer] Sent BORE " << std::endl;

    stopping = false;
    running = true;
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Running");
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;

    running = false;
    // Set a flag to signal to the polling loop that the run is over
    stopping = true;

  }

  // This gets called when the Run Control is terminating
  // we should also exit.
  virtual void OnTerminate() {
    printf("\n---------------------- Terminating... ---------------------\n");

    fExplorer1->close_server_udp(0);		//close data servers
    fExplorer1->close_server_udp(1);
    done = true;

    eudaq::mSleep(1000);
  }

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    int sd0, sd1; // socket Descriptor

    unsigned char data_packet[BUFFER];	//Buffer Data
    unsigned int num_byte_rcv=0;		//number of received bytes
    unsigned int event_size=0;		//number of received adc bytes containig data and header
    unsigned char min_frm_cnt=0;		//checks if the frame with frame count 0 is there
    unsigned int nof=0;			//number of frames received and read
    bool bad=false;				//flag to indicate something is wrong with the data ->Discard Event

    // Loop until Run Control tells us to terminate
    while (!done) {

      if (!running){		//called while run not started yet
	eudaq::mSleep(50);
	continue;
      }
      else{	//running

	std::vector<unsigned char> bufferEvent_LVDS;		//Eventbuffer
	std::vector<unsigned char> bufferEvent_ADC;

	vector<unsigned char> Length;
	sd0 = fExplorer1->get_SD(0);			//socketdescriptors
	sd1 = fExplorer1->get_SD(1);

	fExplorer1->GetSingleEvent();			//Send GetSingleEvent to SRS| calls get_single_Event.py

	float timeout = 0.005; // 5 ms (longer than an acquisition)

	while(!check_socket(sd0, timeout) && !stopping) {
	  std::cout << "Requested event - waiting for its arrival" << std::endl;
	}

	if (!stopping) {
	  //###################################################################################################
	  //Ansatz for alternative solution with changing number of frames
	  /*while(reading){
	    while(  ( num_bytes_rcv = recvfrom( fExplorer1->get_SD(type) , data_packet , BUFFER , 0, NULL,NULL ) )  != 0){
	    tot_byte_rcv+=num_byte_rcv;
	    if(type==1) size_event+=num_byte_rcv;
	    nof++;
	    for( unsigned data_index = 0 ; data_index < num_byte_rcv ; data_index++ ){
	    pack((type==0?bufferEvent_LVDS:bufferEvent_ADC),data_packet[data_index]);//array if not working
	    }
	    }
	    if(type==1) reading=false;
	    type++;
	    nof=0;
	    tot_byte_rcv=0;
	    size_event=0;
	    }*/
	  //##################################################################################################


	  while (check_socket(sd0, timeout)) { //read LVDS Event
	    num_byte_rcv=recvfrom(sd0,data_packet,BUFFER,0,NULL,NULL);			//receive frame
	    event_size+=num_byte_rcv;							//increase eventsize
	    nof++;
	    for( unsigned data_index = 0 ; data_index < num_byte_rcv ; data_index++ ){	//pack data to buffer
	      pack(bufferEvent_LVDS,data_packet[data_index]);
	    }
	  }
	  if(45!=bufferEvent_LVDS.size()){	//check if data size is correct
	    printf("\n----------------------LVDS: Size not correct!------------------\n");
	    std::cout << "Number of frames: " << nof << std::endl;
	    bad=true;
	  }
	  event_size=0;		//reset values
	  nof=0;

	  while (check_socket(sd1, timeout)) {// check_socket(sd1, timeout)) { //read ADC event
	    num_byte_rcv=recvfrom(sd1,data_packet,BUFFER,0,NULL,NULL);			//receive frame
	    //very explicit cuts !!!!Data Format!!!!!
	    if(num_byte_rcv!=9 && num_byte_rcv!=8844 && num_byte_rcv!=7962) bad=true;
	    event_size+=num_byte_rcv;							//increase event size
	    nof++;
	    pack(bufferEvent_ADC,num_byte_rcv);	//add framlength befor the frame

	    if(num_byte_rcv!=9 && data_packet[11]<min_frm_cnt) min_frm_cnt=data_packet[11];	//search for minimum framecnt

	    for( unsigned data_index = 0 ; data_index < num_byte_rcv ; data_index++ ){	//pack data to buffer
	      pack(bufferEvent_ADC,data_packet[data_index]);
	    }
	  }
	  if(min_frm_cnt!=0){	//check if event corrupt
	    printf("\nFramecount not correct\nFrame 0 not contained\n");
	    std::cout << "Number of frames: " << nof << std::endl;
	    bad=true;
	  }
	  unsigned int TotalEventSize=bufferEvent_ADC.size();				//determine total event size framesize+header+data
	  pack(Length,TotalEventSize);							//create byte Vector with eventlength
	  bufferEvent_ADC.insert(bufferEvent_ADC.begin(),Length.begin(),Length.end());	//insert infront of the event data

	  if(140703!=bufferEvent_ADC.size()){		//check buffer size
	    printf("\n----------------------ADC: Size not correct!------------------\n");
	    bad=true;
	  }
	  event_size=0;	//reset values
	  nof=0;

	  //Print number of Event
	  if(m_ev<10 || m_ev%100==0) printf("\n----------------------Event # %d------------------\n",m_ev);
	  // Create a RawDataEvent to contain the event data to be sent
	  eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

	  if (bad){	//discard bad events
	    bad=false;
	    printf("\nSending empty event\n");
	  }
	  else {
	    //ADD BLOCK  Event -->0
	    ev.AddBlock(0, bufferEvent_LVDS);
	    ev.AddBlock(1, bufferEvent_ADC);
	  }
	  // Send the event to the Data Collector
	  SendEvent(ev);
	  // Now increment the event number
	  m_ev++;
	}

	if(stopping && !running){	//in case the run is stopped: finish event and send EORE
	  SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
	  std::cout << "[ExplorerProducer] Sent EORE " << std::endl;
	  while(check_socket(sd0, 1.)) { //read LVDS until empty
	    num_byte_rcv=recvfrom(sd0,data_packet,BUFFER,0,NULL,NULL);			//receive frame
	  }
	  while(check_socket(sd1, 1.)) { //read ADCS until empty
	    num_byte_rcv=recvfrom(sd1,data_packet,BUFFER,0,NULL,NULL);			//receive frame
	  }
	    SetStatus(eudaq::Status::LVL_OK, "Stopped");
	  printf("\n----------------------- Run %i Stopped ----------------------\n",m_run);
	}
      }
    } //end while
  }//end Readout loop

private:

  unsigned m_run, m_ev;			//run and event number of the instace
  char ip[14]; 				//ip string
  bool stopping, running, done;		//run information booleans
  DeviceExplorer* fExplorer1; 		//Pointer to the device object steering the setup

  //Config values
  unsigned int ExplorerNum;
  bool PedestalMeas;
  std::string PedestalFile;
  bool UsePedestal;
  unsigned int Threshold;
  bool UseThreshold;
  config options[6];


  bool check_socket(int fd, float timeout) // timeout in s
  {
    fd_set fdset;
    FD_ZERO( &fdset );
    FD_SET( fd, &fdset );

    struct timeval tv_timeout = { (int)timeout, timeout*1000000 };

    int select_retval = select( fd+1, &fdset, NULL, NULL, &tv_timeout );

    if ( select_retval == -1 ) {
	std::cout << "Could not read from the socket!" << std::endl; // ERROR!
    }

    if ( FD_ISSET( fd, &fdset ) )
      return true; // ready to read

    return false; // timeout
  }
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Explorer Producer", "1.0",
			 "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address",
				   "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "", "Explorer1", "string",
				   "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    ExplorerProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
