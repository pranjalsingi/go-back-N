
/* Client Side GO Back N implementation */
/* Starting code by Adarsh Sethi */
/* Programmed by Pranjal Singi */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <fcntl.h>          /* for fcntl */

# define STRING_SIZE 1024

int fileRead();   /* Function declaration */

FILE *fr;   /* File pointer */

/* Structure for packet deliver */
struct Packet
{
	unsigned short count;
	unsigned short seqNumber;
	char data[80];
} packet;

struct timeval tv;    /* Declaring struct object */
int end = 0;   /* End value */ 
int base = 0;   /* Base value */
int nextSeqnum = 0;   /* Next Sequence value */
int lastSeqnum;   /* Last Sequence vlaue */
int windowSize;   /* Window Size */
int fileFlag;   /* Flag inside function fileRead */
int packetFlag = 0;   /* Flag for packet delivery */
int endFlag  = 0;   /* Flag for end of file */
int countLines = 0;   /* Flag for counting lines in file */
int timeout, temp;   /* Value of timeout */
int packTransInitial = 0;   /* Count for initial packet transmissions */
int byteTransInitial = 0;   /* Count for initial bytes transmitted */
int packReTrans = 0;   /* Count for total packets re transmitted */
int ackRec = 0;   /* Total no of ack received */
int timeoutExp = 0;   /* Count for timeout expired */
long timeout_sec, timeout_usec;   /* Value of timeout sec and usec saved */
long time_sec, time_usec;   /* Value of current time in sec and usec saved */
unsigned short ack;   /* ACK number */
char buff[16][80];   /* Buffer creation for saving packet data */
char fileName[20];   /* Filename entered by user */
int lastFlag = 0;   /* Flag for saving end of file */

