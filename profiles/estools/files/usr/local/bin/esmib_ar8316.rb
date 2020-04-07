#!/usr/local/bin/mruby

# AR8316 MIB
# 0 RxBroad
# 1 RxPause
# 2 RxMulti
# 3 RxFcsErr
# 4 RxAllignErr
# 5 RxRunt
# 6 RxFragement
# 7 Rx64Byte
# 8 Rx128Byte
# 9 Rx256Byte
# 10 Rx512Byte
# 11 Rx1024Byte
# 12 Rx1518Byte
# 13 RxMaxByte
# 14 RxTooLong
# 15 RxGoodByte (64bit)
# 16 RXBadByte (64bit)
# 17 RxOverFlow
# 18 Filtered
# 19 TxBroad
# 20 TxPause
# 21 TxMulti
# 22 TxUnderRun
# 23 Tx64Byte
# 24 Tx128Byte
# 25 Tx256Byte
# 26 Tx512Byte
# 27 Tx1024Byte
# 28 Tx1518Byte
# 29 TxMaxByte
# 30 TxOverSize
# 31 TxByte (64Bit)
# 32 TxCollision
# 33 TxAbortCol

lastfile = "/var/run/esmib_ar8316.last"

mib = ARGV[0].to_i

if mib < 15 then
  offset = mib * 4
  width = 32
elsif mib == 15 || mib == 16
  offset = 15 * 4 + (mib - 15) * 8
  width = 64
elsif mib < 31 
  offset = mib * 4 + 2 * 4
  width = 32
elsif mib == 31
  offset = 31 * 4 + 2 * 4
  width = 64
else
  offset = mib * 4 + 3 * 4
  width = 32
end

t = EtherSwitch.new(0)

t.writereg(0x80, 0x43000000)

val = Array.new

port = 0
while port < 6 do

# only use 32 bit value
  val[port] = t.readreg(0x20000 + port * 0x100 + offset)

  port = port + 1
end

if File.exist?(lastfile)
  fd = File.open(lastfile, "r")
  last = fd.read
  lastval = last.to_s.chop.split(",")
  fd.close
  port = 0
  mibstr = ""
  while port < 6 do
    if port != 0
      mibstr = mibstr + ","
    end
    mibstr = mibstr + (lastval[port].to_i + val[port]).to_s
    port = port + 1
  end
else
  port = 0
  mibstr = ""
  while port < 6 do
    if port != 0
      mibstr = mibstr + ","
    end
    mibstr = mibstr + val[port].to_s
    port = port + 1
  end
end

fd = File.open(lastfile, "w")
fd.puts mibstr
print mibstr

