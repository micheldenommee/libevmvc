#!/bin/bash

agent_count=50
load_count=1000
for ((i=1; i<=agent_count; i++))
do
  curl -s "http://127.0.0.1:8080/views/index?[1-$load_count]" \
    "http://127.0.0.1:8080/html/css/uikit.min.css?[1-$load_count]" \
    "http://127.0.0.1:8080/html/css/app.css?[1-$load_count]" \
    "http://127.0.0.1:8080/html/images/login-logo.jpg?[1-$load_count]" \
    >/dev/null &
  pidlist="$pidlist $!" 
done

FAIL=0
for p in $pidlist
do 
  echo $p
  wait $p || let "FAIL+=1" 
done  

echo "sended '$((load_count*agent_count*4))' requests"
if [ "$FAIL" == "0" ]; then 
  echo "All query succeeded!" 
else 
  echo "FAIL! ($FAIL)" 
fi
