/*
============================================================================================
File: server.cpp

This is used to provide data as JSON by accepting input data as JSON.

Authors:
Kasturi Rangan
Sateesh Reddy


Versions:
0.1 : 2021.06.01 - First release
0.2 : 2021.07.14 - setConfig and getConfig APIs queries optimized
============================================================================================
*/
#include <iostream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <pqxx/pqxx>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include "pstream.h"
#include <jsoncpp/json/json.h>
#include "config_file.h"
#include <sys/stat.h>

#define VERSION_NUMBER "1.1.2"
#define VERSION_COMMENT "first release"

using namespace std;
using namespace pqxx;

std::string current_date();
std::string current_time();
std::string now_time();
std::string today_date();
std::string get_timezoneinfo();
string process_getfields();
string process_setconfig(std::vector<std::string>,std::vector<std::string>);
string process_getconfig(std::vector<std::string>);
string process_fetchvalues(const char*,const char*,const char*,std::vector<std::string>,std::vector<std::string>);
string process_fetchtimestamps(const char*,const char*,std::vector<std::string>,std::vector<std::string>,std::vector<std::string>,std::vector<std::string>,std::vector<std::string>,std::vector<std::string>,std::vector<std::string>);
int CheckPassword(const char*, const char*);
string process_extractT0Csv(const char*,const char*);
string getutcfromlocalusingdb(const char*);
string fetch_dayWiseCounts(const char*,const char*);
string get_versions();
void write_version_to_file();
void clear();

std::string host_addr,db_port,db_uname,db_upass,db_database;
string strName, strKey, strComment, strArraySize;

void splitLine(string strLine){
	char *pStr;
	char *pch;
	string pFinalStr, pStr1, pStr2;
	
	
	pStr = &strLine[0];

	//Remove from the line all characters between { and } inclusive
	pch = strtok( pStr, "{");
	if(pch)
		pStr1= string(pch);
	else
		pStr1 = string("");
	
	pch = strtok(NULL, "}");
	pch = strtok(NULL, "}");
	if(pch)
		pStr2 = string(pch);
	else
		pStr2 = string("");
	
	pFinalStr = pStr1+pStr2;
		
	pStr = &pFinalStr[0];
	//The first part is the name
	pch = strtok( pStr, ":");
	if(pch)
		strName = string(pch);
	else
		strName = string("");

	//The sceond part is the key
	pch = strtok(NULL, "/");
	if(pch)
		strKey = string(pch);
	else
		strKey = string("");
	
	//Skip another '/' to get the comment
	pch = strtok(NULL, "/"); 
	if(pch)
		strComment = string(pch);
	else
		strComment = string("");

	strName.erase(remove(strName.begin(), strName.end(), ' '), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'\t'), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'\r'), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'"'), strName.end());

	strKey.erase(remove(strKey.begin(), strKey.end(), ' '), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'\t'), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'\r'), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'"'), strKey.end());
	
	strComment.erase(remove(strComment.begin(), strComment.end(),'\t'), strComment.end());
	strComment.erase(remove(strComment.begin(), strComment.end(),'\r'), strComment.end());
	
	//Remove characters from ':' in key, if they exist
	std::size_t found = strKey.find(":");
	if(found != std::string::npos){
		strKey.resize(found);
	}
	// Check if the key is an array
	found = strKey.find("Array");
	int iSize=0;
	
	if (found==std::string::npos)
		found = strKey.find("ARRAY");
	if (found!=std::string::npos){
		std::size_t iStart = strKey.find("..");
		std::size_t iEnd = strKey.find("]");
		if ( (iStart!=std::string::npos) && (iEnd!=std::string::npos) ){
			strArraySize = strKey.substr(iStart+2, iEnd-(iStart+2));
			//Now see if the array is of Reals, Ints, Bools ot Bytes
			found = strKey.find("REAL");
			if (found == std::string::npos)
				found = strKey.find("Real");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)*4;
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("INT");
			if (found == std::string::npos)
				found = strKey.find("Int");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)*2;
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("BYTE");
			if (found == std::string::npos)
				found = strKey.find("Byte");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize);
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("BOOL");
			if (found == std::string::npos)
				found = strKey.find("Bool");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)/8;
				strArraySize = std::to_string(iSize);
			}
		}
		else{
			strArraySize="0";
		}
		strKey = "Array";
	}
}

void print(std::vector <string> const &a) {
   std::cout << " The vector elements are : \n";

   for(int i=0; i < a.size(); i++)
   std::cout << a.at(i) << '\n';
}

