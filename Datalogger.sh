echo "Dataloggerapi APIs checking with json requests"

status1=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"rushikesh\",\"passcode\":\"finecho@178\",\"authkey\":\"abcd\",\"endpoint\":\"getFields\"}]")
echo "get fields"
echo $status1

status2=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"sateesh\",\"passcode\":\"123456\",\"authkey\":\"abcd\",\"endpoint\":\"fetchValues\",\"params\":[{\"start_time\":\"2022-09-01 07:06:00\",\"end_time\":\"2022-09-01 07:07:00\",\"skip\":\"600\",\"param_names\":[\"Header_Len\"]}]}]")
echo "fetch values"
echo $status2

status3=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"sateesh\",\"passcode\":\"123456\",\"authkey\":\"abcd\",\"endpoint\":\"fetchTimestamps\",\"params\":[{\"start_time\":\"2022-09-01 07:00:00\",\"end_time\":\"2022-09-01 07:10:00\",\"param_names\":[{\"name\":\"A1_PD1\",\"operation\":\">\",\"value\":\"45\"}]}]}]")
echo "fetch timestamps"
echo $status3
