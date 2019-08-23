#!/bin/bash

clienttest1=0
clienttest2=0
clienttest3=0
stprob=0
retprob=0
delprob=0
usertime=0.0
kerneltime=0.0
starrprob=()
retarrprob=()
delarrprob=()

exec 3<testout.log
while IFS= read -u 3 line
do
  case $line in
    ("Lancio"*"TEST 1")
        clienttest1=$(($clienttest1 + 1));;
    ("Lancio"*"TEST 2")
        clienttest2=$(($clienttest2 + 1));;
    ("Lancio"*"TEST 3")
        clienttest3=$(($clienttest3 + 1));;
    ("STORE"*"FAIL")
        tmpc=${line##*"di"}
        tmpc=${tmpc%" FAIL"}
        starrprob+=(${tmpc})
        stprob=$(($stprob + 1)) ;;
    ("RETRIEVE"*"FAIL")
        tmpc=${line##*"di"}
        tmpc=${tmpc%" FAIL"}
        retarrprob+=(${tmpc})
        retprob=$(($retprob + 1)) ;;
    ("DELETE"*"FAIL")
        tmpc=${line##*"di"}
        tmpc=${tmpc%" FAIL"}
        delarrprob+=(${tmpc})
        delprob=$(($delprob + 1)) ;;
    ("Tempo in UserMode: "*)
        tmptime=${line##"Tempo in UserMode: "}
        tmptime=${tmptime%s}
        usertime=$(echo "$usertime + $tmptime" | bc) ;;
    ("Tempo in KernelMode: "*)
        tmptime=${line##"Tempo in KernelMode: "} 
        tmptime=${tmptime%s} 
        kerneltime=$(echo "$kerneltime + $tmptime" | bc) ;;
    esac
done

echo "########## REPORT MAKEFILE ##########"
echo "Client lanciati per TEST 1: $clienttest1"
echo "Client lanciati per TEST 2: $clienttest2"
echo "Client lanciati per TEST 3: $clienttest3"
echo "Client totale lanciati: $(($clienttest1+$clienttest2+$clienttest3))"
echo "Anomalie riscontrate in TEST 1: $stprob"
echo "Anomalie riscontrate in TEST 2: $retprob"
echo "Anomalie riscontrate in TEST 3: $delprob"
echo "Client che hanno avuto anomalie nella store :"
stuniq=($(printf "%s\n" "${starrprob[@]}" | sort -u))
printf "%s\n" "${stuniq[@]}"
echo "Client che hanno avuto anomalie nella retrieve:"
stuniq=($(printf "%s\n" "${retarrprob[@]}" | sort -u))
printf "%s\n" "${stuniq[@]}"
echo "Client che hanno avuto anomalie nella delete:"
stuniq=($(printf "%s\n" "${delarrprob[@]}" | sort -u))
printf "%s\n" "${stuniq[@]}"
echo "Tempo totale dei client in UserMode: $usertime"
echo "Tempo totale dei client in KernelMode: $kerneltime"
echo "mando segnale SIGUSR1 al server"
killall -s USR1 objectstoreserver.out
