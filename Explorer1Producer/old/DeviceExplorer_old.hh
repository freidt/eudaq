#include <stdio.h>
#include <stdlib.h>
#include <string.h> // string function definitions
#include <unistd.h> // UNIX standard function definitions
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <termios.h> // POSIX terminal control definitionss
#include <time.h>   // time calls
#include <math.h>
/***********************************************************************************************************************
 *                                                                                                                     *
 * Device Explorer Recived the event from a TCP/IP Serve.                                                              * 
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server TCP/IP (create_client_tcp,connect_client_tcp,close_client_tcp)      *
 * The methods for start and stop run (cmdStartRun,cmdStopRun,send_instruction)                                        *
 * The methods for decode event:(scan_event,from_bit_to_binary_string,value_channels,from_bit_to_int,from_bit_to_int)  *
 * The methods for read the event:(read_frame_header_packet,read_global_header,read_data)                              *  
 *                                                                                                                     *
 ***********************************************************************************************************************/
class DeviceExplorer
{
private:
//Network TCP 
    struct sockaddr_in fClientAddr;  //client Address 	 
    int fSD; //Socket Descriptor
    
public:
	DeviceExplorer();               // Costrutctor.
	virtual ~DeviceExplorer();      // Distructor.
	int  create_client_tcp(int port,char *ip);        // Client Socket TCP.
	int  connect_client_tcp(int sd); // Connect Client Socket TCP.
	void close_client_tcp(int sd);  // Closed Client Socket TCP.
	int  cmdStartRun(); // Send Start Run
	int  cmdStopRun();  // Send Stop Run	
        void scan_event(int size_event, char * all_data, char * header, char * trailer, char * data_event ); // Read the char all_data and split the event in char string header triler and data_event
        void from_bit_to_binary_string(unsigned int byte, char *binary_string);// Convert the integer value read from binay data in a binary character string
        void value_channels(char *data,int *value_ch1,int *value_ch2,int *value_ch3,int *value_ch4);// Data contains 6 byte. Read data and calculate the int value of channels
        int from_bit_to_int(char *data,int size);// Data contains one byte. Convert this byte in intger value. Used to calculate the size of event and size of frame.
        int read_frame_header_packet(char *frame_header); // read the header frame of one udp packet return the size of frame.
        void read_global_header(char *buffer, int *num_byte_event, int *num_byte_frame);//Read the global header and update the number of byte for event and the number of byte for frame
        void read_global_trailer(char *data); // Read the trailer.
        void read_data(char *data, int end_data); // read the data.
	int send_instruction(char *word);//send istruction word for start or stop a run.
	int get_socket_descriptor();
        };
