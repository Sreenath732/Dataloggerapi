#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

int main() {
  string strHost, strPort, strUser, strPwd, strDB, strHt;
  //Set the defaults for the cammand line parameters
  strHost = std::string("127.0.0.1");
  strPort = std::string("5432");
  strUser = std::string("postgres");
  strPwd = std::string("postgres");
  strDB = std::string("plc_data");
  string qryCreate = std::string("");

  std::string strConnection = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
  
  try {
    connection C(strConnection);
    if (!C.is_open()) {
      cout << "Can't open database" << endl;
      return 1;     
    }
    //Remove the kpi_def 
    qryCreate=std::string("DROP TABLE IF EXISTS kpi_def;\n");
    work W(C);
    W.exec( qryCreate );    
    cout << "Table kpi_def dropped\n";
    
    qryCreate = std::string("");
    //Create the kpi_def
    qryCreate = "CREATE TABLE kpi_def(\n";
    qryCreate += "kpi_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1),\n";
    qryCreate += "kpi_name character varying(20) NOT NULL,\n";
    qryCreate += "param_name character varying(200) NOT NULL,";
    qryCreate += "bit_mask smallint NOT NULL,";
    qryCreate += "status smallint NOT NULL,";
    qryCreate += "field_name character varying(50),";
    qryCreate += "CONSTRAINT kpi_def_pkey PRIMARY KEY (kpi_id))";
    //cout << "Query" << qryCreate;
    W.exec( qryCreate );  
    cout << "Table kpi_def created...\n" << endl;
    W.commit();


    //Remove the kpi_value_table 
    qryCreate=std::string("DROP TABLE IF EXISTS kpi_value_table;\n");
    work M(C);
    M.exec( qryCreate );    
    cout << "Table kpi_value_table dropped\n";

    qryCreate = std::string("");
    //Create the kpi_value_table
    qryCreate = "CREATE TABLE kpi_value_table(\n";
    qryCreate += "kpi_id bigint NOT NULL,\n";
    qryCreate += "start_time timestamp without time zone,\n";
    qryCreate += "end_time timestamp without time zone,\n";
    qryCreate += "count integer,\n";
    qryCreate += "duration interval,\n";
    qryCreate += "hour double precision NOT NULL,\n";
    qryCreate += "date date NOT NULL,\n";
    qryCreate += "CONSTRAINT kpi_value_table_pkey PRIMARY KEY (kpi_id, hour, date))";
    //cout << "Query" << qryCreate;
    M.exec( qryCreate );  
    M.commit();
    cout << "Table kpi_value_table created...\n" << endl;

    cout << endl << "Disconnecting from DB...\n";
    C.disconnect();
  } catch (const std::exception &e) {
    cerr << e.what() << std::endl;
    //cerr << qryCreate << endl;
    //return 1;
  }
}

