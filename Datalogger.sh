echo "Dataloggerapi APIs checking with json requests"

status1=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"rushikesh\",\"passcode\":\"finecho@178\",\"authkey\":\"abcd\",\"endpoint\":\"getFields\"}]")
echo "get fields"
echo $status1

status2=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"sateesh\",\"passcode\":\"123456\",\"authkey\":\"abcd\",\"endpoint\":\"fetchValues\",\"params\":[{\"start_time\":\"2021-06-23 16:19:04\",\"end_time\":\"2021-06-23 16:24:04\",\"skip\":\"600\",\"param_names\":[\"A1_PD1\",\"A1_PD2\",\"A1_PD5\",\"A1_PD8\"]}]}]")
echo "fetch values"
echo $status2

status3=$(./clientNew 127.0.0.1 6789 "[{\"username\":\"sateesh\",\"passcode\":\"123456\",\"authkey\":\"abcd\",\"endpoint\":\"fetchTimestamps\",\"params\":[{\"start_time\":\"2021-06-23 16:03:04\",\"end_time\":\"2021-06-23 16:24:04\",\"param_names\":[{\"name\":\"Header_Len\",\"operation\":\">\",\"value\":\"45\"}]}]")
echo "fetch timestamps"
echo $status3

