#!/usr/local/bin/mruby

mib = ARGV[0].to_i

t = EtherSwitch.new(0)

for num in [0,1,2,3,4,6] do
  if num != 0
  print ","
  end
  print t.readreg(0x4000 + mib * 4 + 0x100 * num)
end
