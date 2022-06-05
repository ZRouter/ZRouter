#!/usr/local/bin/mruby

# RTL8366SR MIB
#
#        0               IfInOctets
#        1               EtherStatsOctets
#        2               EtherStatsUnderSizePkts
#        3               EtherFregament
#        4               EtherStatsPkts64Octets
#        5               EtherStatsPkts65to127Octets
#        6               EtherStatsPkts128to255Octets
#        7               EtherStatsPkts256to511Octets
#        8               EtherStatsPkts512to1023Octets
#        9               EtherStatsPkts1024to1518Octets
#        10              EtherOversizeStats
#        11              EtherStatsJabbers
#        12              IfInUcastPkts
#        13              EtherStatsMulticastPkts
#        14              EtherStatsBroadcastPkts
#        15              EtherStatsDropEvents
#        16              Dot3StatsFCSErrors
#        17              Dot3StatsSymbolErrors
#        18              Dot3InPauseFrames
#        19              Dot3ControlInUnknownOpcodes
#        20              IfOutOctets
#        21              Dot3StatsSingleCollisionFrames
#        22              Dot3StatMultipleCollisionFrames
#        23              Dot3sDeferredTransmissions
#        24              Dot3StatsLateCollisions
#        25              EtherStatsCollisions
#        26              Dot3StatsExcessiveCollisions
#        27              Dot3OutPauseFrames
#        28              Dot1dBasePortDelayExceededDiscards
#        29              Dot1dTpPortInDiscards
#        30              IfOutUcastPkts
#        31              IfOutMulticastPkts
#        32              IfOutBroadcastPkts
#        33              Dot1dTpLearnEntryDiscardFlag    
#
# IfInOctets and EtherStatsOctets is 64bit wide other is 32bit.

mib = ARGV[0].to_i

t = EtherSwitch.new(0)

portoff = 0

srid = t.readreg(0x0105)
rbid = t.readreg(0x0509)

if srid == 0x8366 then
# RTL8366SR
  portoff = 0x0040
  ctrlreg = 0x11F0
elsif rbid == 0x5937 then
# RTL8366RB
  portoff = 0x0050
  ctrlreg = 0x13F0
else
  p "switch not found"
end


if portoff != 0 then

  port = 0

  while port < 6 do

    if mib <= 1 then
      off = 0x1000 + port * portoff + 4 * mib
    else
      off = 0x1000 + port * portoff + 8 + (mib - 2) * 2
    end

    t.writereg(off, 0)

    st = t.readreg(ctrlreg)

    if st & 3 == 0 then
      val = 0
      if mib <= 1 then
        c = 0
        while c < 4 do
          if c == 0 then   # only use 30 bit
            val = t.readreg(off + c)
          elsif c == 1 then
            val = ((t.readreg(off + c) & 0x3fff) << 16) | val
          else
            dmy = t.readreg(off + c)
          end
          c = c + 1
        end
      else
        c = 0
        while c < 2 do
          if c == 0 then   # only use 30 bit
            val = t.readreg(off + c)
          else
            val = ((t.readreg(off + c) & 0x3fff) << 16) | val
          end
          c = c + 1
        end
      end
    else
      val = 0
    end

    if port != 0 then
      print ","
    end
    print val.to_s

    port = port + 1
  end
end
