# go-back-N is a protocol used in Networking

Using this protocol we will transfer a file from client to server

There are two files:
- udpserver.c
- nonblock-udpclient.c

Clone this repository. After cloning compile these two files and execute them.
Run the server first and then the client.

If you are running it locally then the server name is localhost and the port number its running on is 44444.

While running both the client and server, it will ask for certain parameters. Read them and enter accordingly.
After that type the name of the file which you want to transfer from client to server. I have provided you hello.txt.

After the file copying is done, you can see the stats. Also there will be an output file generated named out.txt which is copy of the uploaded file by client.

Analyses the stats and try to understand the code. Its well commented.

Thats it. Enjoy.!