int main(int argc, char *argv[])
{	
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }

    //version_info,below time shows compilation time in version_info.conf
  	write_version_to_file();

    int port = atoi(argv[1]);
	
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    int create_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(create_sock < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    int bindStatus = bind(create_sock, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
		
	int listen_sock = listen(create_sock, 100);
	if(listen_sock < 0)
	{
		cerr << "Could not listen on TCP socket" << endl;
		exit(0);
	}
	
	struct sockaddr_in listen_addr, client_addr;
	socklen_t addr_len = sizeof(listen_addr);
	socklen_t client_len = sizeof(client_addr);

	getsockname(create_sock, (struct sockaddr *) &listen_addr, &addr_len);

	std::vector<std::string> ln = {"HOST_ADDR","DB_PORT","DB_UNAME","DB_UPASS","DB_DATABASE"};
	std::ifstream f_in("/etc/dataloggerapi/dataloggerapi.conf");
	CFG::ReadFile(f_in, ln, host_addr,db_port,db_uname,db_upass,db_database);
	f_in.close();
	
	for (;;) 
	{
		int conn_fd = accept(create_sock, (struct sockaddr*) &client_addr, &client_len);
        string cdt = current_date() +" "+ current_time();
        cout << "\n Connection Opened from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
        cout << "Current Date Time" << cdt << endl;
		printf("Connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		if(fork() == 0) 
		{
			char msg[80480];
			for (;;) 
			{
				ssize_t num_bytes_received;
				num_bytes_received = recv(conn_fd, msg, sizeof(msg), 0);

				if (num_bytes_received == 0) 
				{
					//string cdt = current_date()+" "+current_time();
					cout << "\n Connection Closed from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
					cout << "Current Date Time" << cdt << endl;
					printf("Closing connection and exiting: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
					shutdown(conn_fd, SHUT_WR);
					close(conn_fd);
					exit(0);
				}
				
				// Start - Reading from json
				Json::CharReaderBuilder b;
				Json::CharReader* reader(b.newCharReader());
				Json::Value root;
				
				JSONCPP_STRING errs;
				bool ok = reader->parse(msg, msg + strlen(msg), &root, &errs);

				std::string endpointip;
				//std::string unameip;
				//std::string passcodeip;
				if (ok && errs.size() == 0)
				{
					endpointip = root[0]["endpoint"].asString();
					//unameip = root[0]["username"].asString();
					//passcodeip = root[0]["passcode"].asString();
				}
				else
				{
					//string cdt = current_date()+" "+current_time();
					cout << "Current Date Time" << cdt << endl;
					cout << endl << "ok=" << ok << ", errs.size()=" << errs.size() << endl;
					shutdown(conn_fd, SHUT_WR);
					close(conn_fd);					
					cout << "\n Connection Closed from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
					exit(0);
				}
				delete reader;
				cout << "Endpoint: " << endpointip << endl;
				// End - Reading from json  
				string data;

				int checkAuth = 0;
				//int checkAuth = CheckPassword(unameip.c_str(),passcodeip.c_str());
				//cout << "checkAuth: " << checkAuth << endl;
				Json::Value resultValue;
				Json::StreamWriterBuilder wbuilder;
				if(true){
				std::vector<string>::iterator it;
				std::vector<string> endpointarr {"getFields", "getConfig", "setConfig","fetchValues","fetchTimestamps","getFieldsSlow","extractToCsv","getservertimezoneInfo","fetchdaywisecounts","get_versions"};
				std::vector<std::string> paramsnamearr;
				std::vector<std::string> paramsopsarr;
				std::vector<std::string> paramsvalarr;
				std::vector<std::string> paramstruearr;
				std::vector<std::string> paramsfalsearr;
				std::vector<std::string> paramsrisearr;
				std::vector<std::string> paramsfallarr;
				std::vector<std::string> paramsalarmarr;
				std::vector<std::string> paramslineitems;				
				if (it != endpointarr.end())
				{
					//string cdt = current_date()+" "+current_time();
					int sz = root[0]["params"].size();
					if(endpointip == endpointarr[0])
					{
						cout << "\n getFields API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						data = process_getfields();
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[1])
					{
						cout << "\n getConfig API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						for ( int index = 0; index < sz; ++index )
						{
							paramsnamearr.push_back(root[0]["params"][index]["name"].asString());
						}
						data = process_getconfig(paramsnamearr);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[2])
					{
						cout << "\n setConfig API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						for ( int index = 0; index < sz; ++index )
						{
							paramsnamearr.push_back(root[0]["params"][index]["name"].asString());
							paramsvalarr.push_back(root[0]["params"][index]["value"].asString());
						}
						data = process_setconfig(paramsnamearr,paramsvalarr);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[3])
					{
						cout << "\n fetchValues API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						string startt; string endt; string sno;
						startt = root[0]["params"][0]["start_time"].asString();
						const char* startT = startt.c_str();
						endt = root[0]["params"][0]["end_time"].asString();
						const char* endT = endt.c_str();
						sno = root[0]["params"][0]["skip"].asString();
						const char* sNo = sno.c_str();
						int mn = root[0]["params"][0]["param_names"].size();
						int pq = root[0]["params"][0]["param_alarms"].size();
						for ( int index = 0; index < mn; ++index )
						{
							cout << root[0]["params"][0]["param_names"][index]<< endl;
							paramsnamearr.push_back(root[0]["params"][0]["param_names"][index].asString());
						}
						for ( int index = 0; index < pq; ++index )
						{
							cout << root[0]["params"][0]["param_alarms"][index]<< endl;
							paramsalarmarr.push_back(root[0]["params"][0]["param_alarms"][index].asString());
						}
						data = process_fetchvalues(startT,endT,sNo,paramsnamearr,paramsalarmarr);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[4])
					{
						cout << "\n fetchTimestamps API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						string startt; string endt;
						startt = root[0]["params"][0]["start_time"].asString();
						endt = root[0]["params"][0]["end_time"].asString();
						const char* endT = endt.c_str();
						const char* startT = startt.c_str();
						int p = root[0]["params"][0]["param_names"].size();
						if(p>0){
							for ( int index = 0; index < p; ++index )
							{
								//cout << root[0]["params"][0]["param_names"][index]<< endl;
								paramsnamearr.push_back(root[0]["params"][0]["param_names"][index]["name"].asString());
								paramsopsarr.push_back(root[0]["params"][0]["param_names"][index]["operation"].asString());
								paramsvalarr.push_back(root[0]["params"][0]["param_names"][index]["value"].asString());
							}
					    }
						
						int q = root[0]["params"][0]["boolTrue"].size();
						for ( int index = 0; index < q; ++index )
						{
							//cout << root[0]["params"][0]["boolTrue"][index]<< endl;
							paramstruearr.push_back(root[0]["params"][0]["boolTrue"][index].asString());
						}
						
						int r = root[0]["params"][0]["boolFalse"].size();
						for ( int index = 0; index < r; ++index )
						{
							//cout << root[0]["params"][0]["boolFalse"][index]<< endl;
							paramsfalsearr.push_back(root[0]["params"][0]["boolFalse"][index].asString());
						}
						
						int m = root[0]["params"][0]["rising"].size();
						for ( int index = 0; index < m; ++index )
						{
							//cout << root[0]["params"][0]["rising"][index]<< endl;
							paramsrisearr.push_back(root[0]["params"][0]["rising"][index].asString());
						}
						
						int n = root[0]["params"][0]["falling"].size();
						for ( int index = 0; index < n; ++index )
						{
							//cout << root[0]["params"][0]["falling"][index]<< endl;
							paramsfallarr.push_back(root[0]["params"][0]["falling"][index].asString());
						}
						
						//string path; string pname; 
						data=process_fetchtimestamps(startT,endT,paramsnamearr,paramsopsarr,paramsvalarr,paramstruearr,paramsfalsearr,paramsrisearr,paramsfallarr);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[5])
					{
						data = process_getfields();
						//send(conn_fd, data.c_str(), data.length(), 0);
						int bufferSize = 1000;
						int messageLength = data.length(); // or whatever
						int sendPosition = 0;
						  
						while (messageLength) {
						    int chunkSize = messageLength > bufferSize ? bufferSize : messageLength;
						    memcpy(msg, data.c_str() + sendPosition, chunkSize);
						    chunkSize = send(conn_fd, msg, chunkSize, 0);
						    messageLength -= chunkSize;
						    sendPosition += chunkSize;
						    sleep(10);
						 }
					}
					if(endpointip == endpointarr[6])
					{
						string startTime; string endTime; 
						startTime = root[0]["start_time"].asString();
						endTime = root[0]["end_time"].asString();
						const char* startT = startTime.c_str();
						const char* endT = endTime.c_str();
						data = process_extractT0Csv(startT,endT);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[7])
					{
					   	string data = get_timezoneinfo();			
					   	send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[8])
					{
					   	string sdate,edate;
						sdate = root[0]["start_date"].asString();	
						edate = root[0]["end_date"].asString();
					   	string data = fetch_dayWiseCounts(sdate.c_str(),edate.c_str());			
					   	send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[9])
					{
						cout << "\n get_versions API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						data = get_versions();
						send(conn_fd, data.c_str(), data.length(), 0);
					}
				}
				else
				{
					cout << "Wrong endpoint" << endl;
					std::string output = "Wrong endpoint";
					data = output;
				}
				shutdown(conn_fd, SHUT_RDWR);
                close(conn_fd);
                exit(0);
			  }
			  /*else{
			  	cout << "Inside checkAuth: " << checkAuth << endl;
				//std::string output;
				Json::Value jVal1, jVal2;
					
				jVal1["result"] = "Fail";
				jVal2["reason"] = "Invalid Credentials";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				data  = Json::writeString(wbuilder, resultValue); 
				send(conn_fd, data.c_str(), data.length(), 0);
			  }*/
			  //send(conn_fd, data.c_str(), data.length(), 0);
			  //printf("Sent: %d bytes\n", bytes_sent);
			}
		}
		else 
		{
			close(conn_fd);
			signal(SIGCHLD, SIG_IGN);
		}
	}
	return 0;
}


/*
----------------------------------------------------------------------------------------
This function gets called when the getFields API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return the parameterNames from database as JOSN format.
-------------------------------------------------------------------------------------------
*/

string process_getfields()
{
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	string qrySelect = std::string("");
	const char * sql;
	//=========new=start===============
	string qrySelect1 = std::string("");
	const char * sql1;
	qrySelect1 += "SELECT alarm_id,db_field_name,bit_number FROM alarm_defs order by alarm_id";
	sql1 = qrySelect1.c_str();
    //=========new=end=================
	qrySelect += "SELECT parameter_name,parameter_data_type,number_of_bytes,scaling_factor,comment,field_name,bit_number,off_set from field_defs where parameter_data_type != 'Packed' order by param_serial, bit_number";
	sql = qrySelect.c_str();
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	try {
		connection C(strConnection);
		if (!C.is_open()) {
			std::string output;
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( sql ));
		//=========new=start===============
		result R1( N.exec( sql1));
        //=========new=end=================
		for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
		{
			Json::Value newValue;
			newValue["paramName"] = c[0].as<string>();
			newValue["paramType"] = c[1].as<string>();
			newValue["scalingFactor"] = c[3].as<string>();
			string cmt = c[4].as<string>();
			int cmtlen = cmt.size();
			if(cmtlen > 0){
				newValue["cmt"] = cmt;
			}
			else{
				newValue["cmt"] = "NA";
			}
			newValue["fieldName"] = c[5].as<string>();
			newValue["bitNumber"] = c[6].as<string>();
            newValue["offSet"] = c[7].as<string>();
			resultValue["field"].append(newValue);
		}
		for (result::const_iterator c1 = R1.begin(); c1 != R1.end(); ++c1) 
		{
			Json::Value newValue1;
			//string cmt1 = c1[0].as<string>();
			int cmtlen2 = c1[0].size();
			if(cmtlen2 > 0){
				newValue1["alarm_id"] = c1[0].as<string>();
			}
			else{
				newValue1["alarm_id"] = "NA";
			}
			
			int cmtlen4 = c1[1].size();
			if(cmtlen4 > 0){
				newValue1["db_field_name"] = c1[1].as<string>();
			}
			else{
				newValue1["db_field_name"] = "NA";
			}
			
			
			int cmtlen5 = c1[2].size();
			if(cmtlen5 > 0){
				newValue1["bitNumber"] = c1[2].as<string>();
			}
			else{
				newValue1["bitNumber"] = "NA";
			}
			newValue1["type"] = "BOOL";
			resultValue["alarm"].append(newValue1);
		}
		C.disconnect ();
	} 
	catch (const std::exception &e)
	{
		Json::Value jVal1, jVal2;
		
		jVal1["result"] = "Fail";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}
	string cdt = get_timezoneinfo();	
	Json::Value newValue2;		
	newValue2["value"] = cdt.c_str();
	resultValue["servertimezone"].append(newValue2);
	std::string output;
	output  = Json::writeString(wbuilder, resultValue);
	return output;
}

/*
----------------------------------------------------------------------------------------
This function gets called when the getConfig API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return the configuration settings from database as JOSN format.
-------------------------------------------------------------------------------------------
*/

string process_getconfig(std::vector<std::string>  iparr)
{
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	
	string paramStr="";
	for(int i=0; i < iparr.size(); i++)
	{
		paramStr += "'" + iparr[i] + "', ";
	}
	paramStr = paramStr.substr(0, paramStr.length()-2);

	string qrySelect = std::string("");
	const char * sql;
	qrySelect += "SELECT config_name,config_value from tbl_config where config_name IN ("+paramStr+")";
	sql = qrySelect.c_str();
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	try {
		connection C(strConnection);
		if (!C.is_open()) {
			std::string output;
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( sql ));
		for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
			Json::Value newValue;
			newValue["name"] = c[0].as<string>();
			string val = c[1].as<string>();
			int vallen = val.size();
			if(vallen > 0){
				newValue["value"] = val;
			}
			else{
				newValue["value"] = "NA";
			}
			resultValue["field"].append(newValue);
		}

		C.disconnect ();
	} 
	catch (const std::exception &e) 
	{
		Json::Value jVal1, jVal2;
		
		jVal1["result"] = "Fail";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}
	std::string output;
	output  = Json::writeString(wbuilder, resultValue);  
	return output;
}

/*
----------------------------------------------------------------------------------------
This function gets called when the getConfig API was invoked
This function insert the data into database if the configuration setting was not exist
This function update the data in database if the configuration setting was already exists
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to store the configuration settings in database.
-------------------------------------------------------------------------------------------
*/

string process_setconfig(std::vector<std::string>  ipnamearr,std::vector<std::string>  ipvalarr)
{
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	const char * upsql;  const char * insql;

	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	for(int i=0; i < ipnamearr.size(); i++)
	{
		Json::Value newValue;
		newValue["name"] = ipnamearr[i];
		string qrySelect = std::string("");
		const char * sql;
		qrySelect += "SELECT config_value from tbl_config where config_name = '"+ipnamearr[i]+"'";
		sql = qrySelect.c_str();
		try {
			connection C(strConnection);
			if (!C.is_open()) {
				std::string output;
				Json::Value jVal1, jVal2;
				
				jVal1["result"] = "Fail";
				jVal2["reason"] = "Could not open database";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);  
				return output;
			}
			nontransaction N(C);
			result R( N.exec( sql ));
			int resSize = R.size();
			N.commit();
			if(resSize > 0){
				work W(C);
				string upsqlstr = std::string("");
				upsqlstr = "UPDATE tbl_config set config_value = '"+ipvalarr[i]+"' where config_name = '"+ipnamearr[i]+"'";
				upsql = upsqlstr.c_str();
				W.exec( upsql );
				W.commit();
				newValue["status"] = "SAVED";
			}
			else
			{
				work W(C);
				string insqlstr = std::string("");
				insqlstr = "INSERT into tbl_config (config_name,config_value)  values ( '"+ipnamearr[i]+"','"+ipvalarr[i]+"')";
				insql = insqlstr.c_str();
				W.exec( insql );
				W.commit();
				newValue["status"] = "CREATED";
			}
			
			C.disconnect ();
		}
		catch (const std::exception &e) 
		{
			Json::Value jVal1, jVal2;
		
			jVal1["result"] = "Fail";
			jVal2["reason"] = e.what();
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
		}
		resultValue["field"].append(newValue);
	}
	
	std::string output;
	output  = Json::writeString(wbuilder, resultValue);  
	return output;
}

/*
----------------------------------------------------------------------------------------
This function gets called when the fetchValues API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return the matched values between the given datetimes as JOSN format.
-------------------------------------------------------------------------------------------
*/

string process_fetchvalues(const char* startt,const char* endt,const char* samples,std::vector<std::string> ipnamearr,std::vector<std::string> ipalarmarr)
{
	std::string output;
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	string paramStr="", fieldStr="", fieldDefs="";
	//const char * factorsql;
	std::vector<std::string> strRetParamNames;
	std::vector<std::string> strRetAlarmNames;	 
	std::string startT = startt;
	std::string endT = endt;
	std::string utcstartT = getutcfromlocalusingdb(startt);
	std::string utcendT = getutcfromlocalusingdb(endt);
	std::string sNo = samples;


	for(int i=0; i < ipnamearr.size(); i++)
	{
		paramStr += "last(\""+ipnamearr[i]+"\",insert_time_stamp) as \""+ipnamearr[i]+"\", ";
		fieldDefs += "'" + ipnamearr[i] + "', ";
	}
	paramStr = paramStr.substr(0, paramStr.length()-2);
	fieldDefs = fieldDefs.substr(0, fieldDefs.length()-2);

	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	if(ipnamearr.size() > 0){
		const char * factorsql;
		string factorSelect = "SELECT parameter_name, parameter_data_type, field_name,  bit_mask FROM field_defs WHERE parameter_name IN (" + fieldDefs +");";
		factorsql = factorSelect.c_str();

		try 
		{
			connection C(strConnection);
			if (!C.is_open()) 
			{
				//std::string output;
				Json::Value jVal1, jVal2;
				
				jVal1["result"] = "Fail1";
				jVal2["reason"] = "Could not open database";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);  
				return output;
			}
			nontransaction N(C);
			result R( N.exec( factorsql ));
			int resSize = R.size();
			if(resSize>0)
			{
				for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
				{
					if( c[1].as<string>() == "BOOL")
					{
						fieldStr += "last( (\"" + c[2].as<string>() +"\" & " + c[3].as<string>() + ")/" + c[3].as<string>() + ",insert_time_stamp)  AS \"" + c[0].as<string>() + "\", ";
					}
					else
					{
						fieldStr += "last(\""+c[2].as<string>() +"\",insert_time_stamp) as \""+c[2].as<string>()+"\", ";
					}
					strRetParamNames.push_back(c[0].as<string>());
				}
				fieldStr = fieldStr.substr(0, fieldStr.length()-2);
			}
			N.commit();
			C.disconnect ();
		}
		catch (const std::exception &e) 
		{
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail2";
			jVal2["reason"] = e.what();
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
		}
	}
	string qrySelect = std::string("");
	const char * sql;
	if(ipnamearr.size() > 0){
		ipnamearr.insert(ipnamearr.begin(), "x");
		strRetParamNames.insert(strRetParamNames.begin(), "x");
		qrySelect = "SELECT max(insert_time_stamp AT TIME ZONE 'utc' at time zone current_setting('TimeZone')) AS x,"+fieldStr+",time_bucket_gapfill( '"+sNo+" milliseconds', insert_time_stamp AT TIME ZONE 'utc' at time zone current_setting('TimeZone'), '"+startT+"','"+endT+"') AS y FROM data_view WHERE insert_time_stamp >= '"+utcstartT+"' AND insert_time_stamp <= '"+utcendT+"' GROUP BY y ORDER BY y;";
		sql = qrySelect.c_str();
		//cout << qrySelect;
		try 
		{
			connection C(strConnection);
			if (!C.is_open()) {
				//std::string output;
				Json::Value jVal1, jVal2;
				
				jVal1["result"] = "Fail5";
				jVal2["reason"] = "Could not open database";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);  
				return output;
			}
			nontransaction N(C);
			result R( N.exec( sql ));
			int resSize = R.size();
		//	int m = 0;
			int lastRow = resSize - 1;
			if(resSize > 0)
			{
				int m = 0;
				for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
				{
					Json::Value colValue;
					for(int i=0; i < strRetParamNames.size(); i++)
					{
						string paramName = strRetParamNames[i];
						if(c[i].is_null())
						{
							if( (m==0) || (m==lastRow) )
								colValue = "0";
							else
								colValue = "NAN";
						}
						else
						{
							colValue = c[i].as<string>();
						}
						resultValue[""+paramName+""].append(colValue);
					}
					m++;
				}
			}
			N.commit();
			C.disconnect ();
		} 
		catch (const std::exception &e) 
		{
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail6";
			jVal2["reason"] = e.what();
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
		}
	}
	if(ipalarmarr.size() > 0){
		//int lastRes = resultValue["x"].size();
		//lastRes = lastRes - 1;
		//string lmt  std::to_string(lastRes);
		strRetAlarmNames.insert(strRetAlarmNames.begin(), "x");
		try{
			connection C(strConnection);
			if (!C.is_open()) {
				//std::string output;
				Json::Value jVal1, jVal2;
				
				jVal1["result"] = "Fail3";
				jVal2["reason"] = "Could not open database";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);  
				return output;
			}
			string alarmStr="";
			for(int i=0; i < ipalarmarr.size(); i++)
			{
				string alarmId = ipalarmarr[i];
				string colName = "";
				string bitMask = "";
				qrySelect = "select db_field_name,bit_number from alarm_defs where alarm_id="+alarmId+"";
				//cout << qrySelect << endl;
				sql = qrySelect.c_str();
				nontransaction N(C);
				result R( N.exec( sql ));
				int resSize = R.size();
				if(resSize > 0)
				{
					for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
					{
						colName = c[0].as<string>();
						bitMask = c[1].as<string>();
					}
				}
				N.commit();
				strRetAlarmNames.push_back(alarmId);
				alarmStr += "last((\""+colName+"\" & (1<< "+bitMask+"))/(1<<"+bitMask+"),insert_time_stamp) as \""+alarmId+"\",";
			}
			ipalarmarr.insert(ipalarmarr.begin(), "x");
			alarmStr.pop_back();
		    //string colValue1 = "";
		    //string colValue2 = "";
			qrySelect = "select max(insert_time_stamp AT TIME ZONE 'utc' at time zone current_setting('TimeZone')) AS x,"+alarmStr+",time_bucket_gapfill( '"+sNo+" milliseconds', insert_time_stamp AT TIME ZONE 'utc' at time zone current_setting('TimeZone'), '"+startT+"','"+endT+"') AS y from alarm_table WHERE insert_time_stamp >= '"+utcstartT+"' AND insert_time_stamp <= '"+utcendT+"' GROUP BY y ORDER BY y;";
			cout << qrySelect << endl;
			sql = qrySelect.c_str();
			nontransaction M(C);
			result T( M.exec( sql ));
			int adSize = T.size();
		    //int m = 0;
			int lastRow = adSize - 1;
			if(adSize > 0)
			{
				int m = 0;
				for (result::const_iterator c = T.begin()+1; c != T.end(); ++c) 
				{
					Json::Value colValue;
					int counter = 0;
					if(ipnamearr.size() > 0){
						counter = 1;
					}
					for(int i=counter; i < strRetAlarmNames.size(); i++)
					{
						string paramName = strRetAlarmNames[i];
						if(c[i].is_null())
						{
							if( (m==0) || (m==lastRow) )
								colValue = "0";
							else
								colValue = "NAN";
						}
						else
						{
							colValue = c[i].as<string>();
						}
						resultValue[""+paramName+""].append(colValue);
					}
					m++;
				}
			}
			M.commit();
			C.disconnect ();
		}
		catch (const std::exception &e) 
		{
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail4";
			jVal2["reason"] = e.what();
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
		}
	}
	//cout << "2 size" << resultValue["2"].size();
	output  = Json::writeString(wbuilder, resultValue); 
	return output;
}
/*
----------------------------------------------------------------------------------------
This function gets called when the fetchtimeStamps API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return the matched timestamps for the given criteria as JOSN format.
-------------------------------------------------------------------------------------------
*/
string process_fetchtimestamps(const char* startt,const char* endt,std::vector<std::string> ipnamearr,std::vector<std::string> ipopsarr,std::vector<std::string> ipvalarr,std::vector<std::string> iptruearr,std::vector<std::string> ipfalsearr,std::vector<std::string> iprisearr,std::vector<std::string> ipfallarr)
{	
	std::string startT = startt;
	std::string endT = endt;
	std::string utcstartT = getutcfromlocalusingdb(startt);
	std::string utcendT = getutcfromlocalusingdb(endt);
	std::string output;
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");

	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;

	std::vector<string> truerisearr;
	truerisearr.reserve(iptruearr.size() + iprisearr.size());
	truerisearr.insert(truerisearr.end(), iptruearr.begin(), iptruearr.end());
	truerisearr.insert(truerisearr.end(), iprisearr.begin(), iprisearr.end());
	truerisearr.erase(unique(truerisearr.begin(), truerisearr.end()), truerisearr.end());
	
	std::vector<string> falsefallarr;
	falsefallarr.reserve(ipfalsearr.size() + ipfallarr.size());
	falsefallarr.insert(falsefallarr.end(), ipfalsearr.begin(), ipfalsearr.end());
	falsefallarr.insert(falsefallarr.end(), ipfallarr.begin(), ipfallarr.end());
	falsefallarr.erase(unique(falsefallarr.begin(), falsefallarr.end()), falsefallarr.end());

	//std::vector<string> fieldNamesList;
	//std::vector<string> bitMaskList;
	string factorSelect = std::string("");
	const char * factorsql;
	
	string paramStr0 = "SELECT insert_time_stamp ";
	int colSize = 1;
	if(ipnamearr.size() > 0)
	{
		colSize++;
		paramStr0 += ", val1, prev1 ";
	}
	if(ipnamearr.size() > 1)
	{
		colSize++;
		paramStr0 += ", val2, prev2 ";
	}
	if(truerisearr.size() > 0)
	{
	    for(int i=1; i <= truerisearr.size(); i++){
		colSize++;
		paramStr0 += ", rising"+std::to_string(i)+", prevRising"+std::to_string(i);
	    }
	}
	if(falsefallarr.size() > 0)
	{
	    for(int i=1; i <= falsefallarr.size(); i++){
		colSize++;
		paramStr0 += ", falling"+std::to_string(i)+", prevFalling"+std::to_string(i);
	    }
	}
	paramStr0 += "\n";
	
	string paramStr1 = "SELECT  insert_time_stamp AT TIME ZONE 'utc' at time zone current_setting('TimeZone') as insert_time_stamp";
	
	string paramStr2 = "((";
	if(ipnamearr.size() > 0){
        paramStr1 += ", \""+ipnamearr[0]+"\" as val1, LAG(\""+ipnamearr[0]+"\") OVER (ORDER BY insert_time_stamp) as prev1";
	    paramStr2 += "val1 "+ipopsarr[0]+" "+ipvalarr[0]+"";
	}

	if(ipnamearr.size()>1)
	{
		paramStr1 += ", \""+ipnamearr[1]+"\" as val2, LAG(\""+ipnamearr[1]+"\") OVER (ORDER BY insert_time_stamp) as prev2";
		paramStr2 += " AND val2 "+ipopsarr[1]+" "+ipvalarr[1]+"";
	}

	if(truerisearr.size()>0){
	  for(int i=1; i <= truerisearr.size(); i++){
	  	if(i == 1 && ipnamearr.size() == 0){
			paramStr2 += " rising"+std::to_string(i)+">0";
	  	}
	  	else{
			paramStr2 += " AND rising"+std::to_string(i)+">0";
	  	}
	  }
	}
	if(falsefallarr.size()>0){
	  for(int i=1; i <= falsefallarr.size(); i++){
	  	if(i == 1 && truerisearr.size() == 0 && ipnamearr.size() == 0){
	  		paramStr2 += " falling"+std::to_string(i)+"=0";
	  	}
	  	else{
	  		paramStr2 += " AND falling"+std::to_string(i)+"=0";
	  	}
	  }
	}

	paramStr2 += ")";

	paramStr2 += " AND (NOT (";
	if(ipnamearr.size() > 0){
	    paramStr2 += "prev1 "+ipopsarr[0]+" "+ipvalarr[0]+"";
	}

	if(ipnamearr.size()>1)
	{
		paramStr2 += " AND prev2 "+ipopsarr[1]+" "+ipvalarr[1]+"";
	}

	if(truerisearr.size()>0){
	  for(int i=1; i <= truerisearr.size(); i++){
	  	if(i == 1 && ipnamearr.size() == 0){
	  		paramStr2 += " prevRising"+std::to_string(i)+">0";
	  	}
	  	else{
	  		paramStr2 += " AND prevRising"+std::to_string(i)+">0";
	  	}
	  }
	}
	if(falsefallarr.size()>0){
	  for(int i=1; i <= falsefallarr.size(); i++){
	  	if(i == 1 && truerisearr.size() == 0 && ipnamearr.size() == 0){
	  		paramStr2 += " prevFalling"+std::to_string(i)+"=0";
	  	}
	  	else{
	  		paramStr2 += " AND prevFalling"+std::to_string(i)+"=0";
	  	}
	  }
	}
	paramStr2 += ")))";
	
	if(truerisearr.size()>0)
	{
		for(int i=1; i <= truerisearr.size(); i++)
		{
			factorSelect = "SELECT field_name, bit_mask FROM field_defs WHERE parameter_name='"+ truerisearr[i-1]+"'";
			//factorSelect += "'" + iprisearr[i] +"', ";		
			
			//factorSelect = factorSelect.substr(0, factorSelect.length()-2);
			//factorSelect += ");";
			factorsql = factorSelect.c_str();	
		
			try 
			{
				connection C(strConnection);
				if (!C.is_open()) 
				{
					//std::string output;
					Json::Value jVal1, jVal2;
				
					jVal1["result"] = "Fail";
					jVal2["reason"] = "Could not open database";
					resultValue["field"].append(jVal1);
					resultValue["field"].append(jVal2);
					output  = Json::writeString(wbuilder, resultValue);  
					return output;
				}
				nontransaction N(C);
				result R( N.exec( factorsql ));
				int resSize = R.size();
				if(resSize>0)
				{
					string risesubQry = "";
					for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
					{
			            risesubQry += "\"" + c[0].as<string>() +"\" & " + c[1].as<string>() + "";
			            paramStr1 += ","+risesubQry+" AS rising"+std::to_string(i)+"";
						paramStr1 += ",LAG("+risesubQry+") OVER (ORDER BY insert_time_stamp) AS prevRising"+std::to_string(i)+"";	
					}
				}
				N.commit();
				C.disconnect ();
			}
			catch (const std::exception &e) 
			{
				Json::Value jVal1, jVal2, jVal3;
			
				jVal1["result"] = "Fail1";
				jVal2["reason"] = e.what();
				jVal3["query"] = factorsql;
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				resultValue["field"].append(jVal3);
			}
	    }
	}
	if(falsefallarr.size()>0)
	{
		for(int i=1; i <= falsefallarr.size(); i++)
		{
		   factorSelect = "SELECT field_name, bit_mask FROM field_defs WHERE parameter_name='"+ falsefallarr[i-1]+"'";		
			factorsql = factorSelect.c_str();	
		
			try 
			{
				connection C(strConnection);
				if (!C.is_open()) 
				{
					//std::string output;
					Json::Value jVal1, jVal2;
				
					jVal1["result"] = "Fail";
					jVal2["reason"] = "Could not open database";
					resultValue["field"].append(jVal1);
					resultValue["field"].append(jVal2);
					output  = Json::writeString(wbuilder, resultValue);  
					return output;
				}
				nontransaction N(C);
				result R( N.exec( factorsql ));
				int resSize = R.size();
				if(resSize>0)
				{
					string fallsubQry = "";
					for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
					{
				        fallsubQry = "\"" + c[0].as<string>() +"\" & " + c[1].as<string>() + "";
				        paramStr1 += ","+fallsubQry+" AS falling"+std::to_string(i)+"";
			            paramStr1 += ", LAG("+fallsubQry+") OVER (ORDER BY insert_time_stamp) AS prevFalling"+std::to_string(i)+"";	
					}
				}
				N.commit();
				C.disconnect ();
			}
			catch (const std::exception &e) 
			{
				Json::Value jVal1, jVal2, jVal3;
			
				jVal1["result"] = "Fail1";
				jVal2["reason"] = e.what();
				jVal3["query"] = factorsql;
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				resultValue["field"].append(jVal3);
			}
		}
	}
	
	paramStr1 += " FROM data_view WHERE insert_time_stamp >='"+utcstartT+"'  AND insert_time_stamp <= '"+utcendT+"' \n";	
	
	//string paramStr3;
	string qrySelect = std::string("");
	const char * sql;
	if(ipnamearr.size() > 0){
	   qrySelect += paramStr0 + " FROM (\n" + paramStr1 + ") dt \nWHERE (\n" + paramStr2 + ")\n ORDER BY insert_time_stamp\n";
	}
	else{
	   qrySelect = paramStr0 + " FROM (\n" + paramStr1 + ") dt \nWHERE " + paramStr2 + " ORDER BY insert_time_stamp\n";
	}
	qrySelect += " LIMIT 500";
	cout << "fetchtimeStamps Qry: " << qrySelect << endl;
	sql = qrySelect.c_str();
	try 
	{
		connection C(strConnection);
		if (!C.is_open()) {
			//std::string output;
			Json::Value jVal1, jVal2;
			
			jVal1["result"] = "Fail";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( sql ));
		int resSize = R.size();
		//int m=0;
		int lastRow = resSize - 1;

		if(resSize>0)
		{
			int m=0;
			for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
			{
				Json::Value newValue;
				for(int i=0; i < colSize; i++)
				{
					Json::Value colValue;
					
					colValue = c[i].as<string>().c_str();
					newValue.append(colValue);
				}
				resultValue["field"].append(newValue);
				m++;
			}
		}
		N.commit();
		C.disconnect ();
	}
	catch (const std::exception &e) 
	{
		Json::Value jVal1, jVal2;
		
		jVal1["result"] = "Fail";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}	
	output  = Json::writeString(wbuilder, resultValue);  
	return output;
}

