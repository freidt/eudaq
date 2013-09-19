#include "DeviceExplorer.hh"
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
#include <arpa/inet.h>
#include <iostream>
#include <fstream> 

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
//Costants
#define MTU_UDP_IP 9000 // Size Buffer
#define PORT 5000       // Port TCP Server
using namespace std;
//Costructor
//----------------------------------------------------------------------------------------------
DeviceExplorer::DeviceExplorer()
{
}
//Costructor
//----------------------------------------------------------------------------------------------
DeviceExplorer::~DeviceExplorer()
{
}
int DeviceExplorer::get_socket_descriptor(){
    return fSD;
    }

// Create a TCP Client to comunicate with device. 
// Parameters: Port and Ip Address
// Call When push the button Configuration.
// Set the Socket Descriptor.
// Return: 
// if there are not errors --> socket Descriptor 
// In case of errors --> -1. 
//----------------------------------------------------------------------------------------------
int DeviceExplorer::create_client_tcp(int port,char *ip){
    printf ("ip_xport %s port_xport %d\n",ip,port);	
    fClientAddr.sin_family	    = AF_INET;
    fClientAddr.sin_port	    = htons((u_short)port);
    fClientAddr.sin_addr.s_addr     = inet_addr(ip);	     
    if((fSD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror ("Created client TCP .................... FAILED ");
    	return -1;	
    	}
    printf("Created Client TCP ................... OK\n");    
    return fSD;   
}

// Connect a TCP Client with device. 
// Parameter: Socket Descriptor.
// Call When push the button Configuration.
// Set the Socket Descriptor.
// Return
// if there are not errors --> 1.
// In case of errors --> -1. 
//----------------------------------------------------------------------------------------------
int DeviceExplorer::connect_client_tcp(int sd){	
    if(connect(sd, (struct sockaddr *)&fClientAddr, sizeof(fClientAddr)) < 0){	
       perror("Connected from client TCP to Xport..... FAILED ");
       exit(0);
       return -1;		  
       }
    printf("Connected Client TCP ................. OK\n"); 
    return 1;  
}

// Close the connection with the device.
// Parameters: Socket Descriptor 
// Call when push button stop.
// Return: nothing the function it is void.
//----------------------------------------------------------------------------------------------
void DeviceExplorer::close_client_tcp(int sd){
	 close(sd);
	 printf("Closed Client TCP .................... OK\n");	
} 

//Command di Start run.
// call when push button stop
// Send the string start to server and wait the answer Ok.
// return:
// -1 if there is a problem to send start string
// 0 don't receved the answer string
// 1 send and recived the string.
//-------------------------------------------------------------------------------------------------
int DeviceExplorer::cmdStartRun(){
     char start_string[]="Start";
     int control;
     control=send_instruction(start_string);
     if (control<0) return -1;
     if (control==0) return 0;
     printf("\n----------------------- Start Run ----------------------\n");
     return 1;
     }
     
//Command di Stop run.
// call when push button stop
// Send the string stop to server, wait the answer ok and close the connection with server.
// return:
// -1 if there is a problem to send start string
// 0 don't receved the answer string
// 1 send and recived the string.
//-------------------------------------------------------------------------------------------------
int DeviceExplorer::cmdStopRun(){
     char stop_string[]="Stop";
     int control;
     
     control=send_instruction(stop_string);
     if (control<0) return -1;
     if (control==0) return 0;
     printf("\n----------------------- Stop Run ----------------------\n");
     close(fSD);
     return 1;
     } 
      
// Send the instruction word to the device.
// Recive the confir from the divce.
// Call from the functions cmdStartRun() and cmdStopRun()
// Parameters: word --> Instruction word (start o stop run).
// Return:
//  1 --> if send instruction and recive  confirm instruction correctly
//  0 --> if send instruction correctly but don't receive the confirm instruction(TIMEOUT)
// -1 --> don't send instruction correctly.
//-------------------------------------------------------------------------------------------------
int DeviceExplorer::send_instruction(char *word){
     int byte_send; // Num byte sent.
     int byte_rcv; // Num Byte recived
     char read_byte[4]; //String byte recived from server
     fd_set set; // fd used for timeout
     struct timeval timeout; //structure of timeout
     int rv; // output of select function.
     
     FD_ZERO(&set); 
     timeout.tv_sec = 2;
     timeout.tv_usec = 0;
     FD_SET(fSD, &set);
     byte_send=send (fSD,word,5,0);
     if(byte_send < 1) return -1;
     rv = select(fSD+ 1, &set, NULL, NULL, &timeout);
     if(rv == -1){ perror("select: "); return -1;} /* an error accured */
     else if(rv == 0) {/* a timeout occured */
          printf("TIMEOUT, Server don't answere.\n");
          return 0;
          } 
     else{
         byte_rcv=recv(fSD,read_byte,2,0);
	 read_byte[byte_rcv]='\0';	     
         printf("  Number byte received from device %d  %s\n",byte_rcv,read_byte);
	 return 1;
	 }
     return -1;	 
     }

// All data contains global header + frame header +data + trailer. It splited in :
// header (Global Trailer + Frame header)
// trailer
// data_event    
//------------------------------------------------------------------------------------------------
void DeviceExplorer::scan_event(int size_event, char * all_data, char * header, char * trailer, char * data_event ){
   int i; //index to scan the char string all data
   int num_byte_event; // Number byte Event
   int num_byte_frame; // Number Byte Frame
   int new_pos_header_frame =0; // Position of header frame
   //int old_pos_header_frame =0; // preview position of header frame
   int size_global_header = 20; // Number of byte of global header
   int pos_end_frame_udp =0;    // Position of end frame
   char global_header_event[21]; // Char String that contains the values of Global Header
   int size_udp_frame_header = 16; // Number of Byte of Udp frame header
   int index_frame_udp=0; //Index to scan frame udp header
   char udp_frame_header[17]; // Char String that contains the values of frame header.
   int size_trailer_event=30; // Number of byte of trailer (24 +6 bad words)
   int index_global_trailer=0; //index to scan the char string trailer_event
   char trailer_event[31];// Char String that contains the values of Global Trailer
   int index_channels_data;//index to scan the char string channels_data
   int start_pos_data=0; //Position start data
   int end_pos_data=0; // Position end data
   char channels_data[145000];// Char String that contains the values of data
   
   // Sacn Event
   for(i=0; i< size_event; i++){
       //Global Header       
       if (i>=0 && i<size_global_header){ 
           global_header_event[i]=all_data[i];
	   *header++=all_data[i];
	   }
       if ( i== size_global_header ){
           global_header_event[i]='\0';
	   read_global_header(global_header_event, &num_byte_event, &num_byte_frame); 
	   //old_pos_header_frame=new_pos_header_frame;
	   new_pos_header_frame=new_pos_header_frame + num_byte_frame + 8;
	   pos_end_frame_udp=new_pos_header_frame + size_udp_frame_header;
	   num_byte_event=num_byte_event-4;
	   start_pos_data = start_pos_data + size_global_header;
	   end_pos_data=new_pos_header_frame;
	   }
       // Frame Header  
       if ( i > size_global_header && i >= new_pos_header_frame && i < pos_end_frame_udp && i < num_byte_event){
           if (i == new_pos_header_frame)index_frame_udp=0;
	   udp_frame_header[index_frame_udp]=all_data[i];
	   *header++=all_data[i];
	   index_frame_udp ++;
	   }
       if ( i > size_global_header && i == pos_end_frame_udp && i<num_byte_event && i < num_byte_event - size_trailer_event){	   
	   udp_frame_header[size_udp_frame_header]='\0';
	   num_byte_frame=read_frame_header_packet(udp_frame_header);
	   //old_pos_header_frame=new_pos_header_frame;
	   new_pos_header_frame=new_pos_header_frame + num_byte_frame + 4;
	   pos_end_frame_udp=new_pos_header_frame + size_udp_frame_header;
	   start_pos_data=end_pos_data+size_udp_frame_header;
	   end_pos_data=new_pos_header_frame;
	   //printf("pos header next frame: %d \n",new_pos_header_frame );
           }
       // Data   
       if (i> 0 && i>= start_pos_data && i<num_byte_event-size_trailer_event && i < end_pos_data){
           if (end_pos_data > num_byte_event-size_trailer_event) end_pos_data=num_byte_event-size_trailer_event;
	   if (i==start_pos_data)index_channels_data=0;
	   channels_data[index_channels_data]=all_data[i];
	   *data_event++=all_data[i];
	   index_channels_data++;
           }
        if (i> size_global_header && i == end_pos_data){	   
	   channels_data[index_channels_data]='\0';
	   //read_data(channels_data,index_channels_data);
           }
       // Global Trailer         
       if (i>size_global_header && i >= num_byte_event - size_trailer_event && i<num_byte_event){
	   if (i == num_byte_event - size_trailer_event)index_global_trailer=0;     
           trailer_event[index_global_trailer]=all_data[i];
	   *trailer++=all_data[i];
	   index_global_trailer++;
           }
       if (i > size_global_header && i == num_byte_event){	   
	   trailer_event[size_trailer_event]='\0';
	   read_global_trailer(trailer_event); 
	   printf("\nposition end trailer %d \n",i); 	       
	   }   
	}
	//*header++='\0';
	//*trailer++='\0';
	//*data_event++='\0';
   }
   
// Scan the char string data(size 12 byte --> 48 bit)
// put in value_ch1,  put in value_ch2,  put in value_ch3,  put in value_ch4 the integer value.
//------------------------------------------------------------------------------------------------   
void DeviceExplorer::value_channels(char *data,int *value_ch1,int *value_ch2,int *value_ch3,int *value_ch4){
    int i=0; //counter number of byte used cycle for
    int pos_bit=0;
    unsigned int byte; // byte to converter in binary character 
    char buffer[8];// The string contain the conversion in binary character of all bytes
    char data_bin[48];
    char ch1[13]; // The string contains the binary character of channel 1
    char ch2[13]; // The string contains the binary character of channel 2
    char ch3[13]; // The string contains the binary character of channel 3
    char ch4[13]; // The string contains the binary character of channel 4
    int k;
    int j; //count bit inside a byte (0 -7)
    int r; // rest of the division
    int num_byte=6; //number of byte to converter.
    int n;
    
    for (i=0; i< num_byte; i++){
         byte=(unsigned char)data[i];
	// printf("Byte [%d]:%d \n",i,byte);
	// printf(" Data %02X\n",(unsigned char)data[i]);
	 j=7;
	 k=r=0;
         while ( j>=0) {
             if (byte>0){
                 r = byte %2;
	         if (r == 0) buffer[j]=48;
                 if (r == 1) buffer[j]=49;
	         byte = byte/2;
                 n++;
	         j--;
	         }
             else if (i<8){
	         buffer[j]=48;
                 n++;
	         j--;
                 }	        
             }
	 //printf ("Byte[%d]= %s \n",i,buffer);
	 buffer[8]='\0';
	 for (k=0; k<8; k++){ 
	      data_bin[pos_bit]=buffer[k];
	      pos_bit++;
	      }   
         }
	 
     for (k= 0; k<12; k++) ch1[k]=data_bin[k];
     j=-1;
     for (k=12; k<24; k++){ j++; ch2[j]=data_bin[k];}
     j=-1;
     for (k=24; k<36; k++){ j++; ch3[j]=data_bin[k];}
     j=-1;
     for (k=36; k<48; k++){ j++; ch4[j]=data_bin[k];}
     ch1[12]='\0';
     ch2[12]='\0';
     ch3[12]='\0';
     ch4[12]='\0';
     *value_ch1=strtol(ch1,NULL,2);
     *value_ch2=strtol(ch2,NULL,2);
     *value_ch3=strtol(ch3,NULL,2);
     *value_ch4=strtol(ch4,NULL,2);
     //printf("\nString data: %s\n",data_bin);
    // printf("ch1: %s %d\n",ch1,*value_ch1);
     //printf("ch2: %s %d\n",ch2,*value_ch2);
    // printf("ch3: %s %d\n",ch3,*value_ch3);
     //printf("ch4: %s %d\n",ch4,*value_ch4);	 
    }
    
// Byte contains the unsigned integer value to convert in binary char string. byte corrispond at one byte of data.
//Used from convert the integer value of the channels.    
//------------------------------------------------------------------------------------------------    
void DeviceExplorer::from_bit_to_binary_string(unsigned int byte, char *binary_string){
    //printf("Caratteri_binari byte: %d ",byte);
    char buffer[8];
    int i = 0;
    int j = 7;
    int r =0;
    while ( j>=0) {
        if (byte>0){
            r = byte %2;
	    if (r == 0) buffer[j]=48;
            if (r == 1) buffer[j]=49;
	    byte = byte/2;
            i++;
	    j--;
	    }
        else if (i<8){
	    buffer[j]=48;
            i++;
	    j--;
            }	        
        }
     buffer[8]='\0';	
     for (i=0;i<8; i++) *binary_string++ = buffer[i]; 
      
    }
    
// Convert a integer a char string data.
// Used to get the values of size event and size frame.
//------------------------------------------------------------------------------------------------    
int DeviceExplorer::from_bit_to_int(char *data,int size){
    int i;
    int k;
    int value;
    int byte;
    int pos_bit=0;
    char byte_string[8];
    char binary_string[1000];
    //printf("\n");
    for (i=0; i< size; i++){ 
	byte=(unsigned char)data[i];
	//printf("[%d] %02X %d\n",i,(unsigned char)data[i],byte);
	from_bit_to_binary_string(byte,byte_string);
	for (k=0; k<8; k++){ 
	      binary_string[pos_bit]=byte_string[k];
	      pos_bit++;
	      }   
	}
    binary_string[size*8]='\0';	
    //printf("Binary String %s",binary_string);	
    value=strtol(binary_string,NULL,2);
    binary_string[0]='\0';	
    return value;
    }
    
 // Read the Global header event and return the size of event and the size of frame header.   
//------------------------------------------------------------------------------------------------    
void DeviceExplorer::read_global_header(char *buffer, int * num_byte_event, int * num_byte_frame){
   int i;
   int j;
   char data[8];
   printf ("\n-------- Header Event --------\n");
   printf ("Size  Event:");
   j=4;
   for (i=0; i< 4; i++){ 
        j --;
        printf ("%02X ",(unsigned char)buffer[i]);
        data[i]=buffer[j];
	}
   *num_byte_event=from_bit_to_int(data,4);	   
   printf(" --> %d byte\n",*num_byte_event);
   j=8;
   printf ("Size  Udp Packet:");
   for (i=4; i< 8; i++){
        j --;
        printf ("%02X ",(unsigned char)buffer[i]);
        data[i-4]=(unsigned char)buffer[j];
	} 
   *num_byte_frame=from_bit_to_int(data,4);	   
   printf(" --> %d byte\n",*num_byte_frame);
   	   
   printf ("Event Counter :");
   for(i=8; i< 16; i++) printf ("%02X ",(unsigned char)buffer[i]);	   
   printf("\n"); 
   printf ("Data Specify :");
   for(i=16; i< 19; i++) printf ("%02X ",(unsigned char)buffer[i]);	  
   printf("\n"); 
   printf ("Frame Counter :");
   for(i=19; i< 20; i++) printf ("%02X ",(unsigned char)buffer[i]);	  
   printf("\n"); 
   }
   
 // Read the header frame  and return the size of frame header.     
//------------------------------------------------------------------------------------------------   
int DeviceExplorer::read_frame_header_packet(char *frame_header){
   int i;
   int j;
   char data[8];
   int num_byte_frame;
   printf ("\n-------- Header Frame --------\n");
   j=4;
   printf ("Size Udp Packet:");
   for (i=0; i< 4; i++){
        j --;
        printf ("%02X ",(unsigned char)frame_header[i]);
        data[i]=(unsigned char)frame_header[j];
	} 
   num_byte_frame=from_bit_to_int(data,4);	   
   printf(" --> %d byte\n",num_byte_frame);   	  
   printf ("Event Counter :");
   for(i=4; i< 12; i++) printf ("%02X ",(unsigned char)frame_header[i]);	   
   printf("\n"); 
   printf ("Data Specify :");
   for(i=12; i< 15; i++) printf ("%02X ",(unsigned char)frame_header[i]);	  
   printf("\n"); 
   printf ("Frame Counter :");
   for(i=15; i< 16; i++) printf ("%02X ",(unsigned char)frame_header[i]);	  
   printf("\n"); 
   return num_byte_frame;
   }
   
// Read the data.  
//------------------------------------------------------------------------------------------------   
void DeviceExplorer::read_data(char *data, int end_data){
   int i;
   int j;
   int pos_channels_data=0;
   int size_channels_data =6;
   char channels_data[6];
   int value_ch1;
   int value_ch2;
   int value_ch3;
   int value_ch4;
   //printf("end_data %d\n",end_data);
   for (i=0;i<end_data;i=i+size_channels_data){
        pos_channels_data=0;
	//printf ("[%d] Exadecimal Data: ",i);
	for (j=i;j<i+ size_channels_data; j++){
            channels_data[pos_channels_data]=data[j];
	    pos_channels_data++;
	    //printf ("%02X ",(unsigned char)data[j]);
	    }
	printf("\n");
	value_channels(channels_data,&value_ch1,&value_ch2,&value_ch3,&value_ch4);
	printf ("%d\t %d\t %d\t %d\t \n",value_ch1,value_ch2,value_ch3,value_ch4);    		
	}	
   }
   
// Read the Trailer.    
//------------------------------------------------------------------------------------------------
void DeviceExplorer::read_global_trailer(char *data){
   int i;
   printf ("-------- Trailer Event --------\n");
   printf ("Event Time Stamp :");
   for(i=0; i<8; i++) printf ("%02X ",(unsigned char)data[i]);	  
   printf("\n");
   printf ("Trigger Time Stamp :");
   for(i=8; i<16; i++) printf ("%02X ",(unsigned char)data[i]);	  
   printf("\n");
   printf ("Trigger Counter :");
   for(i=16; i<20; i++) printf ("%02X ",(unsigned char)data[i]);	  
   printf("\n");
   printf ("TLU Trigger Counter :");
   for(i=20; i<22; i++) printf ("%02X ",(unsigned char)data[i]);	  
   printf("\n");
   printf ("Status :");
   for(i=22; i<24; i++) printf ("%02X ",(unsigned char)data[i]);
   printf("\n");	  
   printf ("Unknow :");
   for(i=24; i<30; i++) printf ("%02X ",(unsigned char)data[i]);
   printf("\n");	  
   }
