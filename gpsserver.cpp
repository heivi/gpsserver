#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <sstream>
#include <cstring>

// detached to thread, handles connection
void* SocketHandler(void*);

// parses the data off the tk102-2 data
int HandleGPSData(char* data);

// checks wether there is something to send back (commands)
std::string SomethingToSend();

// relays data to remote server
int SendTo(std::string data);

// cuts string to vector of strings
std::vector<std::string> CutString(std::string data, std::string delimiter);

int main(int argv, char** argc){
	
	// the port in which the server will listen
	int host_port= 8523;

	struct sockaddr_in my_addr;

	int hsock;
	int * p_int ;
	int err;

	struct timeval tv;

	socklen_t addr_size = 0;
	int* csock;
	sockaddr_in sadr;
	pthread_t thread_id=0;

START:

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n", errno);
		goto FINISH;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;

	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 ) ){
		printf("Error setting options %d\n", errno);
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;

	if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
		goto FINISH;
	}
	if(listen( hsock, 10) == -1 ){
		fprintf(stderr, "Error listening %d\n",errno);
		goto FINISH;
	}

	addr_size = sizeof(sockaddr_in);

	std::cout << "Server started in port " << host_port << "\n";

	while(true){
		//printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));
		if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1 || errno == 4){
			printf("---------------------\nReceived connection from %s\n",inet_ntoa(sadr.sin_addr));
			pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
			pthread_detach(thread_id);
		} else if (errno == 11) {
			// not so serious
			std::cerr << "11, continuing\n";
		} else {
			fprintf(stderr, "Error accepting %d\n", errno);
		}

	}

FINISH:
free(csock);
}

void* SocketHandler(void* lp){
    int *csock = (int*)lp;

	char buffer[8192];
	int buffer_len = 8192;
	int bytecount;
	unsigned long int = 0;

	struct timeval tv;
	tv.tv_sec = 90;
	tv.tv_usec = 90000;
	if (setsockopt(*csock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv)) {
		perror("setsockopt");
		goto FINISH;
	}

	while (true) {

		memset(buffer, 0, buffer_len);
		if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
			fprintf(stderr, "Error receiving data %d\n", errno);
			goto FINISH;
		}

		if (buffer[0] == '\0') {
			goto FINISH;
		}
		//printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);

		// send data to be analyzed
		if ( HandleGPSData(buffer) == 1) {
			printf("Data analyzed fine!\n");
		} else {
			printf("No real data!\n");
		}

		std::string tobesent = "";
		tobesent = SomethingToSend();

		// if first char is !, there is nothing to be sent...
		if ( tobesent.at(0) != '!' ) {
			std::cout << "Sending... \n";
			if((bytecount = send(*csock, tobesent.c_str(), tobesent.length(), 0))== -1){
				fprintf(stderr, "Error sending data %d\n", errno);

				goto FINISH;
			}
		}
		
		std::string readsd = "readsd123456 1";
		if (time(NULL) - sec > 10) {
			if((bytecount = send(*csock, readsd.c_str(), readsd.length(), 0))== -1){
                                fprintf(stderr, "Error asking readsd %d\n", errno);
                                
                                goto FINISH;
                        }
		}

	}
FINISH:

	shutdown(*csock, 2);
	close(*csock);
	free(csock);
    return 0;
}

std::string SomethingToSend() {
	using namespace std;

	ifstream commands;
	commands.open("commands.txt", ios::in);
	string command = "";
	 if (commands.is_open()) {
                if (commands.good()) {
                        getline(commands,command);
			if (command != "") {
				command.append("\r\n");
				cout << "Command to be sent: " << command;
			}
                }
                commands.close();
        } else {
                command = "";
        }

	ofstream commands2;
	commands2.open("commands.txt", ios::trunc);
	commands2.close();

	if (command != "") {
		return command;
	} else {
		return "!";
	}

}

