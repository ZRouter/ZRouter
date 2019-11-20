--- build_config.rb.org	2019-11-20 16:30:15.104464000 +0900
+++ build_config.rb	2019-11-20 17:06:40.550091000 +0900
@@ -150,3 +150,25 @@
 #
 #   conf.test_runner.command = 'env'
 # end
+
+MRuby::CrossBuild.new('zrouter') do |conf|
+  if ENV['ZTARGET'] == 'mips'
+    toolchain :gcc
+  else
+    toolchain :clang
+  end
+  conf.cc do |cc|
+    cc.command = 'cc'
+    cc.include_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/include')
+  end
+  conf.linker do |linker|
+    linker.command = 'cc'
+    linker.library_paths << (ENV['ZWORLDDESTDIR'] + '/usr/local/lib')
+  end
+  conf.archiver do |archiver|
+    archiver.command = 'ar'
+  end
+
+  conf.gembox 'default'
+
+end
