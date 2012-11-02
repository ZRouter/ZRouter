
--[[
ipfw delete 00500 00600 00610 00620 00630
tun0_gw=`ifconfig tun0 | grep inet | awk '{print $4;}'`
tun1_gw=`ifconfig tun1 | grep inet | awk '{print $4;}'`
ipfw add 00500 check-state
ipfw add 00600 prob 0.500000 skipto 620 ip from 192.168.4.0/24 to not me keep-state
ipfw add 00610 skipto 630 ip from 192.168.4.0/24 to not me keep-state
ipfw add 00620 fwd $tun0_gw ip from 192.168.4.0/24 to not me
ipfw add 00630 fwd $tun1_gw ip from 192.168.4.0/24 to not me

]]