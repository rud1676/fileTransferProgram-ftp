////////////////////////////////////////////////////////////////////////
//File Name	:cli.c
//Date		:2020/06/26
//OS		:Ubuntu 16.04 LTS 64bits
// Author		: Joo Kyung Jin										//
// Student ID	: 2015722023										//
// ---------------------------------------- 						//
// Title: System Programing Assignment #4 (ftp)
//	Description: After Assignment#3, client add 'get' and 'put' and 'type'
// commnad. When client connect server, server check cilent IP is vaild..
// Also, client make dataconnection and server try to connect dataserver.
// client only send data in dataconnection when command is 'STOR' and 'NLST'
// ,'LIST','RETR'. and if any command send to server, server send 
// state code (example: 200 PORT commnad~). client can know how to process 
// command in server. 
////////////////////////////////////////////////////////////////////////////
#include <string.h>		//strcmp(), strcpy(), strcat()
#include <stdio.h>		//printf()
#include <unistd.h>		//write()
#include <signal.h>		//process signal
#include <time.h>		//for make random prot num
#include <sys/wait.h>	//wait
#include<sys/stat.h>	//use stat()
#include <netinet/in.h> //sturct sockaddr_in
#include <sys/socket.h> //socket function
#include <arpa/inet.h>	//IP to string or string to IP
#include <stdlib.h>		//atoi() and exit() and so on...
////////////////////////////////////////////////////////////////
// checkArgCountError											//
//==========================================================//
//Input: char** -> argv (to receive argument from main)			//
//		int -> argc(the number of argument from main)//
//		int -> argnum(Required number of arguments for command)
//		char* -> opt(option charaters for command)
//		int -> argmode(how many argument are needed for command?)
//Output: int(error type return - what's the error					//
//Purpose: check error for many command! 									//
/////////////////////////////////////////////////////////////
int checkArgCountError(char *argv[], int argc, int argnum, char *opt, int argmode);

//////////////////////////////////////////////////////////////
// assignErrStr											//
//==========================================================//
//Input: char* -> errStr(to get error string)
//		char** -> argv (to receive argument from main)			//
//		int -> argc(the number of argument from main)//
//		int -> argnum(Required number of arguments for command)
//		char* -> opt(option charaters for command)
//		int -> argmode(how many argument are needed for command?)
// Input for use checkArgCountError so this function is
//'checkArgCountError' function dependent
//Output: int(error type return - what's the error					//
//Purpose: check error for many command! 									//
/////////////////////////////////////////////////////////////
void assignErrStr(char *errStr, char *argv[], int argc, int argnum, char *opt, int argmode);

///////////////////////////////////////////////////////////////////
//commentInput
//===============================================================
//Input 	: char** argv -> save tokenize argument.
//Ouput		: int -> like argc...
//Purpose	: input command for user and this function execute process
//toknize the command
///////////////////////////////////////////////////////////////////////////
int commentInput(char *argv[], char *buff);

/////////////////////////////////////////////////////////////////////////
//sh_int
//=================================================================
//Input		: int signum -> signal type....
//Purpose	: process SIGINT. send server QUIT, and close socket, exit this program...
//////////////////////////////////////////////////////////////////////
void sh_int(int signum);

///////////////////////////////////////////////////////////////////////////////////
//IPcheckProccess
//================================================================================
//Input, Output	: void.
//Purpose		: When client try to connect server, server check client IP valid
//by 'access.txt'. if client IP is not in the file, server reject client. then socket
//close and client abort. 
//////////////////////////////////////////////////////////////////////////////////
void IPcheckProccess();

///////////////////////////////////////////////////////////////////////////////////
//InputUorP
//================================================================================
//Input		: char* -> print
//			  char* -> command
//			'print' is which is correct 'User: ' or ' Password: '
//			command select PASS or USER command. 
//			so print and command is pair as which command.
//Output	: int
// 			state result of USER or PASS command 
//Purpose	: After IpCheck proccess, Authetication process start. then this function
//used by result of user or pass input. In a word, It's function that 
// receives an input value and authenticates it. 
///////////////////////////////////////////////////////////////////////////////////
int InputUorP(char *print,char* command);

//////////////////////////////////////////////////////////////////////////////////
//PortCommand
//===============================================================================
//input		: void
//output	: int
//			state result of PortCommand
//Purpose	: for connecting data connection, Port command is need. So client use
//this function .
//		1. input IP in structure addr_in for dataserver
//		2. making port num randomly.
//		3. save port in structure.
//		4. send port command and received state.
///////////////////////////////////////////////////////////////////////////////////
int PortCommand();