int HandleGPSData(char* data) {
	using namespace std;
	time_t seconds = time(NULL);

	int dataok = 1;

	int timechange = 1136073600 - 2*3600;

	string sdata = string(data);
	ofstream tolog;
	tolog.open ("gps.log", ios::app);
	if (tolog.is_open()) {
		tolog << sdata << "\n";
		tolog.close();
	} else {
		cout << "Couldn't open gps.log!\n";
	}
	
	// sample data:
	//100522004739,+491726862569,GPRMC,224739.000,A,
	//5722.5915,N,02202.2062,E,0.00,0.00,210510,,,A*63,
	//F,, imei:724717032234285,09,470.5,F:3.78V,0,139,
	//17646,262,03,0151,D3FB,itakka_fw4.2
	
	
	// note in imeis.lst: tk102-2 has a space in front of text imei:...,
	// so follow the example data
	vector<string> imeidata;
	ifstream imeis ("imeis.lst");
	if (imeis.is_open()) {
		while (imeis.good()) {
			string temp;
			getline(imeis,temp);
			imeidata.push_back(temp);
		}
		imeis.close();
	} else {
		cout << "Couldn't open imeis.lst!\n";
	}
	
	// cut message by lines, if there happens to be more than one location
	vector<string> rows = CutString(sdata, "\r\n");
	
	for (int c = 0; c < rows.size(); c++) {
		string row = rows.at(c);
		
		if (row == "") {
			continue;
		}

		vector<string> results = CutString(row, ",");
	
		string time_utc = "";
		string lat = "";
		string latNS = "";
		string lon = "";
		string lonWE = "";
		//string speedknots = "";
		//string bearing = "";
		string date_utc = "";
		//string quality = "";
		string alarm = "";
		string imei = "";
		//string sats = "";
		//string alt = "";
		//string voltage = "";
		//string charging = "";
	
		if (results.size() > 21) {
		
			// just the information we need
			time_utc = results.at(3);
			lat = results.at(5);
			latNS = results.at(6);
			lon = results.at(7);
			lonWE = results.at(8);
			//speedknots = results.at(9);
			//bearing = results.at(10);
			date_utc = results.at(11);
			//quality = results.at(15);
			alarm = results.at(16);
			imei = results.at(17);
			//sats = results.at(18);
			//alt = results.at(19);
			//voltage = results.at(20);
			//charging = results.at(21);
			//...
	
		} else if (results.size() > 15 && results.at(3) == "L") {
			dataok = 0;
			//cout << "Not location data, but something!\n";
			imei = results.at(5);
			//voltage = results.at(8);
			//charging = results.at(9);
	
		} else {
			dataok = 0;
			//cout << "No real data!\n";
		}
	
		string id = "";
		for (int i = 0; i < imeidata.size(); i++) {
			if (imeidata.at(i).substr(0,(imeidata.at(i).find(","))) == imei) {
				id = imeidata.at(i).substr(imeidata.at(i).find(",")+1);
			}
		}
		string tofile = "";
		if (id == "") {
			id = imei;
			cout << "Couldn't find imei: �" << imei << "|\n";
		} else {
			cout << "id: |" << id << "| : ";
		}
		
		tofile.append(id);
		tofile.append(".");
		int aika = (int)seconds - timechange;
	
		int hour = 0;
		int min = 0;
		int sec = 0;
		int day = 0;
		int mon = 0;
		int year = 0;
	
		struct tm * timeinfo;
		time_t rawtime;
		time_t timesec = 0;
	
		// parse time
		if (time_utc.length() >= 6 && date_utc.length() >= 6) {
			hour = atoi(time_utc.substr(0,2).c_str());
			min = atoi(time_utc.substr(2,2).c_str());
			sec = atoi(time_utc.substr(4,2).c_str());
			day = atoi(date_utc.substr(0,2).c_str());
			mon = atoi(date_utc.substr(2,2).c_str());
			year = atoi(date_utc.substr(4,2).c_str());
			year = year + 2000 - 1900;
			mon = mon - 1;
			//hour = hour;
	
			time( &rawtime );
			timeinfo = gmtime( &rawtime );
			//timeinfo = localtime( &rawtime );
			timeinfo->tm_year = year;
			timeinfo->tm_mon = mon;
			timeinfo->tm_mday = day;
			timeinfo->tm_hour = hour;
			timeinfo->tm_min = min;
			timeinfo->tm_sec = sec;
	
			timesec = mktime(timeinfo);
	
		} else {
			dataok = 0;
			//cout << "No real time!\n";
		}
	
		string timee;
		stringstream temptime;
		int timesecint = 0;
	
		if (timesec == 0) {
			temptime << aika;
			timee = temptime.str();
			tofile.append(timee);
	
		} else {
			timesecint = (int)timesec - timechange;
			temptime << timesecint;
			timee = temptime.str();
			tofile.append(timee);
		}
	
		int lattofile = 0;
		int lontofile = 0;
	
		// parse coordinates to degrees and decimals
		if (lat.length() >= 2 && lon.length() >= 3) {

		string latdeg = lat.substr(0,2);
		string londeg = lon.substr(0,3);
		double latdec = (atof(lat.substr(2).c_str()))/60;
		double londec = (atof(lon.substr(3).c_str()))/60;
		double latf = (int)atof(latdeg.c_str()) + latdec;
		double lonf = (int)atof(londeg.c_str()) + londec;
		lattofile = latf*100000;
		lontofile = lonf*50000;
	
		} else {
			dataok = 0;
			//cout << "No real location!\n";
		}
	
		string latstr;
		stringstream templat;
		templat << lattofile;
		latstr = templat.str();
	
		string lonstr;
		stringstream templon;
		templon << lontofile;
		lonstr = templon.str();
	
		tofile.append("_");
		tofile.append(lonstr);
		tofile.append("_");
		tofile.append(latstr);
		if (alarm != "") {
			tofile.append("_H");
		}
		tofile.append(".");
	
		ofstream datalst;
		if (dataok) {
			// select the file you want your data to be written in (gpsseuranta.net format)
			datalst.open ("../gps/data.lst", ios::app);
			if (datalst.is_open()) {
				datalst << tofile << "\n";
				datalst.close();
				
				cout << tofile << " : ";
			} else {
				cout << "Couldn't write in data.lst!\n";
			}
	
			SendTo(tofile);
		} else {
			cout << "No real data, not written in data.lst!\n";
		}
	}
	
	return dataok;
}

