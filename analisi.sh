#!/bin/bash

echo " ------------------------------------------------------------------------------------------------------------ "
printf "| %-20s | %-20s | %-20s | %-20s | %s |\n" "ClIENT ID" "N PRODUCTS" "TIME IN SUPERMARKET" "TIME IN QUEUE" "N QUEUE VIEWED"
echo "|------------------------------------------------------------------------------------------------------------|"

while IFS=' ' read -r t id sm_time q_time q_viewed n_prod; do 

    if [ "$t" = "client" ]; then
        printf "| %-20s | %-20s | %-20s | %-20s | %-14s |\n" "$id" "$n_prod" "$sm_time" "$q_time" "$q_viewed"
    fi

done < $1
echo " ------------------------------------------------------------------------------------------------------------ "

echo 
echo
echo " ------------------------------------------------------------------------------------------------------------ "
printf "| %-15s | %-15s | %-15s | %-15s | %-21s | %s |\n" "CASH ID" "N PRODUCTS" "N CLIENTS" "TOT TIME OPEN" "AVERAGE SERVICE TIME" "N CLOSINGS"
echo "|------------------------------------------------------------------------------------------------------------|"

while IFS=' ' read -r t id n_prod n_clients n_closings t_4_clients t_4_openings; do 

    if [ "$t" != "cash" ]; then
        continue
    fi
    
    IFS=',' read -ra time_clients <<< "$t_4_clients";

    sum=0
    for time in $time_clients; do
        sum=$(expr $time + $sum)
    done
    service_avg=$(expr $sum / $n_clients)

    
    IFS=',' read -ra time_openings <<< "$t_4_openings";

    open_time=0
    for time in $time_openings; do
        open_time=$(expr $time + $sum)
    done  
    
    printf "| %-15s | %-15s | %-15s | %-15s | %-21s | %-10s |\n" "$id" "$n_prod" "$n_clients" "$open_time" "$service_avg" "$n_closings"

done < $1
echo " ------------------------------------------------------------------------------------------------------------"