//////////////////////////////////////////////////////////////////////////////////
//userAuthentication
//==============================================================================
//Input,Ouput		:void
//Purpose			:this handles the user authentication process.
// 1. input user User name
// 2. send USER command and check user is correct by server.
// 3. after succcess, user input passwd.
// 4. send PASS command and checking.
// 5. if it's 3 times wrong, server disconnect the client and client abort.
/////////////////////////////////////////////////////////////////////////////////
void userAuthentication();

/////////////////////////////////////////////////////////////////////////////////
//readState()
//==============================================================================
//input		:void
//Ouput		:state of command proccess.
//Purpose	:this function get server state - string data. After get data, convert
//int-type data -state code.(It's return value). so, After client send command to server
//client need to this function.
/////////////////////////////////////////////////////////////////////////////////
int readState();

/////////////////////////////////////////////////////////////////////////////////
//fileSave()
//================================================================================
//input		:char -> newfile
//			newfile is filename in server.
//output	:void
//Purpose	:proccess making file data. receive data from server, and making new file
//which name file from server for save the received data. this process creates the
//same file as the server. this function user dataconnection. so, file data give and
//reieved in data connection.
///////////////////////////////////////////////////////////////////////////////////
void fileSave(char* newfile);

/////////////////////////////////////////////////////////////////////////////////
//fileSend()
//===============================================================================
//input: char -> path
//		path is filename in client area.
//output:void
//Purpose: send file data to server. this process takes in dataconnection. so, 
//follows with the port command.
///////////////////////////////////////////////////////////////////////////////
void fileSend(char* path);

int sockfd = -1; //server socket discriptor. for using SIGINT handler, it delacalred Groval variable!!

int datafd=0,dataclifd=0;	//for dataconnection-server descriptor and for getting data-connection client descriptor
struct sockaddr_in dataConnection,dataClient;	//for saving info dataconnection server and client

	char errStr[200] = "x"; //to save error-message
