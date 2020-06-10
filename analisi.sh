#!/bin/bash

echo " ------------------------------------------------------------------------------------------------------------ "
printf "| %-20s | %-20s | %-20s | %-20s | %s |\n" "ClIENT ID" "N PRODUCTS" "TIME IN SUPERMARKET" "TIME IN QUEUE" "N QUEUE VIEWED"
echo "|------------------------------------------------------------------------------------------------------------|"

while IFS=' ' read -r t id sm_time q_time q_viewed n_prod; do

    if [ "$t" != "client" ]; then
        continue
    fi

    sm_time_sec=$(echo "scale=3; $sm_time / 1000" | bc)
    q_time_sec=$(echo "scale=3; $q_time / 1000" | bc)
    printf "| %-20s | %-20s | %-20s | %-20s | %-14s |\n" "$id" "$n_prod" "$sm_time_sec" "$q_time_sec" "$q_viewed"

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

    if [ "$n_clients" -eq "0" ]; then
        service_avg=""
    else
        IFS="," read -ra time_clients <<< "$t_4_clients";
        sum=0
        for ((i = 0; i < $n_clients; i++)); do
            sum=$(expr ${time_clients[$i]} + $sum)
        done
        service_avg=$(expr $sum / $n_clients)
        service_avg=$(echo "scale=3; $service_avg / 1000" | bc)
    fi


    if [ "$n_closings" -eq "0" ]; then
        open_time=""
    else
        IFS=',' read -ra time_openings <<< "$t_4_openings";
        open_time=0
        for ((i = 0; i<$n_closings; i++)); do
            open_time=$(expr ${time_openings[$i]} + $open_time)
        done
        open_time=$(echo "scale=3; $open_time / 1000" | bc)
    fi
    
    printf "| %-15s | %-15s | %-15s | %-15s | %-21s | %-10s |\n" "$id" "$n_prod" "$n_clients" "$open_time" "$service_avg" "$n_closings"

done < $1
echo " ------------------------------------------------------------------------------------------------------------"
