#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <pqxx/pqxx>
using namespace std;
using namespace pqxx;
int main(int argc,char* argv[]){
 if(argc != 2){
  cerr << "Usage: alarm_counts" << endl;
  exit(0);
 }
 int alarm_counts = atoi(argv[1]);
 string strHost, strPort, strUser, strPwd, strDB;
 //Set the defaults for the cammand line parameters
 strHost = std::string("127.0.0.1");
 strPort = std::string("5432");
 strUser = std::string("postgres");
 strPwd = std::string("postgres");
 strDB = std::string("plc_data");
 std::string strConnection = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
 string qryCreate = std::string("");
 string qryInsert = std::string("");
 try{
  connection C(strConnection);
  if(!C.is_open()){
   cout << "Can't open database" << endl;
   return 1;  
  }
  //Remove the alarm_defs
  qryCreate=std::string("DROP TABLE IF EXISTS alarm_defs;\n");
  work W(C);
  W.exec( qryCreate );  
  cout << "Table alarm_defs dropped\n";
  qryCreate = std::string("");
  //Create the alarm_defs
  qryCreate = "CREATE TABLE alarm_defs(\n";
  qryCreate += "alarm_id INT DEFAULT 0,\n";
  qryCreate += "db_field_name VARCHAR(20),";
  qryCreate += "bit_number INT DEFAULT 0,";
  qryCreate += "field_type VARCHAR(5))";
  //cout << "Query" << qryCreate;
  W.exec( qryCreate ); 
  W.commit();
  qryInsert = std::string("");
  //Create the alarm_defs
  qryInsert = "INSERT INTO alarm_defs(alarm_id,db_field_name,bit_number,field_type) values ";
  //int str2Counter = 1;
  int fnCounter = 1;
  int bitCounter = 0;
  //string dbfn = "";
  for(int i=1;i<=alarm_counts;i++){
   std::string str1 = "Alarm";
   //std::string str2 = "Int";
   int counter = 16;
   str1 = str1 + std::to_string(i);
   //cout << str1 << "\n";
   //cout << "bitCounter" << bitCounter << "\n";
   //cout << "Packed Bool Int" << fnCounter << "\n";
   string dbfn = "Packed Bool Int" + std::to_string(fnCounter);
   qryInsert += "('"+std::to_string(i)+"','"+dbfn+"',"+std::to_string(bitCounter)+",'BOOL'),";
   if(i%counter == 0){
    bitCounter = 0;
    fnCounter = fnCounter + 1;
   }
   else{
    bitCounter++;
   }
  }  
  qryInsert.pop_back(); //Remove the ','
  //cout << "Query" << qryInsert << endl;
  work N(C);
  N.exec( qryInsert );
  //Commit transactions so far
  N.commit();
  cout << "alarm_defs table updated\n";
 }
 catch (const std::exception &e) {
  cerr << e.what() << std::endl;
  //cerr << qryCreate << endl;
  //cerr << qryInsert << endl;
  return 1;
 }
 string qrySelect = std::string("");
 const char * sql;
 
 
 qrySelect += "select db_field_name from alarm_defs group by db_field_name order by LENGTH(db_field_name),db_field_name";
 sql = qrySelect.c_str();
 std::string strConnection1 = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
 try {
  connection C(strConnection1);
  if (C.is_open()) {
   cout << "Opened database successfully: " << C.dbname() << endl;
  } else {
   cout << "Can't open database" << endl;
  }
  /* Create a non-transactional object. */
  work N(C);
  /* Execute SQL query */
  result R( N.exec( sql ));
  /* List down all the records */
  std::vector<std::string> arrayFieldName;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
	  string fieldName;
   fieldName = c[0].as<string>();
   arrayFieldName.push_back(fieldName);
  }
  N.commit();

  qryCreate = std::string("");
    work W(C);

  qryCreate = std::string("DROP VIEW IF EXISTS alarm_view;\n");
  W.exec(qryCreate);
  qryCreate = std::string("");
  cout<<"View alarm_view dropped\n";
  //Remove the alarm_table table
  qryCreate=std::string("DROP TABLE IF EXISTS alarm_table;\n");
 
  W.exec( qryCreate );  
  cout << "Table alarm_table dropped\n";
  W.commit();
  qryCreate = std::string("");
  //Create the data_table
  work WC(C);
  qryCreate = "CREATE TABLE alarm_table(\n";
  qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";
  //string strDBFieldType=std::string("");
  for(int i=0; i < arrayFieldName.size(); i++)
  {
   qryCreate = qryCreate + "\"" + arrayFieldName[i] + "\" INT ,\n";
  }
  qryCreate += "comp_ref smallint default 0,\n";
  qryCreate.pop_back(); //Remove the \n
  qryCreate.pop_back(); //Remove the ','
  qryCreate += ");";
  //cout << qryCreate << endl;
  WC.exec( qryCreate );
  WC.commit();
  cout << "alarm_table created\n";
  //Change the table to a hyertable on the insert_time_stamp
  work Wh(C);
  Wh.exec("create or replace view alarm_view as select * from alarm_table;\n");
  cout <<"alarm_view created\n";
  Wh.exec("SELECT create_hypertable('alarm_table', 'insert_time_stamp');");
  cout << "alarm_table converted to HyperTable\n";
  Wh.commit();
  cout << endl << "Disconnecting from DB...\n";
  C.disconnect();
 }
 catch (const std::exception &e) {
  cerr << e.what() << std::endl;
  //cerr << qryCreate << endl;
  //return 1;
 }
}