int SendTo(std::string data) {

	using namespace std;
	
	// select destination server port, ip
	int host_port = 80;
	char host_name[] = "xx.xx.xx.xx";

	struct sockaddr_in my_addr;

	char buffer[8192];
	int bytecount;
	int buffer_len=0;

	int hsock;
	int * p_int;
	int err;

	string tosend;

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1) {
		cout << "Error initializing socket " << errno << "\n";
		goto FINISH;
	}


	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	if ( connect(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
		if ((err = errno) != EINPROGRESS) {
			cerr << "Error connecting socket " << errno << "\n";
			goto FINISH;
		}
	}

	buffer_len = 8192;
	// sending via GET-request, for example
	tosend = "GET /save.cgi?d="+data+" HTTP/1.1\r\nHost: xx.xx.xx.xx\r\n\r\n";
	strcpy(buffer, tosend.c_str());

	if( (bytecount=send(hsock, buffer, buffer_len, 0))== -1) {
		cerr << "Error sending data " << errno << "\n";
		goto FINISH;
	}

FINISH:
close(hsock);
return 0;
}

std::vector<std::string> CutString(std::string data, std::string delimiter) {
	using namespace std;
	
	vector<string> results;
	
	int cutAt;
	
	while((cutAt = data.find_first_of(delimiter)) != data.npos) {
	
		if (cutAt > 0) {
			results.push_back(data.substr(0,cutAt));
		} else if (cutAt == 0) {
			results.push_back("");
		}

		data = data.substr(cutAt+1);
	}
	if (data.length() > 0) {
		results.push_back(data);
	}
	
	return results;
}