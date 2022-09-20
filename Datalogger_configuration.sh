echo "The installation process need the super user password."
echo "Do you wish to continue?"
select yn in "Yes" "No"; do
    case $yn in
        Yes )
		df -h | awk '{ print $6 "-" $4 }' > dirfile.txt
		sed -i '1d' dirfile.txt
		echo "Choose the data directory for postgresql from the below list."
		dirfile="dirfile.txt"
		dirs=`cat $dirfile`
		currpath=$(pwd)
		select dir in $dirs 
		do
		echo "You have chosen $dir"
		dir=${dir%-*}
		sudo service firewall-cmd status
		status=$?
		if [ $status -ne 0 ];then
			sudo apt install firewalld
		fi
		if [ "$dir" != "" ]; then
			# Checking postgresql
			if [ -d /etc/postgresql ]; then
		   		echo "Postgresql was already installed in your system, do you want to skip reinstallation of postgresql? Or Do you want to reinstall postgresql(if you go with this you will loose all previous data in database).?"
		   		select yn in "Skip" "Reinstall" "Exit"; do
					case $yn in
			        Skip )
					echo "Step 1/11 Reinstallation of postgresql Skipped"   
					sudo systemctl daemon-reload
					
				    sudo service plcdatacollector stop	
				    sudo service alarmdatacollector stop	
				    sudo service dataloggerapi stop		
				    sudo service kpiloggerapi stop	
				    sudo service commissioning-server stop
				
				    sudo service postgresql restart
				    status=$?
				    if [ $status -ne 0 ]; then
				        echo "postgresql.service not started"
				    fi

					val=$(df -h | awk ''$dir'/ { print $5 }')
					val=${val::-1}
					#echo "val="$val
					if [ $val -le 50 ] ; then
						echo "Step 2/11 Checking new database as plc_data"
						status=$(sudo -u postgres psql -qAt -c "select exists( SELECT datname FROM pg_catalog.pg_database WHERE lower(datname) = lower('plc_data'));")
						#echo $status
						if [ $status == 't' ]; then
							echo "Database plc_data already exist"
							#exit
						else
							sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
							status=$?
							if [ $status -ne 0 ]; then
						   		echo "Installation failed at step2"
						   		exit
							fi	
						fi	

						echo "Step 3/11 Checking timescaledb extension on plc_data"
						status=$(sudo -u postgres psql -qAt -d 'plc_data' -c "select exists (select FROM pg_available_extensions where name = 'timescaledb');")
						if [ $status == 't' ]; then
							echo "timescaledb already added on plc_data"
						else
							sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
							status=$?
							if [ $status -ne 0 ]; then
								echo "Installation failed at step3"
								exit
							fi	
						fi
						echo "Step 4/11 Checking new table as data_table"
						status=$(sudo -u postgres psql -qAt -d 'plc_data' -c "SELECT EXISTS (SELECT table_name FROM information_schema.tables WHERE table_name = 'data_table');")
						#echo $status
						if [ $status == 't' ]; then
							echo "Table data_table already exist"
							#exit
						else
							./dbdc -f CART.AWL -P 5432
							status=$?
							if [ $status -ne 0 ];then
						   		echo "Unable to create tables in database"
						   		exit
							fi
							echo "Create a view for data_table"
							sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
							sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
						fi
					else
						echo "Not Enough space for further Installation.Kindly take a backup of existing"
						echo "Use the backup_restore.sh file to backup"

					fi
					break;;
			        Reinstall )
					directory=$( sudo -u postgres psql -t -c "SHOW data_directory" )
					sudo systemctl daemon-reload

				    sudo service plcdatacollector stop		
				    sudo service alarmdatacollector stop		
				    sudo service dataloggerapi stop
				    sudo service kpiloggerapi stop		
				    sudo service commissioning-server stop
				    
					sudo service postgresql stop
					sudo apt --fix-broken install
					status=$?
					if [ $status -ne 0 ];then
						echo "Postgresql service not stopped"
						#exit
					fi   
					sudo apt-get --purge remove postgresql-11 postgresql-client-11 postgresql-client-common postgresql-common
					status=$?
					if [ $status -ne 0 ];then
						echo "Postgresql not removed"
						exit
					fi
					sudo rm -d /etc/postgresql
					status=$?
					if [ $status -ne 0 ];then
						echo "/etc/postgresql not removed"
						#exit
					fi
					sudo rm -d /etc/postgresql-common
					status=$?
					if [ $status -ne 0 ];then
						echo "/etc/postgresql-common not removed"
						#exit
					fi
					sudo rm -rf $directory
					status=$?
					if [ $status -ne 0 ];then
						echo "postgres data directory not removed"
						#exit
					fi
					echo "Postgresql removed"
					echo "Step 1/11 Installation of postgresql-11 and timescaledb."
					sudo dpkg -i *.deb
					status=$?
					if [ $status -ne 0 ];then
						echo "Installation failed at step1"
						exit
					fi
					pgVesrion=$( sudo -u postgres psql -c "SHOW server_version;" )
					pgVesrion=`echo $pgVesrion | tr -dc '[:alnum:]\n\r' | tr '[:upper:]' '[:lower:]'`
					pgVesrion=`echo $pgVesrion | tr -d -c 0-9`
					#check for versions less than or equal to 9
					firstchar=${pgVesrion:0:1}
					status=$?
					if [ $status -ne 0 ];then
						echo "Installation failed at step1"
						exit
					fi
					if [ "$firstchar" -ne "1" ]; then
						echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
						exit
					fi
					#check for versions lessthan than or equal to 10
					first2chars=${pgVesrion:0:2}
					if [ "$first2chars" -lt "11" ]; then
						echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
						exit
					fi

					#Add 
					sudo sed -i "275i shared_preload_libraries = 'timescaledb'" /etc/postgresql/11/main/postgresql.conf
					sudo service postgresql restart	

					d=$(date +%Y%m%d_%H%M%S)
					#echo "$d"
					#Renaming plc_data to plc_data_datetime
					if sudo -u postgres psql -l | grep '^ plc_data\b' > /dev/null ; then
				  	   sudo -u postgres psql -c 'ALTER DATABASE plc_data RENAME to plc_data_'$d''
					  status=$?
					  if [ $status -ne 0 ];then
					    echo "Unable to rename the database"
					    exit
					  fi
					fi

					echo "Step 2/11 Creating new database as plc_data"
					sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
					status=$?
					if [ $status -ne 0 ];then
					   echo "Installation failed at step2"
					   exit
					fi

					echo "Step 3/11 Creating timescaledb extension on plc_data"
					sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
					status=$?
					if [ $status -ne 0 ];then
					   echo "Installation failed at step3"
					   exit
					fi

					if [ "$dir" != "/" ]; then
						if [ ! -d $dir/pgdata ]; then
					   		sudo mkdir $dir/pgdata
						fi
				        sudo chown postgres:postgres $dir/pgdata
				        sudo chmod 700 $dir/pgdata
				        sudo rsync -av /var/lib/postgresql/11/main/ $dir/pgdata/data > pg_data_directory_log.txt
				        SRC=/var/lib/postgresql/11/main
						DST=$dir/pgdata/data
						sudo sed -i "s|$SRC|$DST|" /etc/postgresql/11/main/postgresql.conf
				        sudo service postgresql restart
				    fi

					echo "Step 4/11 Reading AWL file to create analog tables i.e., field_defs and data_table"
					sudo -u postgres psql -c "ALTER USER postgres PASSWORD '143';"
					#read -p "Kindly enter the file name to create analog tables in database: " fname
					# read -p "Kindly enter the path of the file: " fpath
					# if [ ! -f $fpath/$fname ]; then
					#    echo "Unable to locate file"
					#    exit
					# fi
					./dbdc -f CART.AWL -P 5432
					status=$?
					if [ $status -ne 0 ];then
					   echo "Unable to create tables in database"
					   exit
					fi

					echo "Create a view for data_table"
					sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
					sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
					break;;
						Exit )
						exit
						break;;
			   		esac
				done
			else
				echo "Step 1/11 Installation of postgresql-11 and timescaledb."
				sudo dpkg -i *.deb
				status=$?
				if [ $status -ne 0 ];then
					echo "Installation failed at step1"
					exit
				fi
				pgVesrion=$( sudo -u postgres psql -c "SHOW server_version;" )
				pgVesrion=`echo $pgVesrion | tr -dc '[:alnum:]\n\r' | tr '[:upper:]' '[:lower:]'`
				pgVesrion=`echo $pgVesrion | tr -d -c 0-9`
				#check for versions less than or equal to 9
				firstchar=${pgVesrion:0:1}
				status=$?
				if [ $status -ne 0 ];then
					echo "Installation failed at step1"
					exit
				fi
				if [ "$firstchar" -ne "1" ]; then
					echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
					exit
				fi
				#check for versions lessthan than or equal to 10
				first2chars=${pgVesrion:0:2}
				if [ "$first2chars" -lt "11" ]; then
					echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
					exit
				fi

				#Add 
				sudo sed -i "275i shared_preload_libraries = 'timescaledb'" /etc/postgresql/11/main/postgresql.conf
				sudo service postgresql restart	
				
				d=$(date +%Y%m%d_%H%M%S)
				#echo "$d"
				#Renaming plc_data to plc_data_datetime
				if sudo -u postgres psql -l | grep '^ plc_data\b' > /dev/null ; then
			  	  sudo -u postgres psql -c 'ALTER DATABASE plc_data RENAME to plc_data_'$d''
				  status=$?
				  if [ $status -ne 0 ];then
				    echo "Unable to rename the database"
				    exit
				  fi
				fi

				echo "Step 2/11 Creating new database as plc_data"
				sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
				status=$?
				if [ $status -ne 0 ];then
				   echo "Installation failed at step2"
				   exit
				fi

				echo "Step 3/11 Creating timescaledb extension on plc_data"
				sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
				status=$?
				if [ $status -ne 0 ];then
				   echo "Installation failed at step3"
				   exit
				fi

				if [ "$dir" != "/" ]; then
					if [ ! -d $dir/pgdata ]; then
				   		sudo mkdir $dir/pgdata
					fi
			        sudo chown postgres:postgres $dir/pgdata
			        sudo chmod 700 $dir/pgdata
			        sudo rsync -av /var/lib/postgresql/11/main/ $dir/pgdata/data > pg_data_directory_log.txt
			        SRC=/var/lib/postgresql/11/main
					DST=$dir/pgdata/data
					sudo sed -i "s|$SRC|$DST|" /etc/postgresql/11/main/postgresql.conf
			        sudo service postgresql restart
			    fi

				echo "Step 4/11 Reading AWL file to create analog tables i.e., field_defs and data_table"
				sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'postgres';"
				#read -p "Kindly enter the file name to create analog tables in database: " fname
				# read -p "Kindly enter the path of the file: " fpath
				# if [ ! -f $fpath/$fname ]; then
				#    echo "Unable to locate file"
				#    exit
				# fi
				./dbdc -f CART.AWL -P 5432
				status=$?
				if [ $status -ne 0 ];then
				   echo "Unable to create tables in database"
				   exit
				fi

				echo "Create a view for data_table"
				sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
				sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
			fi

			echo "firewall reload at /etc/firewalld/firewalld.conf"

			echo "giving permission to /etc/firewalld folder"
			sudo chmod 777 /etc/firewalld

			echo "giving permission to firewalld.conf file"
			sudo chmod 777 /etc/firewalld/firewalld.conf

			res=$(sudo awk '/IndividualCalls=/ {print}' /etc/firewalld/firewalld.conf)
	
			if [ $res == 'IndividualCalls=no' ]; then
				echo "In firewalld.conf, IndividualCalls=no need to change to IndividualCalls=yes"
				sudo sed -i 's/IndividualCalls=no/IndividualCalls=yes/' /etc/firewalld/firewalld.conf
				status=$?
				if [ $status -ne 0 ];then
					echo "Not able to change the IndividualCalls status"
					else
					echo "In firewalld.conf set IndividualCalls=yes successfully"
				fi	
			else
				echo "In firewalld.conf IndividualCalls=yes already set"
			fi	

			echo "Step 5/11 Set up Configuration file for Data Logger API"
			#Create a folder as dataloggerapi in /etc/
			if [ ! -d /etc/dataloggerapi ]; then
			   sudo mkdir /etc/dataloggerapi
			fi

			#Create a folder as versionInfo in /bin/
			if [ ! -d /bin/versionInfo ]; then
				sudo mkdir /bin/versionInfo
			fi	

			#Move versionInfo exe file to /bin/versionInfo
			sudo cp versionAPI /bin/versionInfo
			status=$?
			if [ $status -ne 0 ]; then
				echo "Installation failed at Step 5A"
				exit
			fi
				
			#Move dataloggerapi configuration file to /etc/plcdatacollector
			sudo cp dataloggerapi.conf /etc/dataloggerapi
		    status=$?
		    if [ $status -ne 0 ];then
		        echo "Installation failed at step5"
		    	exit
		    fi
			
			#Create a folder as dataloggerapi in /bin/
			echo "Step 6/11 Create, Enable and Start the API Service"
			if [ ! -d /bin/dataloggerapi ]; then
			    sudo mkdir /bin/dataloggerapi
			fi
		    sudo cp dataloggerapi /bin/dataloggerapi
		    status=$?
		    if [ $status -ne 0 ];then
		   		echo "Installation failed at step6"
		   		exit
		    fi

		    #sudo cp dataloggerapi.service /etc/systemd/system
		    sudo systemctl daemon-reload
		    sudo systemctl enable $currpath/dataloggerapi.service
		    sudo systemctl start dataloggerapi.service
		fi    
    	exit
    	done
    	break;;
        No ) 
		exit;;    
    esac
done
