#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <pqxx/pqxx> 


using namespace std;
using namespace pqxx;

std::string now_time();
std::string today_date();

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
	//pch = strtok(NULL, "}");
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
	
	
	if (found==std::string::npos)
		found = strKey.find("ARRAY");
	if (found!=std::string::npos){
		std::size_t iStart = strKey.find("..");
		std::size_t iEnd = strKey.find("]");
		
		if ( (iStart!=std::string::npos) && (iEnd!=std::string::npos) ){
			strArraySize = strKey.substr(iStart+2, iEnd-(iStart+2));
			int iSize;
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

	//cout << pFinalStr << endl;
	//cout << "Name: " << strName << " Key: " << strKey << " Comment: " << strComment << endl;
	//cout << endl;
	
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


int main(int argc, char *argv[]){
   
	
	fstream newfile;
	
	string strDataFieldName, strBaseFieldName;
	


	
	std::vector<std::string> dataFieldName;
	std::vector<std::string> dataFieldType;
	
	std::vector<std::string> fdParamName;
	std::vector<std::string> fdParamDataType;
	std::vector<std::string> fdParamFieldName;
	std::vector<std::string> fdBitNumber;
	std::vector<std::string> fdBitMask;
	std::vector<std::string> fdParamDesc;
	std::vector<std::string> fdByteIndex;
	std::vector<std::string> fdFieldSize;
	
	string clValue;
	string strFileIn, strHost, strPort, strUser, strPwd, strDB;
	//string ParameterName, ParameterDataType, PackedParamName, BitNumber;  
	//bool bSkip = false;
	
	//int iLastFieldIndex=0;

	map<string, int> numberOfBytes {
		{"Byte", 1}, {"BYTE", 1}, {"Int", 2}, {"INT",2}, {"Long", 4}, {"LONG",4}, {"Real",4}, {"REAL",4}, {"UInt",2}, {"UINT",2}
	};
	map<string, string> fieldTypes {

		{"Byte", "SMALLINT"}, {"BYTE", "SMALLINT"},
		{"Int" , "SMALLINT"}, {"INT","SMALLINT"},
		{"Long", "INT"}, {"LONG","INT"}, 
		{"Real", "REAL"}, {"REAL","REAL"},
		{"UInt", "SMALLINT"}, {"UINT","SMALLINT"}
	};

	//Set the defaults for the cammaond line parameters
	strFileIn=std::string("");
	strHost = std::string("127.0.0.1");
	strPort = std::string("5423");
	strUser = std::string("postgres");
	strPwd = std::string("postgres");
	strDB = std::string("plc_data");

	// usage <program> -f [input file] -h [database host] -P [port] -u [user name] -p [pass word] -d [database name]
	
	for(int i = 1; i<argc; i=i+2){
		string clOption;
		clOption = std::string(argv[i]);
		clValue = std::string(argv[i+1]);
		
		if(clOption == "-f")
		{
			strFileIn = clValue;
			//cout << "Filename: " << clValue << endl;
		}
		else if(clOption == "-h")
		{
			strHost = clValue;
			//cout << "Host: " << clValue << endl;
		}
		else if(clOption == "-P")
		{
			strPort = clValue;
			//cout << "Port: " << clValue << endl;
		}
		else if(clOption == "-u")
		{
			strUser = clValue;
			//cout << "User: " << clValue << endl;
		}
		else if(clOption == "-p")
		{
			strPwd = clValue;
			//cout << "Password: " << clValue << endl;
		}
		else if(clOption == "-d")
		{
			strDB = clValue;
			//cout << "Database: " << clValue << endl;
		}
		else
		{
			cout << "Unknown option: " << clOption << ": " << clValue << endl;
		}
	}
	
	if( (argc<2) || (strFileIn == "") )
	{
		std::cerr<<"Error! No input file specified. Please specify an input file [*.AWL | *.DB]"<<endl; 
		std::cerr << "Usage " << argv[0] <<  " [options <default>]" << endl;
		cout << "\t-f \tFile name" << endl;
		cout << "\t-h \tHost <" << strHost << ">" << endl;
		cout << "\t-P \tPort <" << strPort << ">" << endl;
		cout << "\t-u \tUser <" << strUser << ">" << endl;
		cout << "\t-p \tPwd <" << strPwd << ">" << endl;
		cout << "\t-d \tDB <" << strDB << ">" << endl;
		return 1;
	}
	
	cout << "Using:" <<endl;
	cout << "\tFile:\t" << strFileIn << endl;
	cout << "\tHost:\t" << strHost << endl;

	cout << "\tPort:\t" << strPort << endl;

	cout << "\tUser:\t" << strUser << endl;
	cout << "\tPwd:\t<pwd>" << endl;
	cout << "\tDB:\t" << strDB << endl;
	
	
	newfile.open(strFileIn,ios::in); //open a file to perform read operation using file object
	if (newfile.is_open()){   //checking whether the file is open
		int iLevel=0;
		string strLine;
		string strPackedFieldBaseName;
		int bitNumber = 1;
		int iByteIndex = 0;
		
		while(getline(newfile, strLine))
		{ //read data from file object and put it into string.

			strLine.erase(remove(strLine.begin(), strLine.end(),'\t'), strLine.end());
			strLine.erase(remove(strLine.begin(), strLine.end(), ';'), strLine.end());
			splitLine(strLine);
			//cout << strName << " | " << strKey << " | " << strComment << endl;
			
			switch(iLevel)
			{
				
				case 0:
					if (strName == "STRUCT")
						iLevel++;
					else if(strName == "END_STRUCT")
						iLevel--;
					break;
				
				case 1:
					if((strKey == "STRUCT") || (strKey == "Struct") ){ 
						strBaseFieldName = "";
						iLevel++;
					}
					else if(strName == "END_STRUCT"){
					   iLevel--;
					}
				   break;
				   
				case 2:
					if( (strKey == "END_STRUCT") || (strName == "END_STRUCT") ){
						iLevel--;
						bitNumber=1;
					}
					else if( (strKey == "STRUCT") || (strKey == "Struct") ){
						strBaseFieldName = strName;
						strDataFieldName = strBaseFieldName;
					
						strPackedFieldBaseName = strDataFieldName;
						bitNumber=1;
						iLevel++;
					}
					else if(strKey == "Array"){
						dataFieldName.push_back(strName);
						dataFieldType.push_back("VARCHAR("+strArraySize+")");
						
						//A record in the field Defs table is required
						fdParamName.push_back(strName);
						fdParamDataType.push_back("Array");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(strArraySize);
						iByteIndex += std::stoi(strArraySize);
						bitNumber=1;
					}
					else{
						strDataFieldName =  strName;
						
						dataFieldName.push_back(strDataFieldName);
						dataFieldType.push_back(strKey);

						//A record in the field Defs table is required
						fdParamName.push_back(strDataFieldName);
						fdParamDataType.push_back(strKey);
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
						iByteIndex += numberOfBytes[strKey];
					}
					break;

				case 3:
				
					if(strName == "END_STRUCT"){
						iLevel--;
						bitNumber=1;
					}
					else if( (strKey == "STRUCT") || (strKey == "Struct") ){
						strDataFieldName = strBaseFieldName + "_" +  strName;					
						strPackedFieldBaseName = strDataFieldName;
						bitNumber=1;
						iLevel++;
						
					}
					else if(strKey == "Array"){
						dataFieldName.push_back(strDataFieldName + "_" + strName);
						dataFieldType.push_back("VARCHAR("+strArraySize+")");
						
						//A record in the field Defs table is required
						fdParamName.push_back(strDataFieldName + "_" + strName);
						fdParamDataType.push_back("Array");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName + "_" + strName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(strArraySize);
						iByteIndex += std::stoi(strArraySize);
						bitNumber=1;
					}
					else if( (strKey == "BOOL") || (strKey == "Bool") ){
						//Field type to be created in the data table - only on the first bool 
						int index;
						if(bitNumber% 16 == 1){
							index=bitNumber/16;
							dataFieldName.push_back(strBaseFieldName + std::to_string(bitNumber / 16));
							dataFieldType.push_back("Byte"); //

							//A record in the field Defs table is also required
							fdParamName.push_back(strBaseFieldName + std::to_string(bitNumber / 16));
							fdParamDataType.push_back("Packed");
							fdParamDesc.push_back(strComment);
							fdParamFieldName.push_back(strBaseFieldName + std::to_string(bitNumber / 16));
							fdBitNumber.push_back(std::to_string(0));
							fdBitMask.push_back(std::to_string(1));
							fdByteIndex.push_back(std::to_string(iByteIndex));					// iByteCount will be incremented on exit from this level
							fdFieldSize.push_back(std::to_string(0));							// Will get edited on exiting from this level 
							iByteIndex += 1;
						}
						
						//A record in the field Defs table is required
						fdParamName.push_back(strPackedFieldBaseName + "__" + strName);
						fdParamDataType.push_back("BOOL");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strPackedFieldBaseName + to_string(index));
						fdBitNumber.push_back(std::to_string(bitNumber%8));
						fdBitMask.push_back(std::to_string(0x01<<((bitNumber-1) % 8)));
						fdByteIndex.push_back(std::to_string(iByteIndex));						// iByteCount remains the same
						fdFieldSize.push_back(std::to_string(0));
						bitNumber++;
					}
					else{
						strDataFieldName = strBaseFieldName + "_" +  strName;
						
						dataFieldName.push_back(strDataFieldName);
						dataFieldType.push_back(strKey);

						//A record in the field Defs table is required
						fdParamName.push_back(strDataFieldName);
						fdParamDataType.push_back(strKey);
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
						iByteIndex += numberOfBytes[strKey];
					}
					break;

				case 4:
					if(strName == "END_STRUCT"){
						iLevel--;
						bitNumber=1;
					}
					else if( 
								(strKey == "Byte") || (strKey == "BYTE") || 
								(strKey == "INT") || (strKey == "Int") || 
								(strKey == "Real") || (strKey == "REAL") ||
								(strKey == "UInt") || (strKey == "UINT") 
								){
						dataFieldName.push_back(strDataFieldName + "_" + strName);
						dataFieldType.push_back(strKey);
						
						//A record in the field Defs table is required
						fdParamName.push_back(strDataFieldName + "_" + strName);
						fdParamDataType.push_back(strKey);
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName + "_" + strName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
						iByteIndex += numberOfBytes[strKey];
						bitNumber=1;
					}
					else if(strKey == "Array"){
						dataFieldName.push_back(strDataFieldName + "_" + strName);
						dataFieldType.push_back("VARCHAR("+strArraySize+")");
						
						//A record in the field Defs table is required
						fdParamName.push_back(strDataFieldName + "_" + strName);
						fdParamDataType.push_back("Array");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName + "_" + strName);
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(0));
						fdByteIndex.push_back(std::to_string(iByteIndex));
						fdFieldSize.push_back(strArraySize);
						iByteIndex += std::stoi(strArraySize);
						bitNumber=1;
					}
					else{
						//Field type to be created in the data table - only on the first bool 
						int index1;
						if(bitNumber%16 == 1){
							
							index1 = bitNumber/16;
							dataFieldName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
							dataFieldType.push_back("Byte");

							//A record in the field Defs table is also required
							fdParamName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
							fdParamDataType.push_back("Packed");
							fdParamDesc.push_back(strComment);
							fdParamFieldName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
							fdBitNumber.push_back(std::to_string(0));
							fdBitMask.push_back(std::to_string(1));
							fdByteIndex.push_back(std::to_string(iByteIndex));					// iByteCount will be incremented on exit from this level
							fdFieldSize.push_back(std::to_string(0));							// Will get edited on exiting from this level 
							iByteIndex += 1;
						}
						
						//A record in the field Defs table is required
						fdParamName.push_back(strPackedFieldBaseName + "__" + strName);
						fdParamDataType.push_back("BOOL");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strPackedFieldBaseName+ to_string(index1));
						fdBitNumber.push_back(std::to_string(bitNumber%8));
						fdBitMask.push_back(std::to_string(0x01<<((bitNumber-1) % 8)));
						fdByteIndex.push_back(std::to_string(iByteIndex));						// iByteCount remains the same
						fdFieldSize.push_back(std::to_string(0));
						bitNumber++;
					}
					break;

				default:
					//cout << "Default case name:" << strName << ", key" << strKey << "\n"; 
					break;
			}
		}
		newfile.close(); //close the file object.
	}

	// The databse processing starts here	
	//std::string strConnection = std::string("dbname = ")+DBNAME+" user = "+DBUSER+" password = "+DBPWD+" hostaddr = "+DBHOST+" port = " + DBPORT;
	std::string strConnection = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
	string qryCreate=std::string("");
	string qryInsert=std::string("");		

	try {
		connection C(strConnection);
		if (!C.is_open()) {
			cout << "Can't open database" << endl;
			return 1;			
		}
		
		qryCreate=std::string("DROP VIEW IF EXISTS data_view;\n");
		work W(C);
		W.exec( qryCreate );		
		//cout << "View data_view dropped\n";

		qryCreate=std::string("ALTER TABLE IF EXISTS data_table RENAME TO data_table_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table data_table renamed\n";

		qryCreate=std::string("ALTER TABLE IF EXISTS field_defs RENAME TO field_defs_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table data_table renamed\n";
		
		qryCreate = std::string("");
		//Create the data_table
		qryCreate = "CREATE TABLE data_table(\n";
		qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";
		qryCreate += "sample_number BIGSERIAL,\n";
		qryCreate += "source_ip VARCHAR(20),\n";
		qryCreate += "source_port VARCHAR(10),\n";
		qryCreate += "caller_id VARCHAR(20),\n";

		string strDBFieldType=std::string("");
		for(int i=0; i < dataFieldType.size(); i++)
		{
			strDBFieldType = fieldTypes[dataFieldType[i]] != "" ? fieldTypes[dataFieldType[i]] : dataFieldType[i];
			qryCreate = qryCreate + "\"" + dataFieldName[i] + "\" " + strDBFieldType + ",\n";
		}
		qryCreate += "comp_ref smallint default 0,\n";
		qryCreate.pop_back();	//Remove the \n
		qryCreate.pop_back();	//Remove the ','

		qryCreate += ");";
		//cout << qryCreate << endl;
		W.exec( qryCreate );
		cout << "data_table created\n";
		
		//Create the field_defs table
		qryCreate = "CREATE TABLE field_defs(\n";
		qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";
		qryCreate += "source_ip VARCHAR(20),\n";
		qryCreate += "source_port VARCHAR(10),\n";
		qryCreate += "caller_id VARCHAR(20),\n";
		qryCreate += "parameter_name VARCHAR(50),\n";
		qryCreate += "parameter_data_type VARCHAR(20),\n";
		qryCreate += "field_name VARCHAR(50),\n";
		qryCreate += "bit_number INT DEFAULT 0,\n";
		qryCreate += "off_set INT DEFAULT 0,\n";
		qryCreate += "scaling_factor REAL DEFAULT 1.0,\n";
		qryCreate += "bit_mask INT DEFAULT 0,\n";
		qryCreate += "comment VARCHAR(100),\n";
		qryCreate += "param_serial INT,\n";
		qryCreate += "number_of_bytes INT";

		qryCreate += ");";
		W.exec( qryCreate );
		cout << "field_defs table created\n";		
		
		//Create view data_view
		qryCreate=std::string("create or replace view data_view as select * from data_table;\n");
		W.exec( qryCreate );		
		cout << "View data_view created\n"; 

		//Commit transactions so far
		W.commit();

		//Change the table to a hyertable on the insert_time_stamp
		work Wh(C);
		Wh.exec("SELECT create_hypertable('data_table', 'insert_time_stamp');");
		cout << "data_table converted to HyperTable\n";
		Wh.commit();
	
	
		//Insert the field definitions in the field_defs table
		for(int i=0, j=0; i < fdParamName.size(); i++)
		{
			//string qryInsert="INSERT INTO field_defs (insert_time_stamp, source_ip, source_port, caller_id,  parameter_name, parameter_data_type, field_name, bit_number, scaling_factor, bit_mask, comment, param_serial) VALUES\n";
			qryInsert += "INSERT INTO field_defs (parameter_name, parameter_data_type, field_name, bit_number, off_set, scaling_factor, bit_mask, comment, param_serial, number_of_bytes) VALUES\n";
			qryInsert += "    ('" + fdParamName[i] + "', '" + fdParamDataType[i] + "', '" + fdParamFieldName[i] + "', '" + fdBitNumber[i] + "', 0, 1.0, " + fdBitMask[i] + ", '" + fdParamDesc[i] + "', " + fdByteIndex[i] +", " + fdFieldSize[i] + ");\n" ;
			j = j + numberOfBytes[fdParamDataType[i]];
		}
		//cout << qryInsert << "\n";
		
		work Wd(C);
		Wd.exec(qryInsert);
		Wd.commit();
		cout << "field_defs table updated\n";
	} 
	catch (const std::exception &e) {
		cerr << e.what() << std::endl;
		cout << "==========================================" << endl;
		cout << qryCreate << endl;
		cout << "==========================================" << endl;
		cout << qryInsert << endl;
		return 1;
	}

}


