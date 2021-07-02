MRuby::CrossBuild.new('zrouter') do |conf|
  if ENV['ZTARGET'] == 'mips'
    toolchain :gcc
  else
    toolchain :clang
  end
  conf.cc do |cc|
    cc.command = 'cc'
    cc.include_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/include')
  end
  conf.linker do |linker|
    linker.command = 'cc'
    linker.library_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/lib')
  end
  conf.archiver do |archiver|
    archiver.command = 'ar'
  end

  conf.gembox 'default'

end