string process_extractT0Csv(const char* startTime,const char* endTime)
{
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	string qrySelect = std::string("");
	const char * sql;
	std::string sTime = startTime;
	std::string eTime = endTime;
	
	string ct;
	string query2;
	int lSize;
	int row1;
	int row2;
	int row3;
	int k=0;
	int l=0;
	
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");

	try{
		connection C(strConnection);
		if (C.is_open()){
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else{
			cout << "Can't open database" << endl;
		}      		
		qrySelect ="SELECT COLUMN_NAME, ORDINAL_POSITION FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 'data_table' \
		and COLUMN_NAME != 'source_ip' and COLUMN_NAME !='source_port' and COLUMN_NAME != 'caller_id' ORDER BY 2;";
		sql = qrySelect.c_str();
		nontransaction N(C);
		result R( N.exec( sql ));
		for (result::const_iterator c = R.begin(); c != R.end(); ++c)
		{
			Json::Value newValue;	
			newValue["column_name"] = c[0].as<string>();
			resultValue["field"].append(newValue);
		}
		lSize = resultValue["field"].size();
		C.disconnect ();
	}
	catch (const std::exception &e){
		cerr << e.what() << std::endl;
	}
		
	row1 = 	lSize;
	string array1[row1];
	const Json::Value column_Names = resultValue["field"];	
  	for(int i=0; i<row1; i++){	
		array1[i] = column_Names[i]["column_name"].asString();
	}

    resultValue = Json::Value::null;
    
	try{
		connection C(strConnection);
		if (C.is_open()){
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else{
			cout << "Can't open database" << endl;
		}      		
		qrySelect ="select field_name from field_defs where parameter_data_type = 'Packed';";
		sql = qrySelect.c_str();
		nontransaction N(C);
		result R( N.exec( sql ));
		for (result::const_iterator c = R.begin(); c != R.end(); ++c)
		{
			Json::Value newValue;	
			newValue["field_name"] = c[0].as<string>();
			resultValue["field"].append(newValue);
		}
		lSize = resultValue["field"].size();	
		C.disconnect ();
	}
	catch (const std::exception &e){
		cerr << e.what() << std::endl;
	}    

	row2 = 	lSize;
	string array2[row2];
	const Json::Value field_Names = resultValue["field"];
  	for(int i=0; i<row2; i++){
		array2[i] = field_Names[i]["field_name"].asString();
	}	

    resultValue= Json::Value::null;

	try{
		connection C(strConnection);
		if (C.is_open()){
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else{
			cout << "Can't open database" << endl;
		}      		
		qrySelect ="select count(parameter_name) from field_defs where parameter_data_type != 'Packed' \
		and field_name in (select parameter_name from field_defs where parameter_data_type = 'Packed');";
		sql = qrySelect.c_str();
		nontransaction N(C);
		result R( N.exec( sql ));
		for (result::const_iterator c = R.begin(); c != R.end(); ++c)
		{
			Json::Value newValue;	
			newValue["count_no"] = c[0].as<string>();
			resultValue["field"].append(newValue);
		}
		const Json::Value count = resultValue["field"];
		ct = count[0]["count_no"].asString();
		C.disconnect ();
	}
	catch (const std::exception &e){
		cerr << e.what() << std::endl;
	}

    resultValue= Json::Value::null;
	l = stoi(ct);
    row3 = (row1 - row2) + l;
	string array3[row3];
	k=1;
	l=0;

	try{
		connection C(strConnection);
		if (C.is_open()){
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else{
			cout << "Can't open database" << endl;
		}
      	query2 = "select \"insert_time_stamp\" AT TIME ZONE 'utc' at time zone current_setting('TimeZone'),";
      	array3[0] = "insert_time_stamp";
       	for(int i=1; i<row1-1; i++){
      		for(int j=0; j<row2; j++){
      			if(array1[i] == array2[j]){
					l++;
      				qrySelect ="select parameter_name from field_defs where field_name = '"+array1[i]+"' \
      				and parameter_data_type != 'Packed';";
					sql = qrySelect.c_str();
					nontransaction N(C);
					result R( N.exec( sql ));
					for (result::const_iterator c = R.begin(); c != R.end(); ++c)
					{
						Json::Value newValue;	
						newValue["parameter_name"] = c[0].as<string>();
						resultValue["field"].append(newValue);
					}
					lSize = resultValue["field"].size();
					if(lSize != 0){
						const Json::Value parameter_Names = resultValue["field"];
					  	for(int m=0; m<lSize; m++){
							array3[k] = parameter_Names[m]["parameter_name"].asString();
							query2 += "(\""+array1[i]+"\" & (select bit_mask from field_defs where parameter_name = '"+array3[k]+"')) / (select bit_mask from field_defs where parameter_name = '"+array3[k]+"') as "+array3[k]+",";
							k++;
						}
					}				
      			}
      		}
      		if(l == 0){
      			array3[k] = array1[i];
      			query2 += "\""+array1[i]+"\",";
      		    k++;
      		    //cout << array1[i] << endl;	
      		}
      		resultValue= Json::Value::null;
      		l=0;
    	}
    	array3[k] = array1[row1-1];
		query2 += "\""+array1[row1-1]+"\" from data_view where insert_time_stamp between (select '"+sTime+"' AT TIME ZONE 'utc' at time zone current_setting('TimeZone')) and (select '"+eTime+"' AT TIME ZONE 'utc' at time zone current_setting('TimeZone')) order by insert_time_stamp;";
		//cout << "query2" << query2;
		C.disconnect();
		
    }	
	catch (const std::exception &e){
		cerr << e.what() << std::endl;
	}	

	resultValue = Json::Value::null; 

	try{
		connection C(strConnection);
		if (C.is_open()){
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else{
			cout << "Can't open database" << endl;
		}  
		
		qrySelect = query2;
		sql = qrySelect.c_str();
		nontransaction N(C);
		result R( N.exec( sql ));
		k=0;
		Json::Value newValue;
		for (result::const_iterator c = R.begin(); c != R.end(); ++c){		
			for(int i=0; i<row3; i++){
				//cout << array3[i]<<endl;
				if(array3[i] == "Spare_Real" || array3[i] == "Spare_Int" || array3[i] == "Spare_Bit" || array3[i] == "Spare_byte"){
					newValue[""+array3[i]+""].append("0");
				}
				else{
					newValue[""+array3[i]+""].append(c[i].as<string>());	
				}
			}
		}
		resultValue["field"].append(newValue);

		/*const Json::Value parameter_Name_Val = resultValue["field"];
		lSize = parameter_Name_Val[0]["insert_time_stamp"].size();
		for(int i=0;i<lSize;i++){
			cout << parameter_Name_Val[0]["insert_time_stamp"][i].asString() << endl;
		}*/	

		C.disconnect ();
		std::string output;
		output  = Json::writeString(wbuilder, resultValue);
		//cout << output << endl;
		return output;	
	}
	catch (const std::exception &e){
		cerr << e.what() << std::endl;
	}	
}
/**
 * @brief [This function will be used to validates the given credentails]
 * @details [In order to use the APIs, the caller must provide the credentials. Upon verifying the credentials, the API will return the result. So this function will be used to validate the credentials provided.]
 * 
 * @param user [description]
 * @param password [description]
 * 
 * @return [description]
 */
int CheckPassword( const char* user, const char* password )
{
    struct passwd* passwdEntry = getpwnam( user );
    if ( !passwdEntry )
    {
        printf( "User '%s' doesn't exist\n", user );
        return 1;
    }

    if ( 0 != strcmp( passwdEntry->pw_passwd, "x" ) )
    {
        return strcmp( passwdEntry->pw_passwd, crypt( password, passwdEntry->pw_passwd ) );
    }
    else
    {
        // password is in shadow file
        struct spwd* shadowEntry = getspnam( user );
        if ( !shadowEntry )
        {
            printf( "Failed to read shadow entry for user '%s'\n", user );
            return 1;
        }

        return strcmp( shadowEntry->sp_pwdp, crypt( password, shadowEntry->sp_pwdp ) );
    }
}

// Function to remove duplicate elements
// This function returns new size of modified
// array.
int removeDuplicates(int arr[], int n)
{
    // Return, if array is empty
    // or contains a single element
    if (n==0 || n==1)
        return n;
 
    int temp[n];
 
    // Start traversing elements
    int j = 0;
    for (int i=0; i<n-1; i++)
 
        // If current element is not equal
        // to next element then store that
        // current element
        if (arr[i] != arr[i+1])
            temp[j++] = arr[i];
 
    // Store the last element as whether
    // it is unique or repeated, it hasn't
    // stored previously
    temp[j++] = arr[n-1];
 
    // Modify original array
    for (int i=0; i<j; i++)
        arr[i] = temp[i];
 
    return j;
}
std::string current_date(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf, sizeof(buf), "%A %d-%m-%Y", &tstruct);
    return buf;
}
std::string current_time(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%X", &tstruct);
    return buf;
}
std::string today_date(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf, sizeof(buf), "%d_%m_%Y", &tstruct);
    return buf;
}
std::string now_time(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%H_%M_%S", &tstruct);
    return buf;
}
std::string getutcfromlocalusingdb(const char* timestr){
	std::string inputStr = timestr;
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	const char * factorsql;
	string factorSelect = "select '"+inputStr+"'::TIMESTAMP WITHOUT TIME ZONE at time zone current_setting('TimeZone') at TIME ZONE 'utc'";
	factorsql = factorSelect.c_str();

	try 
	{
		connection C(strConnection);
		if (!C.is_open()) 
		{
				//std::string output;
			Json::Value jVal1, jVal2;

			jVal1["result"] = "Fail1";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( factorsql ));
		int resSize = R.size();
		if(resSize>0)
		{
			for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
			{
				output = c[0].as<string>();
			}
		}
		N.commit();
		C.disconnect ();
	}
	catch (const std::exception &e) 
	{
		Json::Value jVal1, jVal2;

		jVal1["result"] = "Fail2";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}
	return output;
}
std::string get_timezoneinfo(){
    redi::ipstream proc("cat /etc/timezone",redi::pstreams::pstdout | redi::pstreams::pstderr );
	std::string line;

	// read child's stdout
	while (std::getline(proc.out(), line)){
		std::cout << "stdout: " << line << '\n';
		return line;
	}
	if (proc.eof() && proc.fail())
		proc.clear();
	// read child's stderr
	while (std::getline(proc.err(), line)){
		std::cout << "stderr: " << line << '\n';
		return line;
	}
	return line;
}

string fetch_dayWiseCounts(const char* startDate,const char* endDate){
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string start_date = startDate;
	std::string end_date = endDate;
	std::string condition = std::string("");
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	const char* sql;

	condition = "where date(insert_time_stamp) between '"+start_date+"' and '"+end_date+"'";
	string factorSelect = "select date(insert_time_stamp), min(insert_time_stamp), max(insert_time_stamp), count(*) from data_table "+condition+" group by date(insert_time_stamp);";
	sql = factorSelect.c_str();

	try 
	{
		connection C(strConnection);
		if (!C.is_open()) 
		{
			Json::Value jVal1, jVal2;

			jVal1["result"] = "Fail1";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( sql ));
		Json::Value newValue;
		for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
		{
			newValue["date"] = c[0].as<string>();
			newValue["start_time"] = c[1].as<string>();
			newValue["end_time"] = c[2].as<string>();
			newValue["count"] = c[3].as<string>();
			resultValue["field"].append(newValue);
		}
		N.commit();
		C.disconnect ();
	}
	catch (const std::exception &e) 
	{
		Json::Value jVal1, jVal2;

		jVal1["result"] = "Fail2";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}
	output  = Json::writeString(wbuilder, resultValue);
	return output;
}
string version_info(const char* pkgName){
	std::string ipval = pkgName;
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	
	if(ipval == "plcdatacollector"){
		/*string verNo = "1.0";
		string reldt = "2022-03-04";
		string relnote = "Initial Version";*/
		/*string verNo = "1.1";
		string reldt = "2022-04-11";
		string relnote = "Data will be always logged in UTC irrespective of server's locale";*/
		/*string verNo = "1.2";
		string reldt = "2022-04-19";
		string relnote = "Added a method to consider day light saving when substracting the difference while calculating the UTC time";*/
		/*string verNo = "1.3";
		string reldt = "2022-05-10";
		string relnote = "Added a c++ function to get UTC time by substracting the difference";*/
		string verNo = "1.4";
		string reldt = "2022-05-30";
		string relnote = "Used postgresql in built function to get UTC time";
		Json::Value newValue;
		newValue["versionNo"] = verNo;
		newValue["releaseDate"] = reldt;
		newValue["releaseNote"] = relnote;
		resultValue["field"].append(newValue);
	}
	output  = Json::writeString(wbuilder, resultValue);
	return output;
}
void write_version_to_file()
{
		 /*    map<string, string> month {
        {"Jan", "01"}, {"Feb", "02"}, {"Mar","03"}, {"Apr","04"}, {"May", "05"}, {"Jun","06"}, {"Jul","07"}, {"Aug","08"}, {"Sep","09"}, {"Oct","10"},
        {"Nov","11"},{"Dec","12"}
    };  */

    std::string date = __DATE__;
    std::string time = __TIME__;

    string samp;
    stringstream sstream_word7(date);
    vector<string> param;
    while(sstream_word7 >> samp){
        param.push_back(samp);
    }
    if(stoi(param.at(1)) >=10){
        date = param.at(1)+"-"+param.at(0)+"-"+param.at(2);
    }else{
        	date ="0"+param.at(1)+"-"+param.at(0)+"-"+param.at(2);
    }

    std::string date_time = date+" "+time;

	bool changed_version_num = false;
	bool changed_version_date = false;
	bool changed_version_comment = false;
	int lines_count=0;
	int count1=0,count2=0,count3=0;
  	std::string cur_date = current_date();
	std::string cur_time = current_time();
	ifstream aFile ("/etc/versionInfo/versionconfig.conf");			//server 

 	std::string line,findStr1,findStr2,findStr3,getStr1="#dataloggerapi";
    	while (std::getline(aFile , line))
    	{
    		   ++lines_count;
      		if(getStr1 == line){
          	count1 = lines_count;
     	 		}
     	 	if(lines_count == count1+1){
     	 		findStr1=line;
     	 	}	
     	 	if(lines_count == count1+2){
     	 		findStr2=line;
     	 	}
     	 	if(lines_count == count1+3){
     	 		findStr3=line;
     	 	}
     	}   	
    count2=count1+2;
    count3=count1+3;
    //feetching old version number from the sentence
	string tmp;
	string oldVer_num;

    stringstream sstream_word(findStr1);
    vector<string> words;
    	while(sstream_word >> tmp)
    		{
    			words.push_back(tmp);
    		}
        for(int i = 0; i < words.size(); i++)
        {
                if(i == 2)
                {
                	oldVer_num = words.at(i);
                }
        }

	std::string var_num = VERSION_NUMBER;
 
  	if( oldVer_num != var_num )
  	{
  	//cout<<"string are not equal"<<endl;

	std::string qry1="sudo sed -i '"+to_string(count1+1)+" s/"+findStr1+"/version_number = "+var_num+"  #Changed to "+var_num+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

		int n;
		n=system(qry1.c_str());
		//cout<<"value of n :"<<n<<endl;
		if(n == 0){
			//cout<<"New version added in config file."<<endl;
			changed_version_num = true;
		}else{
			cout<<"Not able to Change version number."<<endl;
		}

  	}
  	else{
  	//cout<<"string are equal.....Version number already updated"<<endl;

  	}   

//----------------------------version date-------------------------------------------
    //feetching old date and time from the sentence
		string tm;
		string old_date_time;

        stringstream sstream_words(findStr2);
    	vector<string> word;
    	while(sstream_words >> tm)
    		{
    			word.push_back(tm);
    		}
        for(int i = 0; i < word.size(); i++)
        {
                if(i == 2 || i == 3)
                {
                	old_date_time += word.at(i);
                	old_date_time += " ";
                }
        }

        old_date_time.pop_back();	//removes the last " "

	  //  cout<<"old_date-time:"<<old_date_time<<endl;

	  // cout<<"new_date-time:"<<date_time<<endl;
	   // cout<<findStr2<<endl;

	if(old_date_time != date_time)
 	 {
  		//cout<<"old date_time and new date_time are different."<<endl;
		std::string qry3="sudo sed -i '"+to_string(count2)+" s/"+findStr2+"/version_date = "+date_time+"  #Changed to "+date_time+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

  	    int p;
		p=system(qry3.c_str());
		//cout<<"value of p :"<<p<<endl;
		if(p == 0){

		//cout<<"New date and time added in config file and commented old ."<<endl;
		changed_version_date = true;

		}else{
			cout<<"Not able to Change date and time."<<endl;
		}

 	 }else{
   		//cout<<"date and time are same"<<endl; 	
  	}

 //--------------------------------version comment--------------------------------------------

    //feetching old version comment from the sentence
	string temp;
	string old_Ver_comment;

    stringstream sstream_word1(findStr3);
    vector<string> Word;
    	while(sstream_word1 >> temp)
    		{
    			Word.push_back(temp);
    		}
        for(int i = 0; i < Word.size(); i++)
        {
                if(i >= 2)
                {
                	if(Word.at(i)=="#Changed"){
                		break;
                	}
                	old_Ver_comment += Word.at(i);
                	old_Ver_comment += " ";
                }
        }      
        old_Ver_comment.pop_back();	//removes the last " "

      //  cout<<"old comment :"<<old_Ver_comment<<endl;
      //  cout<<"new comment :"<<VERSION_COMMENT<<endl;

  		std::string new_comment = VERSION_COMMENT;

  	if(old_Ver_comment != new_comment)
  	{
  //	cout<<"old comment and new comment are different."<<endl;

	std::string qry5="sudo sed -i '"+to_string(count3)+" s/"+findStr3+"/version_comment = "+new_comment+"  #Changed to "+new_comment+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

  		int y;
		y=system(qry5.c_str());
		//cout<<"value of y :"<<y<<endl;
		if(y == 0){

			//cout<<"New version comment added in config file."<<endl;
			changed_version_comment = true;

		}else{
			cout<<"Not able to Change version comment."<<endl;
		}

  	}else{
   		//cout<<"old and new version comment are same"<<endl; 	
 	 }

  if(changed_version_comment || changed_version_date || changed_version_num){

  		std::string qry10 = "sudo sed -i '"+to_string(count1+4)+" a \n' /etc/versionInfo/versionconfig.conf";
  		std::string qry7 = "sudo sed -i '"+to_string(count1+5)+" i #"+findStr3+"' /etc/versionInfo/versionconfig.conf";
  		std::string qry8 = "sudo sed -i '"+to_string(count1+5)+" i #"+findStr2+"' /etc/versionInfo/versionconfig.conf";
  		std::string qry9 =  "sudo sed -i '"+to_string(count1+5)+" i #"+findStr1+"' /etc/versionInfo/versionconfig.conf";

  		int h7,h8,h9,h10;
  		h10= system(qry10.c_str()); //cout<<"h10:"<<h10<<endl; 
  		if(h10==0){
  			//cout<<"New line added."<<endl;
  		}else{
  			cout<<"Failed to add new line."<<endl;
  		}

  		h7= system(qry7.c_str());	//cout<<"h7:"<<h7<<endl;
  		if(h7==0){
  			//cout<<"old version_comment line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_comment line."<<endl;
  		}  	

  		h8= system(qry8.c_str());	//cout<<"h8:"<<h8<<endl;
   		if(h8==0){
  			//cout<<"old version_date line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_date line."<<endl;
  		}  

  		h9= system(qry9.c_str());	//cout<<"h9:"<<h9<<endl;
   		if(h9==0){
  			//cout<<"old version_number line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_number line."<<endl;
  		}  		
  }
}

/*----------------------------------------------------------------------------------------
This function gets called when the get_versions API was invoked
    1. This function will show current versions of all backend files.(version no,relese date,relese comment)
        It will show info of below files:
        a.plcdatacollector.cpp
        b.alarmdatacollector.cpp
        c.server.cpp
        d.kpidata.cpp
        e.kpicollectioncron.cpp
        f.commissioning-server.cpp

This function handles error responses in the follwing cases
    1. When the unidentified errors occured    
    2. versionconfig.conf file should be there in /etc/versionInfo/ otherwise it will show error
-------------------------------------------------------------------------------------------*/

string get_versions(){

	clear();
    system("/bin/versionInfo/versionAPI >> str.txt");
   
    fstream f("str.txt", fstream::in );
    string s="";

    while(!f.eof()){
        string p;
        getline( f, p);
        p+="\n";
        s+=p;
    }
    

    return s;
    f.close();
}

void clear()
{
    ofstream file("str.txt");
    file<<"";
}


//-------------------------------------------------------------------------------------


