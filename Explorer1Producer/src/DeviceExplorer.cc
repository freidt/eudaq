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
 * Device Explorer Recived the event from a UDP/IP Serve.                                                              * 
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		       *
 * The methods for configuring and getting events (Configure, ConfigureDAQ, GetSigngleEvent)                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/
//Costants
#define MTU_UDP_IP 9000 // Size Buffer
#define PORT 6006       // Port TCP Server
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

//UDP server communincation
//----------------------------------------------------------------------------------------------
int DeviceExplorer::create_server_udp(int fecn){	//create udp serves for fecs
	int server;
	char ip[14];
	sprintf(ip,"10.0.%d.3",fecn);	//build the IPs
	if(fecn==0){	//HLVDS FEC
		server = fec0 = socket (AF_INET, SOCK_DGRAM, 0);	//create socket
		if (server < 0){
	        	printf ("\n\n ERROR CREATING SOCKET: \n\n");
        	}
    		else 	printf ("\n\nCreated socket UDP .................... OK\n\n");

		srvAddr_f0.sin_family = AF_INET;
    		srvAddr_f0.sin_port = htons((u_short) PORT);
    		srvAddr_f0.sin_addr.s_addr = inet_addr(ip);

		if(bind(fec0,(struct sockaddr *)&srvAddr_f0,sizeof(srvAddr_f0))<0) 	//bind adress
			perror ( " ERROR BINDING: " );
    		else 	
			printf ("Bind Socket ........................... OK\n\n");
	}
	else	if(fecn==1){	//ADC FEC
			server = fec1 = socket (AF_INET, SOCK_DGRAM, 0);	//create socket
			if (server < 0){
                        	printf ("\n\n ERROR CREATING SOCKET: \n\n");
                	}
                	else    printf ("\n\nCreated socket UDP .................... OK\n\n");

			srvAddr_f1.sin_family = AF_INET;
                	srvAddr_f1.sin_port = htons((u_short) PORT);
                	srvAddr_f1.sin_addr.s_addr = inet_addr(ip);

			if(bind(fec1,(struct sockaddr *)&srvAddr_f1,sizeof(srvAddr_f1))<0) 	//bind adress
				perror ( " ERROR BINDING: " );
    			else 	
				printf ("Bind Socket ........................... OK\n\n");
		}
		else { cout<<"Wrong FEC number!"<<endl; return -1;}
	return server;
}

void DeviceExplorer::close_server_udp(int sd){
	close(sd);
}

int DeviceExplorer::get_SD(int fecn){	//get socket descriptors of fecs
	if(fecn==0) return fec0;
	else	if(fecn==1) return fec1;
		else { cout<<"Wrong FEC number!"<<endl; return -1;}
}

//------------------------------------------------------------------------------------------------
////call python scripts to steer SRS
bool DeviceExplorer::Configure(){
	if(system(NULL)!=0){ system("../Explorer1Producer/srs-software/ConfigureExplorer.sh"); return true; }
	else return false;
}

bool DeviceExplorer::ConfigureDAQ(){
	if(system(NULL)!=0) { system("../Explorer1Producer/srs-software/ConfigureDAQ.sh"); return true; }
	else return false;
}

bool DeviceExplorer::GetSingleEvent(){
	if(system(NULL)!=0) { system("../Explorer1Producer/srs-software/GetSingleEvent.sh"); return true; }
	else return false;
}
