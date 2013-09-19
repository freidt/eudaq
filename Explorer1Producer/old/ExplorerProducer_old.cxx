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
      m_run(0), m_ev(0), stopping(false), done(false) {
        fExplorer1= new DeviceExplorer();

	running = false;
        }
    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Configuring: " << param.Name() << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Wait");
      
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

      Threshold		=   param.Get("Threshold", 7);
      ss << Threshold;
      options[5].second	=   ss.str();
      ss.str("");

     	//output for the xterm 
      cout<<"Parameters:"<<endl<<"Explorer Chip:\t\t\t"<<ExplorerNum<<endl
	  <<"Pedestal Measurement:\t\t"<<(PedestalMeas==1?"Yes":"No")<<endl
	  <<"Use Pedestal?\t\t\t"<<(UsePedestal==1?"Yes":"No")<<endl
	  <<"Pedestal Filename(.ped):\t"<<PedestalFile<<endl
	  <<"Use Threshold?\t\t\t"<<(UseThreshold==1?"Yes":"No")<<endl
	  <<"Threshold (ADC-Counts):\t\t"<<Threshold<<endl;
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
			<<endl<<"Setting UsePedestal to false!"<<endl;
			//param.SetString("UsePedestal", "0");
			UsePedestal=false;
		}

		if(UseThreshold){ 
			std::cout<<"Use of Threshold not possible!"<<endl<<"This is a Pedestal Measurement!"
			<<endl<<"Setting UseThreshold to false!"<<endl;
			//param.SetString("UseThreshold", "0");
			UseThreshold=false;
		}		
	}

	else{
		//check for file existence
		if(UsePedestal){
			std::ifstream in(PedestalFile.c_str());
			if(!in.good()){
				std::cout<<"No Pedestal File Found!"<<endl<<"Switching to Threshold"<<endl;
				//param.SetString("UsePedestal", "0");
				UsePedestal=false;
				//param.SetString("UseThreshold", "1");
				UseThreshold=true;
			}
		}
		else{
			//threshold case
			if(!UseThreshold){
				std::cout<<"No measure method choosen!"<<endl<<"Switching to Threshold"<<endl;
				//param.SetString("UseThreshold", "1");
				UseThreshold=true;
				Threshold=0;
				//param.SetString("Threshold","0");
			}
		}
      }
      options[3].second	=   PedestalFile;	//Save new name
      		

      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Configured");
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      port=5000;
      sprintf(ip,"127.0.0.1"); 
      fSD=fExplorer1->create_client_tcp(port,ip);// Create socket TCP/IP
      fExplorer1->connect_client_tcp(fSD); //connect socket with device.
      fExplorer1->cmdStartRun();
      stopping = false;
      running = true;
       
      std::cout << "Start Run: " << m_run << std::endl;
      
      // It must send a BORE to the Data Collector
      eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      std::cout << "[ExplorerProducer] Sent BORE " << std::endl;
      
      // You can set tags on the BORE that will be saved in the data file		FIXME??
      // and can be used later to help decoding
      //bore.SetTag("DET", eudaq::to_string(m_exa));
      //
      //SetTags
      for(int i=0;i<6;i++){
	bore.SetTag(options[i].first,options[i].second);
	cout<<"SetTag"<<i<<endl;
      }


      // Send the event to the Data Collector
      SendEvent(bore);
      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Running");
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      std::cout << "Stopping Run" << std::endl;
      fExplorer1->cmdStopRun();
      running = false;
      // Set a flag to signal to the polling loop that the run is over
      stopping = true;
	//***###***moved to readout
      //SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
      //std::cout << "[ExplorerProducer] Sent EORE " << std::endl;
      //SetStatus(eudaq::Status::LVL_OK, "Stopped");       
    }

    // This gets called when the Run Control is terminating
    // we should also exit.
    virtual void OnTerminate() {
      std::cout << "Terminating..." << std::endl;
      fExplorer1->close_client_tcp(fSD);
      done = true;
      eudaq::mSleep(1000);
    }

    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
       char data_packet[1400];// Buffer Data
       int sd; // scoket Descriptor
       int start_run =1;
       int byte_sent;
       int byte_rcv;
       int num_byte_rcv;
       char buffer [12];
       int start_event=1;
       int size_event=0; //Number of byte per event
       int count_packet=0;
       int index_data_packet=0; // index used to scan the char array that contains the data of one packet TCP/IP.
       int end_event=0;  
       int tot_byte_recived=0;  
      // Loop until Run Control tells us to terminate
      while (!done) {
	if (!running){
	    eudaq::mSleep(50);
	    continue;
	    }
        if (running){
	    printf("ReadoutLoop Running\n");
	    std::vector<unsigned char> bufferEvent;
            end_event=0;
	    start_run=1;
	    start_event=0;
	    sd= fExplorer1->get_socket_descriptor();
	    while (end_event==0){
	        // Send Start Event
                if (start_run == 1){
		   printf("\n-------------------- Start Event %d --------------------\n",m_ev);
                   printf("\n Send Start Event -->");
                   byte_sent=send (sd,"Start Event",11,0);
	           printf("%d byte\n",byte_sent);
                   byte_rcv=recv(sd,buffer,8,0);
	           buffer[byte_rcv]='\0';
	           printf("Num Byte recived from Server: %d --> %s",byte_rcv,buffer);
                   if (byte_rcv==8 && strncmp(buffer,"End Event",8) == 0) close(sd);
	           if (byte_rcv==2){ start_event=1; start_run=2;}	
	           } 
                //Start Event	
                if (start_event==1){       
    	            num_byte_rcv=recv(sd,data_packet,sizeof(data_packet),0);	
    	            //printf("Num Byte Recived: %d\n",num_byte_rcv);
    	            size_event = size_event + num_byte_rcv;
		    tot_byte_recived=tot_byte_recived + num_byte_rcv;
    	            count_packet++;
		    //printf("\n Data: ");
    	            for (index_data_packet=0;index_data_packet< num_byte_rcv; index_data_packet++){
		        //printf("%02X",(unsigned char)data_packet[index_data_packet]);
		        pack(bufferEvent,data_packet[index_data_packet]);
                        }
               //END EVENT    
    	           if (num_byte_rcv == 8){
                       printf("\n-------------------- End Event %d --------------------\n",m_ev);
		       printf("Num Byte Recived %d  Size Event %d\n",tot_byte_recived,size_event);
		       end_event=1;
		       size_event=0;
                       }
		   }    
	       }

//Print number of Event          
           std::cout << "event Number: #" << m_ev << std::endl;           
        // If we get here, there must be data to read out
        // Create a RawDataEvent to contain the event data to be sent
            eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
//ADD BLOCK  Event -->0
            printf("Add Block Start \n");
            ev.AddBlock(0, bufferEvent);
	    printf("Add Block End \n");
        // Send the event to the Data Collector      
            SendEvent(ev);
	    printf("Send Event \n");
        // Now increment the event number
            m_ev++;
	    printf("next_event %d\n",m_ev);
	    if(stopping && !running){
      		SendEvent(eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev));
      		std::cout << "[ExplorerProducer] Sent EORE " << std::endl;
      		SetStatus(eudaq::Status::LVL_OK, "Stopped");       
	    }
	    }//end running 
	 printf("End Runnig\n");            
        } //end while
     printf("end while\n"); 	
    }//end Readout loop

  private:
  // This is just a dummy class representing the hardware
  // It here basically that the example code will compile
  // but it also generates example raw data to help illustrate the decoder
  eudaq::ExampleHardware hardware;
  unsigned m_run, m_ev;
  char ip[14]; // indirizzo ip del dispositivo.
  int port;    // porta usata dal socket.
  bool stopping, running, done,configure;// Variabile booleane di controllo
  DeviceExplorer* fExplorer1; // Object class DeviceExplorer
  int fSD; //Socket Descriptor

  //Configure
      unsigned int ExplorerNum;
      bool PedestalMeas;
      std::string PedestalFile;
      bool UsePedestal;
      unsigned int Threshold;
      bool UseThreshold;
      config options[6];

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
