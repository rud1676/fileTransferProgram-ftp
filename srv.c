/////////////////////////////////////////////////////////////////////
// File Name	: srv.c										//
// Data			: 2020/06/26										//
// Os			: Ubuntu 16.04 LTS 64bits							//
// Author		: Joo Kyung Jin										//
// Student ID	: 2015722023										//
// ---------------------------------------- 						//
// Title: Assignment#4-ftp server
// Description: Unlike the previous assignment, server check client IP
// and client USER and PASS. IP check proceeds as soon as the client
// conncets. userAuthentication proceeds client send USER and PASS command.
// It is ready to receive commands after all the process is over.
// Unlike befoe,RETR and STOR command have been added.
/////////////////////////////////////////////////////////////////////
#include <stdio.h>	  //sprintf()
#include <stdlib.h>	  //free(), exit()
#include <unistd.h>	  //lsat(), write(), read(),getcwd(),chdir(),rmdir(),mkdir(),rename(),remove()
#include <dirent.h>	  //struct dirent,scandir(),alphasort
#include <string.h>	  //strcat(),strcpy(),strtok(),strcmp(),strrchr()
#include <sys/stat.h> //struct stat,lstat(), about file macros
#include <pwd.h>	  //getpwuid()
#include <grp.h>	  //getgrgid()
#include <time.h>	  //time_t,tm,strftime(),localtime()
#include <arpa/inet.h>//inet_ntoa()
#include <sys/wait.h>	//wait()
#include <signal.h>		//for signal handling
#include <time.h>		//time_t time()
#define MIN(a,b) ((a<b)?(a):(b))	//macro define to pasing IP in txt
//////////////////////////////////////////////////////////////
// toGetpermission											//
//==========================================================//
//Input:struct stat -> buf(get file info..)
//		char* -> plist(save file info to print 'ls' result )
//Output: void - result value save in plist...					//
//Purpose: check file information and save to 'ls' format
//string about file info. this function is pItemL dependency	//
/////////////////////////////////////////////////////////////
void toGetpermission(struct stat buf, char *plist);

//////////////////////////////////////////////////////////////
// checkColor											//
//==========================================================//
//Input:struct stat -> buf(get file info..)
//Output: char - result charater value for file type...					//
//Purpose: as file type, result is color.(regular is white)	//
/////////////////////////////////////////////////////////////
char checkColor(struct stat buf);

//////////////////////////////////////////////////////////////
// printColor											//
//==========================================================//
//Input:char* -> plist(to print result)						//
//		char -> color(what's color?)						//
//Output: void - result is in plist. beacause format character//
//convert real color character!								//
//Purpose: to get real color character by color character. 	//
//this function related to checkColor.
/////////////////////////////////////////////////////////////
void printColor(char *plist, char color);

//////////////////////////////////////////////////////////////
// pItemL(printItemList)											//
//==========================================================//
//Input:struct stat -> buf(get a file..)//
//		char* -> plist(save file info to print 'ls' result )//
//		char* -> filename(for save filename. buf don't have filename..)//
//Output: void - result value save in plist...					//
//Purpose: For ls -l option. we can print detail file information.//
//this function to get one file detail info. this is printItems function dependency	//
/////////////////////////////////////////////////////////////
void pItemL(struct stat buf, char *plist, char *filename);

//////////////////////////////////////////////////////////////
// printItems(printItems)											//
//==========================================================//
//Input:struct struct dirent ** -> dir(get directory information)//
//		int -> count(the number of file in directory )//
//		int -> Aopt(has -a option? - 0: none, 1 true)//
//		int -> Lopt(has -a option? - 0: none, 1 true)//
//		char*-> result(to send result for client)
//Output: void - result value save in plist...					//
//Purpose: This function save files infomation in directory. so //
//print them in the form of options. default is only filename and
//Five ar printed on one line. '-a' option is print all file(Even hidden files)
//'-l' option print file info detail. so print one by line.
//This function is NLSTfunc dependency
/////////////////////////////////////////////////////////////
void printItems(char* result,struct dirent **dir, int count, int Aopt, int Lopt);

//////////////////////////////////////////////////////////////
// NLSTfunc('ls'function)											//
//==========================================================//
//Input:char -> *ftpcommand(to get full command)//
//		int mode(0: use by 'NLST' 1:use by'LIST")//
//Output: int - result value Success(0) or fauilure(-1)					//
//Purpose: This function execute 'NLST' command.if the argument
//can't load file info, throw error(to save errStr). and 'NLST' is different
//by each argument. argument is file... default option is '-l'.
//folder is default option is 'al'							//
/////////////////////////////////////////////////////////////
int NLSTfunc(char *ftpcommand, int mode);

//////////////////////////////////////////////////////////////
// PWDfunc('pwd'function)											//
//==========================================================//
//Input: None
//Output: void - print direct stdout.					//
//Purpose: print current directory to stdout
///////////////////////////////////////////////////////////////
void PWDfunc();

//////////////////////////////////////////////////////////////
// CWD('cd'function)											//
//==========================================================//
//Input: char ->*path (to change path)
//		int -> mode(select 'CWD' - 0 or 'CDUP' - 1)
//Output: function Success or faile value					//
//Purpose: change working directory by chdir() function. end of function
// call PWDfunc() check alright. this function use 'CWD' and 'CDUP'. this
// is selected by mode.													//
///////////////////////////////////////////////////////////////
void CWD(char *path, int mode);

//////////////////////////////////////////////////////////////
// MKD('mkdir'function)											//
//==========================================================//
//Input: char ->*path (to path for making directory)
//Output: void. this function only make dir. error string save errStr					//
//Purpose: making directory for path.										//
///////////////////////////////////////////////////////////////
void MKD(char *path);


//////////////////////////////////////////////////////////////
// RMD('rmdir'function)											//
//==========================================================//
//Input: char ->*path (to path for delete directory)
//Output: void. this function only delete dir. error string save errStr					//
//Purpose: delet directory for path.										//
///////////////////////////////////////////////////////////////
void RMD(char *path);

