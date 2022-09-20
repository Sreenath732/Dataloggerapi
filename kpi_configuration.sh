echo "Do you wish to setup KPI data collection and it's API service?"
select yn in "Yes" "No"; do
	case $yn in
        Yes )
	./cdcreate
	status=$?
	if [ $status -ne 0 ];then
	   echo "Unable to create KPI tables in database"
	   exit
	fi
        echo "Create, Enable and Start the KPI Logger API Service"
	if [ ! -f /etc/systemd/system/kpiloggerapi.service ]; then
	    sudo mkdir /bin/kpiloggerapi
	    sudo cp kpidata /bin/kpiloggerapi
	    status=$?
	    if [ $status -ne 0 ];then
	   	echo "Installation failed at step6"
	   	exit
	    fi
	    #sudo cp kpiloggerapi.service /etc/systemd/system
	    sudo systemctl daemon-reload 
	    sudo systemctl enable $currpath/kpiloggerapi.service
	    sudo systemctl start kpiloggerapi.service
	fi
	#Create a folder as kpidatacollector in /etc/
	if [ ! -d /etc/kpidatacollector ]; then
	   sudo mkdir /etc/kpidatacollector
	fi
	sudo cp kpidatacollectioncron /etc/kpidatacollector/kpidatacollectioncron
	sudo sed -i "22i 10 */1 * *  * root  /etc/kpidatacollector/kpidatacollectioncron" /etc/crontab
	sudo service cron restart
	
	echo "Enable port access"
	sudo firewall-cmd --zone=public --permanent --add-port=9876/tcp
	sudo firewall-cmd --reload
	echo "The KPI Data Collection was enabled successfully. Kindly follow below steps to check if it's working or not."
	echo "Kindly create at least one KPI and check data in kpi_value_table after every hour 10 minutes."
	echo "Kindly check kpiloggerapi was running or not. Use service kpiloggerapi status to check the same."
	break;;
        No ) 
	echo "KPI Data Collection Skipped"
	break;;
   esac
done