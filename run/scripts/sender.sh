# sudo tcpdump -i ingress -w tmp.pcap &
sed "/dest_ip/c\           \"dest_ip\":  \"$MAHIMAHI_BASE\"," -i ./config/sender_gcc-$1.json 
rm ./sender.log; ./peerconnection_gcc ./config/sender_gcc-$1.json 2>sender_warn.log