int main(int _, char *socketinfo[]) //It takes argument to get started...
{
	int i = 0;				//for loop
	char pf[300]={0,};		//for print result - write server and STDOUT
	char *RNTOpath;			//rename -> RNFR and RNTO so we need save RNTO path!
	char ftpcommand[1000] = {
		//convert for FTP server command
		0,
	};
	char lsResult[10000] = {
		0,
	}; //received data from server.

	char buff[1000] = {
		0,
	}; //for userinput.
	char *argv[10] = {
		0,
	};							  //for tokenize userinput -> It was used by check-error fucntion and converting.
	int argc = 0;				  //for tokenize. It's the number of argument.

	struct stat tp;//to use stat() function - it's dummy structure.

	struct sockaddr_in serv_addr; //save server socket
	opterr = 0;					  //for not print getopt() default error message
	///////////////////////signal handle///////////////////////////
	if (ssignal(SIGINT, sh_int) == SIG_ERR)
	{ //to proccess child change. call sh_chld()
		sprintf(pf,"Signal can't Catch... Program abnormal termination\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		raise(SIGINT);
	}
	////////////////////////handling end/////////////////////////

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("client Error: socket() function Fail...\n");
		return 0;
	} //regist socket and save socket discriptor

	memset(&serv_addr, 0, sizeof(serv_addr));			  //init server socket
	serv_addr.sin_family = AF_INET;						  //IPtype
	serv_addr.sin_addr.s_addr = inet_addr(socketinfo[1]); //IP
	serv_addr.sin_port = htons(atoi(socketinfo[2]));	  //port
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		sprintf(pf,"Client Error: connect() function Fail..\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		return 0;
	} //try to connect server using server socket information

	IPcheckProccess();		//first, checking client IP valid... 

	userAuthentication();		//second USER authentication process...

	while (1)		//state of getting command from user
	{
		////////////////////////initialize all buffer..///////////////////////////
		memset(errStr, 0, sizeof(errStr));
		memset(ftpcommand, 0, sizeof(ftpcommand));
		strcpy(errStr, "x"); //spectial default value of errStr is character 'x'.
		for (i = 0; i < argc; i++)
			argv[i] = NULL;
		memset(buff, 0, sizeof(buff));
		////////////////////////////end of initialize///////////////////////

		argc = commentInput(argv, buff); // input command ... and tokenize user input.
		//////////////////////////check what's command?//////////////////////////

		if (argc == 0) //input is none... this is error!
			strcpy(errStr, "Please Enter a command\n");

		else if (strcmp(argv[0], "ls") == 0)
		{												   //IF command is 'ls'
			assignErrStr(errStr, argv, argc, 0, "-al", 3); //check error
			if (strcmp(errStr, "x") == 0)				   //none error..
				argv[0] = "NLST";						   //change ftp command
		}
		else if (strcmp(argv[0], "dir") == 0)
		{												//IF command is 'dir'
			assignErrStr(errStr, argv, argc, 0, "", 3); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "LIST";						//change ftp command
		}
		else if (strcmp(argv[0], "pwd") == 0)
		{												//IF command is 'pwd'
			assignErrStr(errStr, argv, argc, 0, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "PWD";						//change ftp command
		}
		else if (strcmp(argv[0], "cd") == 0)
		{												//IF command is 'cd'
			assignErrStr(errStr, argv, argc, 1, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
			{
				if (strcmp(argv[1], "..") == 0) //if path of cd is '..' then change ftp command 'CDUP'
				{
					argv[0] = "CDUP"; //change ftp command
				}
				else				 //path is normal path...
					argv[0] = "CWD"; //change ftp command 'CWD'
			}
		}
		else if (strcmp(argv[0], "mkdir") == 0)
		{												//IF command is 'mkdir'
			assignErrStr(errStr, argv, argc, 1, "", 3); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "MKD";						//change ftp command
		}
		else if (strcmp(argv[0], "delete") == 0)
		{												//IF command is 'delete'
			assignErrStr(errStr, argv, argc, 1, "", 3); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "DELE";						//change ftp command
		}
		else if (strcmp(argv[0], "rmdir") == 0)
		{												//IF command is 'rmdir'
			assignErrStr(errStr, argv, argc, 1, "", 3); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "RMD";						//change ftp command
		}
		else if (strcmp(argv[0], "rename") == 0)
		{												//IF command is 'rename'
			assignErrStr(errStr, argv, argc, 2, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
			{
				argv[0] = "RNFR";	//first of rename argument is same argument of 'RNFR'
				RNTOpath = argv[2]; //for 'RNFR' command path
				argv[2] = "RNTO";	//input RNTO command..
				argv[3] = RNTOpath; //beacuse user command 'rename' need 'RNTO' command
			}
		}
		else if (strcmp(argv[0], "quit") == 0)
		{							  //IF command is 'quit'
			argv[0]="QUIT";			//to save QUIT command
		}
		else if ((strcmp(argv[0], "bin") == 0))
		{												//IF command is 'bin'
			assignErrStr(errStr, argv, argc, 0, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "TYPE I";						//change ftp command
		}
		else if ((strcmp(argv[0], "ascii") == 0))
		{												//IF command is 'ascii'
			assignErrStr(errStr, argv, argc, 0, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "TYPE A";						//change ftp command
		}
		else if (strcmp(argv[0], "type") == 0)
		{												//IF command is 'rename'
			assignErrStr(errStr, argv, argc, 1, "", 1); //check error
			if(strcmp(argv[1],"binary")==0){		//same as ascii and bin
				argv[0]="TYPE";
				argv[1]="I";
			}else if(strcmp(argv[1],"ascii")==0){
				argv[0]="TYPE";
				argv[1]="A";
			}else{
				strcpy(errStr,"Input type correct. ascii or binary?\n");//only two type... so another is error
			}
		}
		else if ((strcmp(argv[0], "get") == 0))
		{												//IF command is 'get'
			assignErrStr(errStr, argv, argc, 1, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "RETR";						//change ftp command
		}
		else if ((strcmp(argv[0], "put") == 0))
		{												//IF command is 'put'
			if(stat(argv[1],&tp)==-1) strcpy(errStr,"file is not existed....\n");	//error check - if file is here?
			assignErrStr(errStr, argv, argc, 1, "", 1); //check error
			if (strcmp(errStr, "x") == 0)				//none error..
				argv[0] = "STOR";						//change ftp command
		}
		else //user input unknown command
		{
			strcpy(errStr, "Enter correct command\n"); //error
			write(1, errStr, strlen(errStr));		   //if error is here, print error to standart output(shell)
			continue;
		}
		//////////////////////////end check what's command?//////////////////////////

		if (strcmp(errStr, "x") != 0)
		{
			write(1, errStr, strlen(errStr)); //if error is here, print error to standart output(shell)
			continue;
		}

		//////////////////////////print ftpcommand process///////////////////////////////
		else
		{
			if (strcmp(argv[0], "RNFR") == 0) //"rename input1 input2" convert "RNFR input1 RNTO input2". this means three arument convert four argument
			{
				for (i = 0; i < 2; i++) //so save four argument
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); //separate them with space
				}
				write(sockfd,ftpcommand,strlen(ftpcommand));	//send RNFR command
				if(readState()==550) continue;					//result is correct next step -> send RNTO

				memset(ftpcommand,0,sizeof(ftpcommand));
				for (i = 2; i < 4; i++) //so save four argument
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); //separate them with space
				}
				write(sockfd,ftpcommand,strlen(ftpcommand));//send RNTO
				readState();//get result state
				continue;
			}
			else if (strcmp(argv[0], "CDUP") == 0) // "cd .." is change "CDUP". this means two argument convert one argument
			{
				strcat(ftpcommand, argv[0]);
			}
			else if ((strcmp(argv[0], "NLST")==0)|| (strcmp(argv[0],"LIST")==0)){//"NLST" and "LIST" command use data connection
				if(PortCommand()==550)//create data connection - send PORT command
					continue;//fail PORT command... then don't proceed next...
				for (i = 0; i < argc; i++)	//send NLST or LIST
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); // separate them with space
				}
				write(sockfd,ftpcommand,strlen(ftpcommand));//send command 
				readState();//received state result

				read(dataclifd,lsResult,10000);//received command result - in dataconnection
				write(STDOUT_FILENO,lsResult,strlen(lsResult));//print that on STDOUT

				if(readState()==226){//if success command, how many data size from server?
					memset(lsResult,0,10000);
					read(dataclifd,lsResult,10000);//get the result
					write(STDOUT_FILENO,lsResult,strlen(lsResult));//print that
				}
				close(datafd);//close data connection
				continue;
			}
			else if (strcmp(argv[0],"RETR")==0){//get file from server
				if(PortCommand()==550)//connect data-connection
					continue;
				for (i = 0; i < argc; i++)
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); // separate them with space
				}
				write(sockfd,ftpcommand,strlen(ftpcommand));//send RETR command
				readState();//received result state

				fileSave(argv[1]);//process file save.
				
				continue;

			}
			else if (strcmp(argv[0],"STOR")==0){
				if(PortCommand()==550)//make data connection
					continue;//fail command
				for (i = 0; i < argc; i++)
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); // separate them with space
				}
				write(sockfd,ftpcommand,strlen(ftpcommand));//send STOR command
				readState();//received result state

				fileSend(argv[1]);//process file send to server
				
				continue;

			}
			else //the number of parameters for other command is same. so save argument according to the number of input argument.
			{
				for (i = 0; i < argc; i++)
				{
					strcat(ftpcommand, argv[i]);
					strcat(ftpcommand, " "); // separate them with space
				}
			}
			write(sockfd, ftpcommand, strlen(ftpcommand)); //The saved ftpcommand send this command
		}
		//////////////////////////end print ftpcommand process///////////////////////////////

		if(readState()==221){//if QUIT command result...
			close(sockfd);//exit process - close socekt and exit
			exit(0);
		}
	}
}