//////////////////////////////////////////////////////////////
// DELE('delete'function)											//
//==========================================================//
//Input: char ->*path (to path for delete file)
//Output: void. this function only delete file. error string save errStr					//
//Purpose: delet file for path.										//
///////////////////////////////////////////////////////////////
void DELE(char *path);

//////////////////////////////////////////////////////////////
// RNFR('rename'function)											//
//==========================================================//
//Input: char ->*oldname (to path for change)
//Output: void...					//
//Purpose: to get oldname file or directory. so check the oldname is existed?//
///////////////////////////////////////////////////////////////
void RNFR(char* path,char *oldfile);

//////////////////////////////////////////////////////////////
// RNTO('rename'function)											//
//==========================================================//
//Input: char ->*oldname (to path for change)
//		 char ->*newname (to path for change)
//Output: void...					//
//Purpose: execute rename file or directory. and check exception//
///////////////////////////////////////////////////////////////
void RNTO(char *newfile, char *oldfile);

////////////////////////////////////////////////////////////////////////
//IPAccessControl
//=====================================================================
//Input: void
//Output: int
//		It's return process result state code.
//Purpose: when client try to connect to server, server call this function
//and check client IP is in access.txt. after Check IPvalid,
//fail is client reject, success is proceeds to the next
/////////////////////////////////////////////////////////////////////////
int IPAccessControl();

//////////////////////////////////////////////////////////////////
//user_match
//======================================================================
//Input: char* -> username
//		char* -> user
//		username is save user input value -> for send state string to client
//		user is compare value from client USER command
//Output: int
//		success is 1 fail is 0
//Purpose: After IPAccessControl, User authentication is performed.
//this function compare client input user data to user name in passwd file.
///////////////////////////////////////////////////////////////////////////
int user_match(char *username,char* user);

//////////////////////////////////////////////////////////////////////////////
//userAuthentication
//===========================================================================
//Input:void
//Output:int
//		return value is state code..
//Purpose:all process Authentication.
// 1. input user- id
// 2. send command USER
// 3. input user -passwd
// 4. send command -PASS
//it try to third times...
//return value is state code!
///////////////////////////////////////////////////////////////////////////////
int userAuthentication();

////////////////////////////////////////////////////////////////////////////
//PORTcommand()
//============================================================================
//input 	:char* -> arg
//			arg is ip and port num - divide by ','
//Output	:void
//Purpose	: process PORT command. first, convert ip - use inet_atoi() fucntion
//and then create random portnum and save those in addr structure.
//After this, making data-connection and try to connect client - data server.
///////////////////////////////////////////////////////////////////////////////
void PORTcommand(char *arg);

///////////////////////////////////////////////////////////////////////////////
//TYPEcommand
//=============================================================================
//input		:char* ->arg
//			arg is type. ascii(A) or bin (B)
//output	:void
//Purpose	:change mode type - ascii or bin. global variable as arg - curType.
///////////////////////////////////////////////////////////////////////////////
void TYPEcommand(char* arg);

/////////////////////////////////////////////////////////////////////////////
//RETRcommand
//==========================================================================
//input		: char* ->path
//			path is file in server
//output	: void
//Purpose	: RETR command is send data to client. first, read file in server
//and read buf send client - repeat until EOF. and then statecode and result
//value send.
/////////////////////////////////////////////////////////////////////////////
void RETRcommand(char* path);

//////////////////////////////////////////////////////////////////////////////
//STORcommand
//==========================================================================
//input		:char* -> newfile
//			newfile is filename to get client file
//output	:void
//Purpose	: STORcommand is received data to client. recieved data, and then
//write the data to file. same is client get file from server.
/////////////////////////////////////////////////////////////////////////////
void STORcommand(char* newfile);

//////////////////////////////////////////////////////////////
// sh_usr2											//
//==========================================================//
//Input:int signum -> type of signal 
//Purpose: define my singal to instruct their children process
// to do something special. - when SIGINT handler called, the function
// called SIGUSR2 function
///////////////////////////////////////////////////////////////
void sh_usr2(int signum);

//////////////////////////////////////////////////////////////
// sh_int											//
//==========================================================//
//Input:int signum -> type of signal 
//Purpose: when SIGINT signal come to this process, this looks like
//termination. so parent process give signal SIGUSR2 to children process
//all connection become disconnect and all children abort. and exit parent proccess..
///////////////////////////////////////////////////////////////
void sh_int(int signum);

//////////////////////////////////////////////////////////////
// sh_chld											//
//==========================================================//
//Input:int signum -> type of signal 
//Purpose: when children exit, parent received SIGCHLD signal//
//so parent need to change children process information which 
//is in parent process (the number of child proccess)
///////////////////////////////////////////////////////////////
void sh_chld(int signum);

pid_t pid[100]; //for saving processes id -> child process is 0
int childNum =0;	//the number of childproccess

int server_fd, client_fd,datafd;                    //server socket descirptor, cilent socket descriptor.
struct sockaddr_in server_addr, client_addr,dataServer; //server socket data, client socket data.

time_t st=0;	//to get program execute time....

int curType = 0; //0 is binary, 1 is ascii

