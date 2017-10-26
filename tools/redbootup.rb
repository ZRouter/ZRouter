# need boot delay to 5 sec
# example usage
# ruby redbootup.rb f 192.168.1.1 Fon_FON2201
# command
# f = flash at autoboot F = flash at not autoboot
# e = flash at autoboot E = flash at not autoboot


require 'net/ping'
require 'net/telnet'

if ARGV.size != 3 then
  print "Usage: ruby redbootup.rb <command> <target ip address> <build name>\n"
  exit
end

addr = ARGV[1]
port = 9000

target = ARGV[2]

pinger = Net::Ping::External.new(addr)

while pinger.ping == false do
  sleep 1
  print "."
end

print "\nstart telnet\n"

telnet = Net::Telnet.new("Host" => addr, "Port" => port,
                         "Timeout" => 3600, "Prompt" => /^RedBoot>/n)

if ARGV[0] == "f" || ARGV[0] == "e" then
  telnet.puts("\003")
end

if ARGV[0] == "f" || ARGV[0] == "F" then

  telnet.cmd("fis init -f") {|c| print c}

  STDOUT.flush

  telnet.cmd("Match" => /^.*continue \(y\/n\)\?/n, "String" => "y") {|c| print c}

  STDOUT.flush

  cmd = "load -r -v -b 0x80050000 " + target + "_kernel.gz.sync"

  telnet.cmd(cmd) {|c| print c}

  STDOUT.flush

  telnet.cmd("fis create -e 0x80050100 kernel") {|c| print c}

  STDOUT.flush

  cmd = "load -r -v -b 0x80050000 " + target + "_rootfs_clean.iso.ulzma"

  telnet.cmd(cmd) {|c| print c}

  STDOUT.flush

  telnet.cmd("fis create -e 0x00000000 rootfs") {|c| print c}

  STDOUT.flush

  telnet.cmd("fis list") {|c| print c}

  STDOUT.flush

elsif ARGV[0] == "e" || ARGV[0] == "E" then

  cmd = "load " + target + "_kernel"

  telnet.cmd(cmd) {|c| print c}

  STDOUT.flush

  telnet.cmd("exec") {|c| print c}

  STDOUT.flush

  if ARGV[0] == "e" then
    telnet.cmd("exec") {|c| print c}

    STDOUT.flush
  end

end

telnet.close
