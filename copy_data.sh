status=$(sudo psql -h 127.0.0.1 -U postgres -d plc_data -p 5432  -c "\COPY data_table from '/home/circleci/project/data_table_10min.csv' DELIMITER ',' CSV HEADER;")


read -p "Please enter postgres password to copy data into database: " 
        
        echo $status