int checkArgCountError(char *argv[], int argc, int argnum, char *opt, int argmode)
{
	int countArg = 0; //for counting arguments in the user input.
	int i = 0;		  //for loop
	char o;			  //to getopt()
	optind = 0;		  //for check many argument so require initialize
	argnum++;		  //Compared value.

	/////////////start check the number of argument loop///////////////////
	for (i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-') //option is not argument
		{
			if (strcmp(argv[i], "-") == 0) //but only "-" is argument!
			{
				countArg++; //increase argcount
				continue;	//check next argument.
			}
			if (strcmp(opt, "") == 0) //this check non-option command.
				return 1;			  //non-option command must have only argument.. so return error
			continue;				  //if option... continue check next argument
		}
		countArg++; //increase arugment count
	}
	/////////////end the number of argument loop///////////////////

	/////////////check error type///////////////////////////////////////
	if (countArg > argnum && argmode != 3)						  //argument is too much for the command...
		return 2;												  //too much argument		//error
	else if (countArg < argnum && (argmode == 1 || argmode == 3)) //argument is lack for the command...
		return 3;												  //need argument error!
	else if (strcmp(opt, "") == 0)								  //There may or may not be options.
		return 0;												  //this is not error!
	else														  //option check!
	{
		while ((o = getopt(argc, argv, opt)) != -1) //getopt()function process option analysis
		{											//checking unknown option!!
			switch (o)
			{
				case '\000': //getopt sometimes this value.. so exception handling.
					break;
				case '\001': //getopt sometimes this value.. so exception handling.
					break;
				case '?':	  //if the arg has unknown option...
					return 4; //another option here.. error!
					break;
			}
		}
	}
	//////////////////end check error type///////////////////////////////////////
}

