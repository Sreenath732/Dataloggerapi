echo "Do you wish to setup alarm data collection and it's API service?"
select yn in "Yes" "No"; do
	case $yn in
        Yes )
	echo "Reading Excel file to create alarm tables i.e., alarm_defs and alarm_table"
	sudo xls2csv -g 2 CMS_Editor_English_16W_example.xls > alarm.csv
	sudo sed -i '1,4d' alarm.csv
	sudo sed -i '10001,$d' alarm.csv
	./cdcsv
	status=$?
	if [ $status -ne 0 ];then
	   echo "Unable to create alarm tables in database"
	   exit
	fi
	echo "Create, Enable and Start the alarmdatacollector Service"
	sudo -u postgres psql -d 'plc_data' -c "SELECT set_chunk_time_interval('alarm_table', INTERVAL '24 hours');"
	read -p "Kindly enter the port No of alarm data: " alarmprt
	sudo sed -i 's/2061/'$alarmprt'/' /etc/plcdatacollector/plcdatacollector.conf
	sudo firewall-cmd --zone=public --permanent --add-port=$alarmprt/udp
	sudo firewall-cmd --reload
	status=$?
	if [ $status -ne 0 ];then
	   echo "Installation failed to setup alarm data collection"
	   exit
	fi
	if [ ! -f /etc/systemd/system/alarmdatacollector.service ]; then
	    sudo mkdir /bin/alarmdatacollector
	    sudo cp alarmdatacollector /bin/alarmdatacollector
	    status=$?
	    if [ $status -ne 0 ];then
	   	echo "Installation failed to setup alarm data collection"
	   	exit
	    fi 
	    #sudo cp alarmdatacollector.service /etc/systemd/system
	    sudo systemctl daemon-reload 
	    sudo systemctl enable $currpath/alarmdatacollector.service
	    sudo systemctl start alarmdatacollector.service
	fi
	echo "The alarm data collection was enabled successfully. Kindly follow below steps to check if it's working or not."
	echo "Kindly check alarmdatacollector service is running or not. Use service alarmdatacollector status to check the same."
	echo "Check the data in alarm_table, if the data is inserting or not. The row count will be increased everytime if you execute the query select count(*) from alarm_table"
	break;;
        No ) exit;;
    esac
done