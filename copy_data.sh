sudo -u postgres psql
\c plc_data
status= $(select \COPY data_table from '/home/sreenath/Downloads/data_table_10min.csv' DELIMITER ',' CSV HEADER;)
echo $status
