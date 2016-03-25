
/* Server Side GO Back N implementation */
/* Starting code by Adarsh Sethi */
/* Programmed by Pranjal Singi */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 44444

FILE *fr;   /* File pointer */

/* Structure for packet deliver */
struct Packet
{
	unsigned short count;
	unsigned short seqNumber;
	char data[80];
} packet;

int simulate(float packetLossRate, int packetDelay);   /* Simulate function declaration */
int ACKsimulate(float ackLossRate);   /* ACKSimulate function declaration */
unsigned short expectedSeqnum = 0;   /* Value of expected sequence number */
unsigned short ack;   /* value of ACK */
int packRecSuccess = 0;   /* Packet received successfully */
int bytesRec = 0;   /* Bytes received successfully */
int dupDataPack = 0;   /* Duplicate data packet */
int packDropLoss = 0;   /* Loss due to packet drop */
int ackWithoutLoss = 0;   /* ACK not lost */
int ackWithLoss = 0;   /* ACK lost */

int main(void) {

	int sock_server;  /* Socket on which server listens to clients */

	struct sockaddr_in server_addr;  /* Internet address structure that
		                        stores server address */
	unsigned short server_port;  /* Port number used by server (local port) */

	struct sockaddr_in client_addr;  /* Internet address structure that
		                        stores client address */
	unsigned int client_addr_len;  /* Length of client address structure */

	int bytes_sent, bytes_recd; /* number of bytes sent or received */
	unsigned int i;  /* temporary loop variable */
	unsigned short windowSize;   /* Value of window size */
	float packetLossRate;   /* Value of Packet loss rate between 0 and 1 */
	unsigned short packetDelay;   /* Value of packet delay */
	float ackLossRate;   /* Value of ACL loss rate between 0 and 1 */
	char *test;
	int end = 0;   /* Value for end of program */

	fr = fopen ("out.txt", "w+");   /* Open file out.txt */

	/* open a socket */

	if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Server: can't open datagram socket\n");
		exit(1);
	}

	/* initialize server address information */

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
		                        any host interface, if more than one
		                        are present */
	server_port = SERV_UDP_PORT; /* Server will listen on this port */
	server_addr.sin_port = htons(server_port);

	/* bind the socket to the local server port */

	if (bind(sock_server, (struct sockaddr *) &server_addr,
		                    sizeof (server_addr)) < 0) {
		perror("Server: can't bind to local address\n");
		close(sock_server);
		exit(1);
	}	
	
	
	srand((long)time(NULL));   /* For generating random numbers each time */
	
	/* Configuration parameters for UDP Server */
	printf("Enter Window Size : ");
	scanf("%i",&windowSize);
	printf("Enter Packet Loss Rate : ");
	scanf("%f", &packetLossRate);
	printf("Enter Packet Delay : ");
	scanf("%i", &packetDelay);
	printf("Enter ACK Loss Rate : ");
	scanf("%f", &ackLossRate);

	/* wait for incoming messages in an indefinite loop */

	printf("Waiting for incoming messages on port %hu\n\n", 
		           server_port);

	client_addr_len = sizeof (client_addr);
	
	/* Loop till the value of end becomes 1 */
	while(end != 1)
	{
		/* Bytes received by recvfrom function */
		bytes_recd = recvfrom(sock_server, (struct Packet *) &packet, sizeof(packet), 0,(struct sockaddr *) &client_addr, &client_addr_len);
		/* Converting from network to host bytes */
		packet.count = ntohs(packet.count);
		packet.seqNumber = ntohs(packet.seqNumber);
		/* Checking for packet count to be 0  */
		if(packet.count == 0)
		{
			/* End of transmission printed and loop ends */
			printf("End of Transmission Packet with sequence number %d received with %d data bytes\n", packet.seqNumber%16, packet.count);
			end = 1;
		}
		else
		{
			/* Checking condition of expected sequence number and received sequence number of packet*/
			if(expectedSeqnum == packet.seqNumber)
			{
				/* Checking for packet is lost or not */
				if(simulate(packetLossRate, packetDelay) == 1)
				{
					/* Printing of received packet information */
					printf("Packet %d received with %d data bytes\n", packet.seqNumber%16, packet.count);
					/* Saing text in out.txt */
					fputs(packet.data, fr);
					printf("Packet %d delivered to user\n", packet.seqNumber%16);
					ack = expectedSeqnum;
					expectedSeqnum++;
								
					packRecSuccess++;
					bytesRec = bytesRec + packet.count;
					/* Checking for ACK is lost or not */
					if(ACKsimulate(ackLossRate) == 1)
					{
						/* Preparing ACK and transmitting it */
						printf("ACK %d transmitted\n\n", ack%16);
						ackWithoutLoss++;
						ack = htons(ack);
						bytes_sent = sendto(sock_server, &ack, sizeof(ack), 0,(struct sockaddr*) &client_addr, client_addr_len);
					}
					else
					{
						/* When ACK lost */
						printf("ACK %d lost\n\n", ack%16);
						ackWithLoss++;
					}
				}
				else
				{
					/* When packet is lost due to simulate function */
					printf("Packet %d lost\n\n", packet.seqNumber%16);
					packDropLoss++;
				}
			}
			/* Checking condition if expected sequence number greater than received sequence number of packet*/
			else if(expectedSeqnum > packet.seqNumber)
			{
				/* Duplicate packet received. Sendiing ACK of previous correctly received packet */
				printf("Duplicate packet %d received with %d data bytes\n", packet.seqNumber%16, packet.count);
				printf("ACK %d transmitted\n\n", (expectedSeqnum-1)%16);
				dupDataPack++;
				ackWithoutLoss++;
				ack = htons(expectedSeqnum-1);
				bytes_sent = sendto(sock_server, &ack, sizeof(ack), 0,(struct sockaddr*) &client_addr, client_addr_len);
			}
			/* If expected sequence number less than received sequence number of packet*/
			else
			{	if(expectedSeqnum != 0)
				{
					/* Out of order packet received. Sendiing ACK of previous correctly received packet */
					printf("Out of order packet %d received with %d data bytes\n", packet.seqNumber%16, packet.count);
					printf("ACK %d transmitted\n\n", (expectedSeqnum-1)%16);
					dupDataPack++;
					ackWithoutLoss++;
					ack = htons(expectedSeqnum-1);
					bytes_sent = sendto(sock_server, &ack, sizeof(ack), 0,(struct sockaddr*) &client_addr, client_addr_len);
				}
			}
		}
	}
	fclose(fr);   //Close file

	/* Statistics for the blocking udpserver */
	printf("Number of data packets received successfully : %d\n", packRecSuccess);
	printf("Total number of data bytes received which are delivered to user : %d\n", bytesRec);
	printf("Total number of duplicate data packets received : %d\n", dupDataPack);
	printf("Number of data packets received but dropped due to loss : %d\n", packDropLoss);
	printf("Total number of data packets received : %d\n", packRecSuccess + dupDataPack + packDropLoss);
	printf("Number of ACKs transmitted without loss : %d\n", ackWithoutLoss);
	printf("Number of ACKs generated but dropped due to loss : %d\n", ackWithLoss);
	printf("Total number of ACKs generated : %d\n", ackWithoutLoss + ackWithLoss);
}

/* Function for simulating packet. Whether the packet is lost or delayed */
int simulate(float packetLossRate, int packetDelay)
{
	double randNumber;
	int flag, i;
	/* Generating a random number */
	randNumber = (double)rand() / (double)RAND_MAX;
	/* Checking packet lost or not */
	if(randNumber < packetLossRate)
	{
		flag = 0;
	}
	else
	{
		/* Checking if packet is delayed or not */
		if(packetDelay == 0)
		{
			flag = 1;
		}
		else
		{
			/* Generate random number for delaying packet */
			randNumber = (double)rand() / (double)RAND_MAX;
			randNumber = randNumber * 100000000;
			/* Delay packet by running loop */
			for(i = 1; i < randNumber; i++)
			{
				
			}
			flag = 1;
		}
	}

	return flag;   //return flag
}

/* Function for simulating ACK. Whether the ACK would be lost or not */
int ACKsimulate(float ackLossRate)
{
	double randNumber;
	int flag;
	/* Generating a random number */
	randNumber = (double)rand() / (double)RAND_MAX;
	/* Checking ack lost or not */
	if(randNumber < ackLossRate)
	{
		flag = 0;
	}
	else
	{
		flag = 1;
	}
	
	return flag;	//return flag
}
