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
 * Device Explorer Recived the event from a UDP/IP Server.                                                              * 
 * In this class implemented:                                                                                          *
 * the methods for create a connection with server UDP/IP (create_server_udp, close_server, get_SD)		       *
 * The methods for configuring and getting events (Configure, ConfigureDAQ, GetSigngleEvent)                           *
 *                                                                                                                     *
 ***********************************************************************************************************************/
class DeviceExplorer
{
private:
	int fec0, fec1;		//socket descriptor
	struct sockaddr_in srvAddr_f0, srvAddr_f1;
    
public:
	DeviceExplorer();               // Costrutctor.
	virtual ~DeviceExplorer();      // Distructor.

	int create_server_udp(int fecn);
	void close_server_udp(int sd);
	int get_SD(int fecn);
	
	bool Configure();
	bool ConfigureDAQ();
	bool GetSingleEvent();
        };
