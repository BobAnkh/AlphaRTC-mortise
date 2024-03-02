#!/bin/bash
sudo killall peerconnection_gcc 2> /dev/null

rm ./receiver.log; ./peerconnection_gcc ./config/receiver_gcc-$2.json 2>receiver_warn.log &

sleep 1s

mm-delay 20 mm-link ./traces/$1 ./traces/$1 --uplink-queue=droptail --downlink-queue=droptail --uplink-queue-args="packets=200" --downlink-queue-args="packets=200" -- scripts/sender.sh $2

# tcpstat -o "%b\n" -r tmp.pcap 0.5 -f "src 100.64.0.4" |  awk 'BEGIN {cnt=0} {print cnt *0.5, $1 / 1024.0 / 1024.0; cnt+=1}' > log/tmp_rate.log
#  tcptrace -R tmp.pcap   -f "hostaddr=100.64.0.4" && grep dot b2a_rtt.xpl | awk 'BEGIN{start=0;} {if (start==0) start = $2; print $2-start, $3}' > log/tmp_rtt.log

# cd .. && ./evaluate.sh run/videos/720p-20s.yuv  run/videos/outvideo.yuv  ./run/sender_warn.log