void assignErrStr(char *errStr, char *argv[], int argc, int argnum, char *opt, int argmode)
{
	switch (checkArgCountError(argv, argc, argnum, opt, argmode)) //check error (what's type of error?)
	{
		case 1: //case 1:Command without options but user input option.
			{
				strcpy(errStr, "This command doesn't use option\n");
				break;
			}
		case 2: //case 2:too many argument for the command
			{
				strcpy(errStr, "check your arguments. Too many arguments are here.....\n");
				break;
			}
		case 3: //case 3: the argument is lack for the command
			{
				strcpy(errStr, "check argument. Need argument for this command\n");
				break;
			}
		case 4: //case 4: unknown option here..
			{
				strcpy(errStr, "check option. invalid option is here\n");
				break;
			}
	}
}

int commentInput(char *argv[], char *buff)
{
	int i = 0;					   //for tokenize
	int n = 0;					   //to delete end of \n
	char *ptr = NULL;			   //for tokenizing
	memset(buff, 0, sizeof(buff)); //for read userinput...

	for (i = 0; i < 100; i++)
	{ //initialize value of argument...
		argv[i] = NULL;
	}

	write(STDOUT_FILENO, "> ", 2);		//print input your command by '>'
	n = read(STDIN_FILENO, buff, 1000); //read user command - save count n
	buff[n - 1] = '\0';					//change '\n' to '\0'

	i = 0;
	ptr = strtok(buff, " "); //start tokenize
	if (ptr == NULL)
		argv[i++] = buff; //only command... just the number of argument is noe.
	while (ptr != NULL)
	{ //tokenize command
		argv[i++] = ptr;
		ptr = strtok(NULL, " ");
	};		  //process tokenize
	return i; //the number of argument tokenized
}

void sh_int(int signum)
{
	write(sockfd, "QUIT", 4); //Signit send QUIT and action same...
	close(sockfd);			  //disconnect socket
	exit(0);				  //abort..
}

int readState(){
	char readState[300]={0,};	//to get state result string
	memset(readState,0,sizeof(readState));//initialize readState
	read(sockfd,readState,300);	//to get State as string
	write(STDOUT_FILENO,readState,strlen(readState));	//print STD about that
	return atoi(strtok(readState," "));	//convert string - int then, return value
}
void IPcheckProccess(){		//to get IPcheckProcess
	if(readState()==431){	//fail checkProccess
		close(sockfd);		//exit program..
		exit(-1);
	}
}
void userAuthentication(){	
	int count=0;//until try to third times
	int pr=0;	//to get state
	while(1){//if it is wrong, try again
		count++;//count try
		if(InputUorP("Name : ","USER")==430){//USER input
			if(count==3){//try third, exit program
				break;
			}
			continue;//fail is retry
		}
		pr=InputUorP("Password : ","PASS");//PASS input
		if(pr==430 || pr == 530){//if it's fail...
			if(count==3){
				break;//try thrid, exit program
			}
			continue;
		}
		return;//success is funtion retur.
	}
	close(sockfd);//fail third times...
	exit(0);//exit program and socket close
}
int InputUorP(char *print,char* command){
	char input[30]={0,};//for user input
	char userCommand[50]={0,};//to send command to server
	write(STDOUT_FILENO,print,strlen(print));//inform user - input USERname or PASS
	read(STDIN_FILENO,input,20);//input data from user
	strstr(input,"\n")[0]='\0';//enter is not need
	sprintf(userCommand,"%s %s",command,input);//convert command to server
	write(sockfd,userCommand,strlen(userCommand));//send server USER command
	return readState();//to get state 
}