int main(void) {

	int sock_client;  /* Socket used by client */ 

	struct sockaddr_in client_addr;  /* Internet address structure that
		                        stores client address */
	unsigned short client_port;  /* Port number used by client (local port) */

	struct sockaddr_in server_addr;  /* Internet address structure that
		                        stores server address */
	struct hostent * server_hp;      /* Structure to store server's IP
		                        address */
	char server_hostname[STRING_SIZE]; /* Server's hostname */
	unsigned short server_port;  /* Port number used by server (remote port) */

	int bytes_sent, bytes_recd; /* number of bytes sent or received */

	int fcntl_flags; /* flags used by the fcntl function to set socket
		       for non-blocking operation */
  
	/* open a socket */

	if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Client: can't open datagram socket\n");
		exit(1);
	}

	/* Note: there is no need to initialize local client address information
	    unless you want to specify a specific local port.
	    The local address initialization and binding is done automatically
	    when the sendto function is called later, if the socket has not
	    already been bound. 
	    The code below illustrates how to initialize and bind to a
	    specific local port, if that is desired. */

	/* initialize client address information */

	client_port = 0;   /* This allows choice of any available local port */

	/* Uncomment the lines below if you want to specify a particular 
	     local port: */
	/*
	printf("Enter port number for client: ");
	scanf("%hu", &client_port);
	*/

	/* clear client address structure and initialize with client address */
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
		                        any host interface, if more than one 
		                        are present */
	client_addr.sin_port = htons(client_port);

	/* bind the socket to the local client port */

	if (bind(sock_client, (struct sockaddr *) &client_addr,
		                    sizeof (client_addr)) < 0) {
		perror("Client: can't bind to local address\n");
		close(sock_client);
		exit(1);
	}

	/* make socket non-blocking */
	fcntl_flags = fcntl(sock_client, F_GETFL, 0);
	fcntl(sock_client, F_SETFL, fcntl_flags | O_NONBLOCK);

	/* end of local address initialization and binding */

	/* initialize server address information */

	printf("Enter hostname of server: ");
	scanf("%s", server_hostname);
	if ((server_hp = gethostbyname(server_hostname)) == NULL) {
		perror("Client: invalid server hostname\n");
		close(sock_client);
		exit(1);
	}
	printf("Enter port number for server: ");
	scanf("%hu", &server_port);

	/* Clear server address structure and initialize with server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
		                    server_hp->h_length);
	server_addr.sin_port = htons(server_port);

	/* user interface */

	/* Configuration parameters entered here */
	printf("Enter name of file to be transfered : ");
	scanf("%s", fileName);
	fr = fopen(fileName, "r");

	printf("Enter Window Size (Integer between 1-8) : ");
	scanf("%d", &windowSize);

	printf("Enter Timeout (Integer between 1-10) : ");
	scanf("%d", &timeout);
	
	/* Loop for generating timeut entered by user*/
	temp = timeout;
	timeout = 1;
	while(temp > 0)
	{
		timeout = timeout * 10;
		temp--;
	}
	/* Setting initial value of timeout in sec and usec*/
	time_sec = timeout / 1000000;
	time_usec = timeout % 1000000;

	/* Outer loop running till the end of file transmission */   
	while(end != 1)
	{
		/* Inner loop sending packets till the window size gets full and also end of transmission is reached or not*/
		while((nextSeqnum < (base + windowSize)) && endFlag == 0)
		{
			/* Checking flag whether it is original transmission or duplicate transmission */
			if(packetFlag == 0)
			{
				/* Checking that is their any data in the file */
				if(fileRead(nextSeqnum%16) == 1)
				{
					/* Setting up packet to be send to server */
					packet.count = strlen(buff[nextSeqnum%16]);
					packet.seqNumber = nextSeqnum;
					memset(packet.data,0,sizeof(packet.data));
					strcpy(packet.data, buff[nextSeqnum%16]);
					printf("Packet %d transmitted with %d data bytes\n", packet.seqNumber%16, packet.count);   /* Printing the transmission with sequence number and bytes */
				
					packTransInitial++;   // Incrementing initial packet transmission
					byteTransInitial = byteTransInitial + packet.count;   //Counting bytes transmitted

					/* Converting count and sequence number to networks bytes */
					packet.count = htons(packet.count);
					packet.seqNumber = htons(packet.seqNumber);
			
					/* Paccket send takes place */
					bytes_sent = sendto(sock_client, (struct Packet *) &packet, sizeof(packet), 0,(struct sockaddr *) &server_addr, sizeof (server_addr));
				

					/* When first time base equal to next Sequence then the timer is started */
					if(base == nextSeqnum)
					{
						/* Getting the current time of day and setting up timeout value */
						gettimeofday(&tv, NULL);
						/* Checking for the need to carry usec value */
						if((tv.tv_usec + time_usec) >= 1000000)
						{
							timeout_sec = tv.tv_sec + time_sec + (tv.tv_usec + time_usec) / 1000000;
							timeout_usec = (tv.tv_usec + time_usec) % 1000000;
						}
						else
						{
							timeout_sec = tv.tv_sec + time_sec;
							timeout_usec = tv.tv_usec + time_usec;
						}
					}
					nextSeqnum++;  /* increasing next Sequnece value */
				}
				else
				{
					endFlag = 1;  /* When end of file is reached */
					lastFlag = 1; /* Another flag set which will be used to set endFlag 1 again */
				}
			}
			else
			{
				/* Making and re-transmitting packets */
				packet.count = strlen(buff[nextSeqnum%16]);
				packet.seqNumber = nextSeqnum;
				memset(packet.data,0,sizeof(packet.data));
				strcpy(packet.data, buff[nextSeqnum%16]);
				printf("Packet %d retransmitted with %d data bytes\n", packet.seqNumber%16, packet.count);

				packReTrans++;   // Incrementing packet Re-transmission

				/* Converting to network byte */
				packet.count = htons(packet.count);
				packet.seqNumber = htons(packet.seqNumber);
				/* Re-transmission of packet done */
				bytes_sent = sendto(sock_client, (struct Packet *) &packet, sizeof(packet), 0,(struct sockaddr *) &server_addr, sizeof (server_addr));
				nextSeqnum++;
				if(lastFlag == 1 && nextSeqnum == countLines)
				{
					break;
				}
				
			}
			
		}

		packetFlag = 0;  /* Initializing packetFlag to 0 agaiin to increase value of base if ack received from server */

		/* If lastFlag was set 1 then it sets the endFlag to 1 again */
		if(lastFlag == 1)
		{
			endFlag = 1;
		}	
	
		/* Checking that all transmission is done and transmitting EOT */
		if(base == nextSeqnum && endFlag == 1 && base == countLines)
		{
			packet.count = 0;
			packet.seqNumber = nextSeqnum;
			memset(packet.data,0,sizeof(packet.data));
			printf("End of Transmision with sequence number %d with %d data bytes\n", packet.seqNumber%16, packet.count);
			
			packet.count = htons(packet.count);
			packet.seqNumber = htons(packet.seqNumber);
		
			bytes_sent = sendto(sock_client, (struct Packet *) &packet, sizeof(packet), 0,(struct sockaddr *) &server_addr, sizeof (server_addr));

			end = 1;
			break;
		}

		/* get response from server */
		printf("\nWaiting for response from server...\n");

		/* Inner do while loop for checking bytes received. Its implemented as non-blocking fashion */
		do 
		{
			bytes_recd = recvfrom(sock_client, &ack, sizeof(ack), 0,(struct sockaddr *) 0, (int *) 0);
			/* Getting current time of the day */
			gettimeofday(&tv, NULL);
			/* Checking need for borrow */
			if((tv.tv_usec - timeout_usec) < 0)
			{
				tv.tv_sec = tv.tv_sec - 1;
				tv.tv_usec = tv.tv_usec + 1000000;
			}
			/* Checking timeout expiry */
			if((tv.tv_sec >= timeout_sec) && (tv.tv_usec >= timeout_usec))
			{
				printf("Timeout expired for packet numbered %d\n", base%16);
				timeoutExp++;
				nextSeqnum = base;  /* Making next Sequence number as base to re transmit*/
				bytes_recd = 1;  /* For exciting this loop */
				packetFlag = 1;  /* Setting flag for re-transmission of packet */
				endFlag = 0;
				
				
				/* Setting new timeout for transmission */
				gettimeofday(&tv, NULL);
				if((tv.tv_usec + time_usec) >= 1000000)
				{
					timeout_sec = tv.tv_sec + time_sec + (tv.tv_usec + time_usec) / 1000000;
					timeout_usec = (tv.tv_usec + time_usec) % 1000000;
				}
				else
				{
					timeout_sec = tv.tv_sec + time_sec;
					timeout_usec = tv.tv_usec + time_usec;
				}
			}
			
		}
		while (bytes_recd <= 0);	
		
		/* Checking coondition for packet transmit. If flag 1 the re- transmission is taking place */
		if(packetFlag == 0)
		{
			/* Converting from network to host bytes */
			ack = ntohs(ack);
			printf("ACK %d received\n", ack%16);   /* Printing the ACK received */
			ackRec++;
			/* Conditions for ack received in various fashion */
			if(ack > base)
			{
				base = base + (ack - base);
			}
			if(ack == base)
			{
				base++;
			}
			
			/* Checking for base not equal to next Sequence */
			if(base != nextSeqnum)
			{
				/* Stop old and Start new timer when an ACK is received */
				gettimeofday(&tv, NULL);
				if((tv.tv_usec + time_usec) >= 1000000)
				{
					timeout_sec = tv.tv_sec + time_sec + (tv.tv_usec + time_usec) / 1000000;
					timeout_usec = (tv.tv_usec + time_usec) % 1000000;
				}
				else
				{
					timeout_sec = tv.tv_sec + time_sec;
					timeout_usec = tv.tv_usec + time_usec;
				}
			}
		}
	}

	/* Statistics for the nonblocking-udpclient */
	printf("Number of data packets transmitted : %d\n", packTransInitial);   
	printf("Total number of data bytes transmitted : %d\n", byteTransInitial);
	printf("Total number of retransmissions : %d\n", packReTrans);
	printf("Total number of data packets transmitted : %d\n", packTransInitial + packReTrans);
	printf("Number of ACKs received : %d\n", ackRec);
	printf("Count of how many times timeout expired : %d\n", timeoutExp);

	/* Closing client side file */
	fclose(fr);
	/* close the socket */
	close (sock_client);
	
}

/* Function for reading file */
int fileRead(int base)
{
	char line[80];
	/* Fetching and checking data */
	if(fgets (line, 80, fr) != NULL)
	{
		strcpy(buff[base], line);
		countLines++;   /* Lines counting */
		fileFlag = 1;   /* Flag setting for new line is there */
	}
	else
	{
		fileFlag = 0;   /* Flag for end of fetching data */
	}
	
	return fileFlag;   /* Return file flag */
}