int main(int argc,char* argv[])
{
	char pf[300]={0,};	//for print STD
	char getCommand[1000] = {
		0,
	};						//read stdinput...
	char oldname[100] = {
		0,
	}; //for rename
	char *token2=NULL;		//parse argument
	char *token;			//to parse command
	int statecode=0;		//to get state - IPaccess and authentication
	int len;                                     //to get sizeof...

	time_t t;	//to get access time
	char *ct;	//to convert string - time

	char username[20]={0,};	//to save ID information
	////////////////////Signal handler regist!//////////////////////////////////////
	if(signal(SIGCHLD,sh_chld)==SIG_ERR){
		strcpy(pf,"Signal can't Catch... Program abnormal termination\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		raise(SIGINT);
	}
	if(signal(SIGINT,sh_int)==SIG_ERR){
		strcpy(pf,"Signal can't Catch... Program abnormal termination\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		raise(SIGINT);
	}

	if(signal(SIGUSR2,sh_usr2)==SIG_ERR){
		strcpy(pf,"Signal can't Catch... Program abnormal termination\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		raise(SIGINT);
	}
	///////////////////////////end signal handler regist////////////////////////////////

	//////////////////////////socket regist proccess////////////////////////////////////////////////////
	server_fd = socket(PF_INET, SOCK_STREAM, 0); //make server socket descriptor and regist this to kernel
	if(server_fd<0){
		strcpy(pf,"Server Error:socket function Fail...\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		return 0;
	}

	memset(&server_addr, 0, sizeof(server_addr));    //init server_addr
	server_addr.sin_family = AF_INET;                //server IP-type
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //server IP
	server_addr.sin_port = htons(atoi(argv[1]));     //server port

	if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0){
		strcpy(pf,"Server Error:bind() function Fail...\n");
		write(STDOUT_FILENO,pf,strlen(pf));
		return 0;
	} //save server socket to socket and to regist this in kernel
	listen(server_fd, 5);                                                  //check client request.
	//////////////////////////////end regist///////////////////////////////////////////////////////////

	/////////////////////////////////connect client and make child proccess corresponding to client/////////////////////////////////////
	while(1){
		len = sizeof(client_addr);		//for client socket
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len); //permit request and kernel make client socket.
		if (client_fd < 0){         //connect fail....
			strcpy(pf,"Server Error: accept() Fail...\n");
			write(STDOUT_FILENO,pf,strlen(pf));
			continue;
		}			//program exit

		pid[childNum++] = fork(); // make child proccess for this client
		time(&st);		//count start proccess time!

		if(pid[childNum-1] ==0){//if child process....

			//////////////////////////////IP access process//////////////////////////////////////
			statecode = IPAccessControl();
			if(statecode == 220){	//access success
				time(&t);
				ct=ctime(&t);
				sprintf(pf,"220 sswlab.kw.ac.kr FTP server (version myftp [1.0] %s KST) ready.\n",ct);
				write(client_fd,pf,strlen(pf));
				write(STDOUT_FILENO,pf,strlen(pf));
			}
			else if(statecode == 431){	//access fail
				sprintf(pf,"431 This client can't access. Close the session.\n");
				write(client_fd,pf,strlen(pf));
				write(STDOUT_FILENO,pf,strlen(pf));
				close(client_fd);
				exit(0);				//child process exit...
			}
			/////////////////////////////////////////////////end IPAccess/////////////////////////////////////////

			//////////////////////////Authentication/////////////////////////////////////////////////
			statecode = userAuthentication(username);	
			if(statecode ==230){		//success
				sprintf(pf,"230 logged in\n");
				write(client_fd,pf,strlen(pf));
				write(STDOUT_FILENO,pf,strlen(pf));
			}else if(statecode == 530){	//fail
				sprintf(pf,"530 Fail to log-in\n");
				write(client_fd,pf,strlen(pf));
				write(STDOUT_FILENO,pf,strlen(pf));
				close(client_fd);
				exit(0);				//child process eixt and close socket
			}
			////////////////////////////////////////////////////////end /////////////

			////////////////////////////////////get command until client quit!/////////////////////////////////////////////
			while(1){
				memset(getCommand,0,sizeof(getCommand)); //getCommand must initialize 0 because of readding command
				len = read(client_fd, getCommand, 1000);	//to get stdinput getcommand
				write(STDOUT_FILENO,getCommand,sizeof(getCommand)); //user input rename is specail so not print command now...
				write(STDOUT_FILENO,"\n",strlen("\n"));		//print next line.
				if(len<0){			//read fail!
					close(client_fd); //Maybe client disconnect so server disconnect the client.
					exit(0);		//the proccess exit
				}
				token = strtok(getCommand, " "); //parse command
				token2 = strtok(NULL,"");//parse argumnet..
				/////////////////////////process command//////////////////////////////
				if (strcmp(token, "NLST") == 0) //'NLST"
				{
					NLSTfunc(token2, 0);
				}
				else if (strcmp(token, "LIST") == 0) //'LIST"
				{
					NLSTfunc(token2, 1);
				}
				else if (strcmp(token, "PWD") == 0) //'PWD"
				{
					PWDfunc();
				}
				else if (strcmp(token, "CWD") == 0) //'CWD"
				{
					CWD(token2, 0);
				}
				else if (strcmp(token, "CDUP") == 0) //'CDUP"
				{
					CWD(token2, 1);
				}
				else if (strcmp(token, "MKD") == 0) //'MKD"
				{
					MKD(token2);
				}
				else if (strcmp(token, "RMD") == 0) //'RMD"
				{
					RMD(token2);
				}
				else if (strcmp(token, "DELE") == 0) //'DELE"
				{
					DELE(token2);
				}
				else if (strcmp(token, "RNFR") == 0) //'RNFR"
				{
					RNFR(token2,oldname);		
				}
				else if (strcmp(token, "RNTO") == 0) //'RNTO'
				{
					RNTO(token2,oldname);		
				}
				else if (strcmp(token, "PORT") == 0) //'PORT'
				{
					PORTcommand(token2);		
				}
				else if (strcmp(token, "TYPE") == 0) //'TYEP'
				{
					TYPEcommand(token2);		
				}
				else if (strcmp(token, "RETR") == 0) //'RETR'
				{
					RETRcommand(token2);		
				}
				else if (strcmp(token, "STOR") == 0) //'STOR'
				{
					STORcommand(token2);
				}
				else if (strcmp(token, "QUIT") == 0) //"QUIT"
				{
					write(client_fd, "221 Goodbye\n", strlen("221 Goodbye\n")); //print QUIT.
					raise(SIGUSR2);	//child server exit...
					break;
				}
				/////////////////////////end process command//////////////////////////////
			}
			//////////////////////////////////////////endline get commmand from client/////////////////////////////////////////////////////
		}else        {
			///////////////////////////parent proccess print info client program.///////////////////////////////
			strcpy(pf,"=========Clinet info==========\n");
			write(STDOUT_FILENO,pf,strlen(pf));
			sprintf(pf,"client IP: %s\n\nclient port: %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
			write(STDOUT_FILENO,pf,strlen(pf));
			strcpy(pf,"==============================\n");
			write(STDOUT_FILENO,pf,strlen(pf));
			sprintf(pf,"Child Process ID : %d\n\n", pid[childNum-1]);
			write(STDOUT_FILENO,pf,strlen(pf));
			/////////////////////////////////end print info/////////////////////////////////////////////////////////////
		}
	}
	///////////////////////////////endline connect client and make child proccess corresponding to client/////////////////////////////////////
}

void toGetpermission(struct stat buf, char *plist)
{
	///////////////////////check file type -directory, regular... and so on/////////////
	if (S_ISREG(buf.st_mode))
	{
		strcat(plist, "-"); //convert file info to string(save plist)
	}
	else if (S_ISDIR(buf.st_mode))
	{
		strcat(plist, "d"); //convert file info to string(save plist)
	}
	else if (S_ISCHR(buf.st_mode))
	{
		strcat(plist, "c"); //convert file info to string(save plist)
	}
	else if (S_ISBLK(buf.st_mode))
	{
		strcat(plist, "b"); //convert file info to string(save plist)
	}
	else if (S_ISFIFO(buf.st_mode))
	{
		strcat(plist, "p"); //convert file info to string(save plist)
	}
	else if (S_ISLNK(buf.st_mode))
	{
		strcat(plist, "l"); //convert file info to string(save plist)
	}
	else if (S_ISSOCK(buf.st_mode))
	{
		strcat(plist, "s"); //convert file info to string(save plist)
	}
	///////////////////////end check file type -directory, regular... and so on/////////////

	///////////////////////check file permission -user group other/////////////////////////
	if (S_IRUSR & buf.st_mode) //have user read permission?
	{
		strcat(plist, "r"); //yes
	}
	else
		strcat(plist, "-");	   //no..
	if (S_IWUSR & buf.st_mode) //have user write permission?
	{
		strcat(plist, "w");
	}
	else
		strcat(plist, "-");
	if (S_IXUSR & buf.st_mode) //have user execute permission?
	{
		strcat(plist, "x");
	}
	else
		strcat(plist, "-");
	if (S_IRGRP & buf.st_mode) //have group read permission?
	{
		strcat(plist, "r");
	}
	else
		strcat(plist, "-");
	if (S_IWGRP & buf.st_mode) //have group write permission?
	{
		strcat(plist, "w");
	}
	else
		strcat(plist, "-");
	if (S_IXGRP & buf.st_mode) //have group execute permission?
	{
		strcat(plist, "x");
	}
	else
		strcat(plist, "-");
	if (S_IROTH & buf.st_mode) //have other read permission?
	{
		strcat(plist, "r");
	}
	else
		strcat(plist, "-");
	if (S_IWOTH & buf.st_mode) //have other write permission?
	{
		strcat(plist, "w");
	}
	else
		strcat(plist, "-");
	if (S_IXOTH & buf.st_mode) //have other execute permission?
	{
		strcat(plist, "x");
	}
	else
		strcat(plist, "-");
	///////////////////////end check file permission -user group other/////////////////////////
	strcat(plist, " "); //To distinjuish next info.
}

char checkColor(struct stat buf)
{
	/////////////////checkfile- directory? symbolic link? execute permission?//////////////////
	if (S_ISDIR(buf.st_mode))
		return 'd';
	else if (S_ISLNK(buf.st_mode))
	{
		return 'c';
	}
	else if (S_IXUSR & buf.st_mode)
	{
		return 'x';
	}
	else if (S_IXGRP & buf.st_mode)
	{
		return 'x';
	}
	else if (S_IXOTH & buf.st_mode)
	{
		return 'x';
	}
	/////////////////end checkfile- directory? symbolic link? execute permission?//////////////////

	return 0; // not search... this is white..
}

void printColor(char *plist, char color)
{
	////////////////////convert real color////////////////////////
	switch (color)
	{
		case 'c':
			strcat(plist, "\033[1;33m"); //yellow
			break;
		case 'x':
			strcat(plist, "\033[1;32m"); //green
			break;
		case 'd':
			strcat(plist, "\033[1;34m"); //bold blue
			break;
		case '0':
			strcat(plist, "\033[0m"); //white color
			break;
	}
	////////////////////end convert real color////////////////////////
}

void pItemL(struct stat buf, char *plist, char *filename)
{
	char info[200];				 //to save type ~ permission
	char color = 0;				 //to print real another color.
	struct passwd *pw;			 //to convert userid. from int to string
	struct group *gr;			 //to convert groupid. from int to string
	struct tm *lt;				 //to convert time infor. It can be printed in the format I want.
	toGetpermission(buf, plist); //to save permission and type

	sprintf(info, "%-1d ", (unsigned int)buf.st_nlink); //to save formating string in 'info'
	strcat(plist, info);								//then save in plist.

	pw = getpwuid(buf.st_uid);			 //to get file user id(int) and convert passwd struct then save pw.
	sprintf(info, "%-6s ", pw->pw_name); // to save user id in 'info'
	strcat(plist, info);				 //then save in plist.

	gr = getgrgid(buf.st_gid); //same pw
	sprintf(info, "%-6s ", gr->gr_name);
	strcat(plist, info);

	sprintf(info, "%8d", (unsigned int)buf.st_size); //save file size
	strcat(plist, info);							 //save in plist

	lt = localtime(&buf.st_mtime);			  //to get modified time . then convert struct tm.
	strftime(info, 200, " %b %e %H:%M ", lt); //to save according to the format.
	strcat(plist, info);					  //save in plist

	color = checkColor(buf); //to get color info!(about file-type and permission)
	if (color != 0)			 //if there are special colors...
	{
		///////////////////////color change process/////////////////////////
		printColor(plist, color);
		strcat(plist, filename); //file name save with special color.
		printColor(plist, '0');
		///////////////////////end color change process/////////////////////////
	}
	else
		strcat(plist, filename); //file name save with white.
	strcat(plist, "\n");
}

void printItems(char* result,struct dirent **dir, int count, int Aopt, int Lopt)
{
	struct stat buf; //to get a file info one by one in directory.

	char plist[100000] = {
		0,
	};					//to print files in directory
	char getitem[1000]; //to print one file on format.(to use sprintf()funct)
	char color = 0;		//for print color text

	int i = 0;		//for loop
	int format = 0; //for print format(five by line)
	/////////////////////////////print files one by one////////////////////////////
	for (i = 0; i < count; i++)
	{
		lstat(dir[i]->d_name, &buf);				 //get a file info and save in buf.
		color = checkColor(buf);					 //what's file type color?
		if (Aopt == 0 && (dir[i]->d_name[0] == '.')) //none -a option not print hidden files..
			continue;								 //to thenext file
		if (Lopt == 0)								 //'-l' option turn off
		{
			if (color != 0)								//if file-type have color...
				printColor(plist, color);				//give text color
			sprintf(getitem, "%-15s ", dir[i]->d_name); //to print according to format format
			strcat(plist, getitem);						//to save plist
			if (color != 0)								//if file-type have color...
				printColor(plist, '0');					//return color white

			format++;			 //format variable increase
			if (format % 5 == 0) //by five..
			{
				format = 0;			 //set 0
				strcat(plist, "\n"); //enter line
			}
		}
		else //'-l' option turn on
		{
			pItemL(buf, plist, dir[i]->d_name); //print detail file information one by line
		}
	}
	/////////////////////////////end print files one by one////////////////////////////
	if(Lopt==0 && format != 0) strcat(plist, "\n");			//print enter to end.
	strcat(result,plist);
}

int NLSTfunc(char *ftpcommand, int mode)
{
	char *token;		 //tokenize ftpcommand
	char pf[100]={0,};
	char *path[100] = {0,};	 //to distinguish argument
	char result[10000]={0,};
	struct dirent **dir; //to get files in directory
	struct stat buf;	 //to get one file info.
	int argunum=0;
	int pathCount=0;

	int checkfold = 0; //if argument is fold?
	int checkerror=0;
	int count = 0;	   //file count in directory
	int Aopt = 0;	   //-a option or not
	int Lopt = 0;	   //-l option or not
	int i = 0;		   //for loop
	if (mode)		   //if command is 'LIST'
	{
		Aopt = 1; //set default option -al
		Lopt = 1;
	}

	strcpy(pf,"150 Opening data connection for directory list.\n");	//send data connection - for NLST	
	write(client_fd,pf,strlen(pf));
	write(STDOUT_FILENO,pf,strlen(pf));

	token = strtok(ftpcommand, " "); //to tokenize ftpcommand

	//////////////////////////////////analysis ftpcommand//////////////////////////
	while (token != NULL)
	{
		if (token[0] == '-') //the argument is option?
		{
			if (strcmp(token, "-") == 0) //but... only '-' is not option
			{
				path[argunum++] = token;			   //'-' save in 'path' as path
				token = strtok(NULL, " "); ////to the next step
				continue;				   ////to the next step
			}
			for (i = 1; i < strlen(token); i++) //option check one by one.
			{
				if (token[i] == 'a') //-a option here?
					Aopt = 1;
				else if (token[i] == 'l') //-l option here?
					Lopt = 1;
			}
		}
		else					   //the argument is file or directory?
			path[argunum++] = token;		   //set path
		token = strtok(NULL, " "); //to the next step
	}
	//////////////////////////////////end analysis ftpcommand//////////////////////////

	/////////////////////////////////analysis path///////////////////////////////////
	if(argunum==0) argunum++;
	while(pathCount<=argunum-1){
		checkfold =0;//if fold... this value is 'ON'
		if(path[0]!=NULL && path[1]!=NULL){	//if many argument from ls.... It must print information where path is.
			strcat(result,path[pathCount]);
			strcat(result,":\n");
		}
		if (path[0] == NULL) //only input command "NLST"
		{
			if ((count = scandir(".", &dir, NULL, alphasort)) == -1) //check working directory
			{
				strcpy(pf, "550 Failed transmission\n"); //can't open is error.
				checkerror=1;
			}
			else //success open current directory
			{
				checkfold = 1;						//set checkfold is on
				printItems(result,dir, count, Aopt, Lopt); //print files in directory
			}
		}
		else //input with path(file or directory)
		{
			if (strrchr(path[pathCount], '/') == NULL) //it is file in current dir...
			{
				if (lstat(path[pathCount], &buf) < 0) //check file
				{
					strcpy(pf, "550 Failed transmission\n"); //can't open is error.
					checkerror=1;
				}
				else //success to approach file
				{
					if(S_ISDIR(buf.st_mode)){
						strcpy(pf, "550 Failed transmission\n"); //can't open is error.
						checkerror=1;
					}
					else{
						pItemL(buf, result, path[pathCount]);		  //to save string file info detail
					}
				}
			}
			else if (strlen(strrchr(path[pathCount], '/')) == 1) //it's folder
			{
				if ((count = scandir(path[pathCount], &dir, NULL, alphasort)) == -1) //can't open dir
				{
					strcpy(pf, "550 Failed transmission\n"); //can't open is error.
					checkerror=1;
				}
				else
				{
					checkfold = 1;						//can open dir
					printItems(result,dir, count, Aopt, Lopt); //to print files info.
				}
			}
			else //it is file anywhere(to input ()./filename or /bin/filename and so on)..)...
			{
				if (lstat(path[pathCount], &buf) < 0) //can't open file
				{
					strcpy(pf, "550 Failed transmission\n"); //can't open is error.
					checkerror=1;
				}
				else //can open file
				{
					if(S_ISDIR(buf.st_mode)){
						strcpy(pf, "550 Failed transmission\n"); //can't open is error.
						checkerror=1;
					}
					else{
						pItemL(buf, result, path[pathCount]);		  //to save string file info detail
					}
				}
			}
		}
		/////////////////////////////////end analysis path///////////////////////////////////
		if (checkfold == 1) //if directory..
		{
			for (i = 0; i < count; i++) //free to memory directory and files in directory
			{							//free mem!
				free(dir[i]);
			}
			free(dir);
		}
		pathCount++;	//print another path!
		strcat(result,"\n");	//to next line...
	}
	if(!checkerror) strcpy(pf,"226 Complete transmission\n");
	write(datafd,result,10000);	//result send client
	write(client_fd,pf,100);	//result send client
	write(STDOUT_FILENO,pf,strlen(pf));	//result send client

	if(!checkerror){//not error send result string to client
		sprintf(pf,"OK. %d bytes is received\n",(int)strlen(result));
		write(datafd,pf,100);	//result send client
	}
	close(datafd);
}

void PWDfunc()
{
	char buf[100] = {
		0,
	};							//to save current directory
	char result[200];
	getcwd(buf, sizeof(buf));	//to save string in buf by getcwd function
	sprintf(result,"257 \"%s\" is current directory\n",buf);
	write(client_fd, result, strlen(result)); //print stdout
	write(STDOUT_FILENO, result, strlen(result)); //print stdout
}

void CWD(char *path, int mode)
{
	char result[100]={0,};
	path = strtok(path, " "); //to get path.
	//////////////////////////change working directory/////////////////////
	if (mode == 0)			  //input argument path
	{
		if (chdir(path) == -1) //fail chdir()
			sprintf(result, "550 %s. Can't file such file or directory\n",path); //save error
		else strcpy(result,"250 CWD command succeeds.\n");
	}
	else //input path is '..'
	{
		if (chdir("../") == -1) //if fail chdir()
			sprintf(result, "550 Can't file such file or directory\n"); //save error
		else strcpy(result,"250 CWD command succeeds.\n");
	}
	write(client_fd,result,strlen(result));	//if not success..send error message..
	write(STDOUT_FILENO,result,strlen(result));	//if not success..send error message..
}

void MKD(char *path)
{
	char result[100]={0,};
	char *foldname = strtok(path, " "); //to get foldname by path
	while (foldname != NULL)			//To create multiple folders
	{
		if (mkdir(foldname, 0777) != 0)												   //fail mkdir...
			sprintf(result, "550 %s. can't create directory\n",path); //to save error
		else{
			foldname = strtok(NULL, " ");
			strcpy(result,"250 MKD command performed successfully\n");
		}//to the next step
	}
	write(client_fd,result,sizeof(result));	//send message
}

void RMD(char *path)
{
	char *foldname = strtok(path, " "); //to get foldname by path
	char result[200]={0,};
	while (foldname != NULL)			//To delete multiple folders
	{
		if (rmdir(foldname) != 0)																			  //delete fail
			sprintf(result, "550 %s. can't create directory\n",path); //to save error
		else{
			foldname = strtok(NULL, " ");
			strcpy(result,"250 MKD command performed successfully\n");
		}
	}
	write(client_fd,result,sizeof(result));//send message
}

void DELE(char *path)
{
	struct stat buf;				 //check path is file
	char result[200]="x";
	char *token = strtok(path, " "); //tokenize path.
	while (token != NULL)			 //To delete  files
	{
		if (lstat(token, &buf) < 0) //none file...
		{
			sprintf(result, "550 %s. Can't fild such file or directory.\n",token); //save error
			break;
		}
		else if (S_ISDIR(buf.st_mode)){											 //but directory is for 'rmdir'
			sprintf(result, "550 %s. Can't fild such file or directory.\n",token); //save error
			break;
		}
		else																	 //correct path...
		{
			remove(token); //delete file
		}
		token = strtok(NULL, " "); //to the next step
	}
	if(strcmp(result,"x")==0) strcpy(result,"250 DELE command performed successfully.\n");
	write(client_fd,result,sizeof(result));//send message
	write(STDOUT_FILENO,result,sizeof(result));
}

void RNFR(char* path,char *oldfile)
{
	struct stat buf; //to get file or directory info
	char result[200]={0,};
	strstr(path," ")[0]='\0';
	strcpy(oldfile,path);
	if (lstat(oldfile, &buf) < 0) //none file or directory
		sprintf(result, "550 %s. Can't file such file or directory\n",oldfile); //save error
	else //success file or directory load
		strcpy(result, "350 File Exists, ready to rename\n"); //inform process RNFR success
	write(client_fd, result, strlen(result));			
	write(STDOUT_FILENO, result, strlen(result));							  //print stdout
}

void RNTO(char* newfile , char *oldfile)
{
	struct stat buf; //if directory, move working directory.
	char result[100]={0,};
	strstr(newfile," ")[0]='\0';

	if(lstat(newfile,&buf)>=0)
		sprintf(result, "550 %s. can't be renamed\n",newfile); 

	else if (rename(oldfile,newfile ) == -1) //fail rename...
		sprintf(result, "550 %s. can't be renamed\n",newfile); 
	else
		strcpy(result, "250 RNTO command succeeds\n"); //success message print.
	write(client_fd, result, strlen(result));
	write(STDOUT_FILENO, result, strlen(result));							  //print stdout
}

void sh_int(int signum){
	int i=0;
	if(pid[childNum-1]!=0){		//only parent proccess
		while(childNum!=0){		//send signal which is exit itself
			kill(pid[childNum-1],SIGUSR2);
		}
		while(wait(NULL)>0);	//until all child proccess exit....
		exit(0);			//prarent exit
	}else if(childNum==0) exit(0);		//only parent here... exit!
}

void sh_usr2(int signum){
	char buf[100]={0,};
	char pf[300]={0,};
	sprintf(pf,"Client(%d)'s Release\n\n",getpid());	//print info before exit client
	write(STDOUT_FILENO,pf,strlen(pf));
	close(client_fd);//close client connect
	exit(0);		//exit
}


void sh_chld(int signum){
	int i=0;
	int swapOn=0;
	int stat;
	pid_t chlpid = waitpid(-1,&stat,WNOHANG);	//which child send this signal?
	for(i=0;i<childNum;i++){		//delete this child pid
		if(pid[i]==chlpid) swapOn=1;
		if(swapOn) pid[i] = pid[i+1];
	}
	childNum--;		//childNum -1
	alarm(0);		//alarm reset
}

int IPAccessControl(){
	char buf[50]={0,},check[50]={0,};	//read file in buf, check IP
	char *token1[5]={0,},*token2[5]={0,};	//for tokenize - check and file
	char *tmp=NULL;						//for strtok_r argu.. dummy
	int ipValid = 0;				//result of Valid - 1 can access
	int i=0,j=0;				//for loop
	int minlen=0;				//result minlen
	FILE* fp_checkIP=NULL;		//save in FILE*

	fp_checkIP = fopen("access.txt","r");		//to get IPlist which is acceptible client IP 
	if(fp_checkIP==NULL){			//fail get file...
		return 431;
	}

	////////////////check IP is in file?/////////////////////
	while(fgets(buf,20,fp_checkIP)!=NULL){	//read one line in file until EOF
		ipValid = 1;						//vaild setting true...
		i=0;								//init i,j for loop
		j=0;
		strcpy(check,inet_ntoa(client_addr.sin_addr));//for tokenizing, save client IP as string
		token1[i++]=strtok_r(check,".",&tmp);		//tokenize client IP	
		while((token1[i++]=strtok_r(NULL,".",&tmp))!=NULL);

		token2[j++]=strtok_r(buf,".",&tmp);		//tokenize IP in file.	
		while((token2[j++]=strtok_r(NULL,".",&tmp))!=NULL);

		strstr(token2[3],"\n")[0] = '\0';		//end of line has '\n' so change this to \0.

		for(i=0;i<4;i++){						//cheking process
			minlen=MIN(strlen(token1[i]),strlen(token2[i]));	//get min IP length by part. (part is x.x.x.x -> x)
			if(strlen(token1[i])!=strlen(token2[i]) && (strstr(token2[i],"*")==NULL)){		//if two part lens are diffrent, it is diffrent IP.
				ipValid = 0;// set valid is FALSE
				break;		//alread check valid is FALSE, finish cheking IP process .
			}
			for(j=0;j<minlen;j++){	//until minlen...
				if(token1[i][j]==token2[i][j]) continue;//same character
				else if(token2[i][j]=='*')break;	//* is valid is not change...
				else{								//	diferrent character
					ipValid =0;
					break;
				}
			}
			if(!ipValid){							//if valid is 0 there is no need to check anymore...
				break;		//break look
			}
		}
		if(!ipValid){		//get next IP from file..
			continue;
		} else break;	//ipValid True... break!
	}
	if(ipValid){//inform access IP!
		return 220;
	}else{ //not accept....
		return 431;
	}
}

int userAuthentication(char *username){
	char getCommand[50]={0,};//get USER Command	
	char fp[100]={0,};//save string
	int count=0;//count login try number

	while(1){	//try to login
		count++;//count increase
		memset(getCommand,0,sizeof(getCommand));//initialize getcommand
		read(client_fd,getCommand,50);	//get USER command
		strtok(getCommand," ");			//tokenize that string	
		if(!user_match(username,strtok(NULL," "))){	//check USER - user_match()function
			strcpy(fp,"430 Invalid username or password\n");//fail is send message client
			write(client_fd,fp,strlen(fp));
			write(STDOUT_FILENO,fp,strlen(fp));
			if(count==3)//if try times is third..
				return 530;	//return reject state code
			continue; 
		}
		return 230;
	}

}
int user_match(char *username,char* user){	
	struct passwd *pw;		//get pw
	FILE *f;				//read passwd
	char readCommand[50]={0,};//read PASS command
	char fp[100]={0,};		//save string
	f = fopen("passwd","r");	//open passwd
	while((pw=fgetpwent(f))!=NULL){	//read EOF
		if(strcmp(user,pw->pw_name)==0) {	//if id correct..
			strcpy(fp,"331 Password is required for username\n");//send state client
			write(client_fd,fp,strlen(fp));	
			write(STDOUT_FILENO,fp,strlen(fp));

			read(client_fd,readCommand,50);	//read command PASS
			strtok(readCommand," ");		//tokeinze	
			if(strcmp(pw->pw_passwd,strtok(NULL," "))==0){	//compare passwd
				strcpy(username,user);		//save username!
				fclose(f);	//file close
				return 1;	
			}
			fclose(f);
			return 0;//if not match pass -> return - retry input id	
		}
	}
	fclose(f);
	return 0;//if not id match .... return fail	
}

void PORTcommand(char *arg){
	char pf[100]={0,};//save string
	char *token=NULL;//for tokenize ip,portnum 
	char temp[20]={0,};//save ip
	int n=0;	//for loop
	int port_num=0;//save port num
	token=strtok(arg,",");	//tokenize by ','
	strcat(temp,token);		//save temp - as ip
	strcat(temp,".");		//ip is tokenize by '.'

	for(n=0;n<3;n++){		//tokenize IP address and save temp
		token=strtok(NULL,",");
		strcat(temp,token);
		if(n!=2) strcat(temp,".");
	}

	token=strtok(NULL,",");	//tokenize port num
	port_num = atoi(token);
	port_num *= 256;		//to upper num
	token=strtok(NULL,"");
	port_num += atoi(token);//to down num

	datafd=socket(PF_INET,SOCK_STREAM,0);		//make data-connection socket
	if(datafd<0){	//socket()function fail.....
		strcpy(pf,"500 Failed to access\n");
	}

	memset(&dataServer,0,sizeof(dataServer));	//prepare data-connection
	dataServer.sin_family=AF_INET;
	dataServer.sin_port=htons(port_num);
	dataServer.sin_addr.s_addr=inet_addr(temp);		//input data-connection server information
	if(connect(datafd,(struct sockaddr*)&dataServer,sizeof(dataServer))>=0){	//connect data-server
		strcpy(pf,"200 Port Command successful\n");//return success
	}else {
		strcpy(pf,"500 Failed to access\n");//return success
	}
	write(client_fd,pf,strlen(pf));		//send result state client
	write(STDOUT_FILENO,pf,strlen(pf));
}

void TYPEcommand(char* arg){
	char fp[30]={0,};//save string
	strstr(arg," ")[0]='\0';//change ' ' to \0
	if(strcmp(arg,"A")==0) {//type ascii
		strcpy(fp,"201 Type set to A\n");
		curType = 1;//set 1
	}
	else if(strcmp(arg,"I")==0){//type binary
		strcpy(fp,"201 Type set to I\n");
		curType =0;//set 0
	}
	else strcpy(fp,"502 Type doesn't set.\n");//another is fail

	write(client_fd,fp,strlen(fp));
	write(STDOUT_FILENO,fp,strlen(fp));
}

void RETRcommand(char* path){
	FILE* f=0;//file structure-open file
	int buflen=0;//save read buf len
	int bufNum=0;//cur bufnum
	int totalbufNum=0;//total bufNum
	int sendByte=0;//data byte sent
	int fileSize=0;	//to get filesize
	char type[10]={0,};//convert curType - to string
	char readbuf[1024]={0,};//read filebuf

	strstr(path," ")[0]='\0';
	if(curType==1) strcpy(type,"ASCII");//convert curType - to string
	else strcpy(type,"BINARY");
	
	sprintf(readbuf,"150 Opening %s mode data connection for %s\n",type,path);//ready ls command
	write(client_fd,readbuf,strlen(readbuf));//send that string
	write(STDOUT_FILENO,readbuf,strlen(readbuf));
	f=fopen(path,"rb");	//open file - read only and binary

	if(f ==NULL){//fail file open...
		sprintf(readbuf,"550 Failed transmission\n");//send state code client
		write(client_fd,readbuf,strlen(readbuf));
		write(STDOUT_FILENO,readbuf,strlen(readbuf));
		close(datafd);
		return;
	}
	////////////////////////////////save file size and type////////////////////////////
	fseek(f,0,SEEK_END);
	fileSize = ftell(f);
	totalbufNum = fileSize/1024;
	fseek(f,0,SEEK_SET);	

	sprintf(readbuf,"%d %s",fileSize,type);	
	////////////////////////////////////end save////////////////////////////////////////
	write(datafd,readbuf,sizeof(readbuf));//send that information client

	read(datafd,readbuf,sizeof(readbuf));//received client 'OK' sign

	memset(readbuf,0,sizeof(readbuf));
	while(buflen=fread(readbuf,sizeof(char),sizeof(readbuf),f)){//until EOF
		if(buflen<=0) break;//read fail or EOF break!
		write(datafd,readbuf,buflen);//send data to client
		sendByte += buflen;//caculate total byte sent
		memset(readbuf,0,sizeof(readbuf));
	}

	fclose(f);//close file
	read(datafd,readbuf,sizeof(readbuf));//read OK signal from client'
	if(strcmp(readbuf,"OK")==0){	//if OK
		sprintf(readbuf,"226 Complete transmission\n");//result and state code send! to client
		write(client_fd,readbuf,strlen(readbuf));
		write(STDOUT_FILENO,readbuf,strlen(readbuf));

		sprintf(readbuf,"OK. %d bytes is received\n",sendByte);
		write(datafd,readbuf,strlen(readbuf));
		close(datafd);//close data connection!
	}	 
}


void STORcommand(char* newfile){
	FILE* f=0;//to save file info
	int readlen=0;//lenth of data received client
	int fileSize=0;	//filesize of client file 
	int totalbufNum=0;//total bufnum
	int bufferNum=0;//cur bufNum
	char buf[1024]={0,};//buf
	char type[10]={0,};//save type

	strstr(newfile," ")[0]='\0';
	if(curType==1) strcpy(type,"ASCII");//convert to type - string
	else strcpy(type,"BINARY");

	sprintf(buf,"150 Opening %s mode data connection for %s\n",type,newfile);//send ready STOR
	write(client_fd,buf,strlen(buf));
	write(STDOUT_FILENO,buf,sizeof(buf));
	memset(buf,0,sizeof(buf));

	read(datafd,buf,1024);//recieved filesize
	fileSize=atoi(buf);//convert int
	totalbufNum = fileSize/1024 + 1;	//caculate totalbufnum

	write(datafd,"OK",2);	//send OK sign

	if(curType==1)	//open file as curType
		f=fopen(newfile,"wt");
	else
		f=fopen(newfile,"wb");

	while(bufferNum != totalbufNum){//save data in new-file until totalbufNum
		readlen = read(datafd,buf,1024);
		bufferNum++;
		fwrite(buf,sizeof(char),readlen,f);
	}
	fclose(f);//close file

	sprintf(buf,"226 Complete transmission\n");//send OK
	write(client_fd,buf,strlen(buf));
	write(STDOUT_FILENO,buf,strlen(buf));

	sprintf(buf,"OK. %d bytes is received\n",fileSize);//send result data
	write(datafd,buf,strlen(buf));
	close(datafd);
}