int PortCommand(){
	char addr[100]={0,};
	char randnum1[10]={0,}, randnum2[10]={0,};	//to make random prot
	int i=0;	// for loop
	int chrand=0;
	int n=0;
	memset(&dataConnection,0,sizeof(dataConnection));	//prepare making DataConnection
	dataConnection.sin_family = AF_INET;				//save ip and port in dataconnection - saver information
	dataConnection.sin_addr.s_addr = inet_addr("127.0.0.1");			

	memset(addr,0,sizeof(addr));						//set zero..
	/////////////////////make PORT Commmand////////////////////////////
	strcpy(addr,"PORT ");								
	strcat(addr,inet_ntoa(dataConnection.sin_addr));//change ip addr -> dotted string
	for(i=0;i<strlen(addr);i++){			//PORT command only ','
		if(addr[i]=='.') addr[i]=',';
	}
	srand((unsigned int)time(NULL));		//make random port num
	while(1){
		chrand=rand()%1000;
		if(chrand>=40 && chrand <=230)break;
	}
	sprintf(randnum1,"%d",chrand);
	srand((unsigned int)time(NULL));
	sprintf(randnum2,"%d",rand()%1000);
	strcat(addr,",");
	strcat(addr,randnum1);	//input random num in PORT command
	strcat(addr,",");
	strcat(addr,randnum2);
	///////////////////end making PORT command/////////////////////	

	dataConnection.sin_port=htons(atoi(randnum1)*256+atoi(randnum2));	//set port- for data_connection
	datafd=socket(PF_INET,SOCK_STREAM,0);								//making socket descriptor - data connection

	bind(datafd,(struct sockaddr*)&dataConnection,sizeof(dataConnection));	//bind() about data connection
	listen(datafd,5);					//listen() about data connection
	
	write(sockfd,addr,strlen(addr));		//return PORT command
	memset(&dataClient,0,sizeof(dataClient));	
	n=sizeof(dataClient);
	dataclifd=accept(datafd,(struct sockaddr*)&dataClient,&n);
	return readState();
}

void fileSave(char* newfile){
	FILE* f=0;	//file struture
	int readlen=0;//how many byte read from file
	int fileSize=0;//total file size	
	int totalbufNum=0;//how many buffer to get file data
	int bufferNum=0;//cur buffer count
	char buf[1024]={0,};//to get file data
	char *mode=NULL;//ascii? or bin?
	char result[50]={0,};//to get how many data sended...

	readlen=read(dataclifd,buf,1024);	//to get file size from server
	if(readlen==0) {	//no size.. so error
		readState();
		return;
	}
	fileSize=atoi(strtok(buf," "));//chagne file size- string to int
	totalbufNum = fileSize/1024 + 1;//caculate total buffer num	
	mode = strtok(NULL,"");		//check ascii or bin

	write(dataclifd,"OK",2);		//get OK signed..
	if(strcmp(mode,"ASCII")==0)//fopen Open in different formats depending on the mode
		f=fopen(newfile,"wt");
	else
		f=fopen(newfile,"wb");
	
	while(bufferNum != totalbufNum){	//until it receive the file data
		readlen = read(dataclifd,buf,1024);//get file data in buf
		bufferNum++;//cur count increase..
		fwrite(buf,sizeof(char),readlen,f);//to save file data from server in client -create-file
	}
	write(dataclifd,"OK",2);//all data is send, server need to know OK signed... 
	fclose(f);//file close
	if(readState()!=226){//get result state
		close(datafd);//fail this command... return function
		return;
	}

	read(dataclifd,result,50);	//get how many byte send.. from server
	write(STDOUT_FILENO,result,strlen(result));//print STDOUT
	close(datafd);//data connection close
}

void fileSend(char* path){
	FILE* f=0;//file structure
	int buflen=0;//how many read from file
	int fileSize=0;//to get filesize	
	char readbuf[1024]={0,};//file read buffer

	f = fopen(path,"rb");//open file

	fseek(f,0,SEEK_END);//process check file size
	fileSize = ftell(f);
	fseek(f,0,SEEK_SET);	

	sprintf(readbuf,"%d",fileSize);	//to send filesize to server
	write(dataclifd,readbuf,sizeof(readbuf));

	read(dataclifd,readbuf,sizeof(readbuf));	//get OK signal

	memset(readbuf,0,sizeof(readbuf));
	while(buflen=fread(readbuf,sizeof(char),sizeof(readbuf),f)){//until file EOF
		if(buflen<=0) break;//EOF or error is break.
		write(dataclifd,readbuf,buflen);//send the file data
		memset(readbuf,0,sizeof(readbuf));//clear readbuf
	}
	fclose(f);//file close
	if(readState()!=226){//if state fail
		close(datafd);//function return
		return;
	}
	read(dataclifd,readbuf,sizeof(readbuf));//read how many data recevied...
	write(STDOUT_FILENO,readbuf,sizeof(readbuf));//print that STDOUT
	close(dataclifd);//close data connection.
}

