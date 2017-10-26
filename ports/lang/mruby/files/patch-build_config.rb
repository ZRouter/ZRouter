--- build_config.rb	2015-11-17 18:02:30.000000000 +0900
+++ build_config.rb	2016-09-24 08:18:20.000000000 +0900
@@ -22,15 +22,16 @@
   # include the default GEMs
   conf.gembox 'default'
   # C compiler settings
-  # conf.cc do |cc|
+   conf.cc do |cc|
   #   cc.command = ENV['CC'] || 'gcc'
+     cc.command = '/usr/bin/cc'
   #   cc.flags = [ENV['CFLAGS'] || %w()]
   #   cc.include_paths = ["#{root}/include"]
   #   cc.defines = %w(DISABLE_GEMS)
   #   cc.option_include_path = '-I%s'
   #   cc.option_define = '-D%s'
   #   cc.compile_options = "%{flags} -MMD -o %{outfile} -c %{infile}"
-  # end
+   end
 
   # mrbc settings
   # conf.mrbc do |mrbc|
@@ -38,8 +39,9 @@
   # end
 
   # Linker settings
-  # conf.linker do |linker|
+   conf.linker do |linker|
   #   linker.command = ENV['LD'] || 'gcc'
+     linker.command = '/usr/bin/cc'
   #   linker.flags = [ENV['LDFLAGS'] || []]
   #   linker.flags_before_libraries = []
   #   linker.libraries = %w()
@@ -48,19 +50,21 @@
   #   linker.option_library = '-l%s'
   #   linker.option_library_path = '-L%s'
   #   linker.link_options = "%{flags} -o %{outfile} %{objs} %{libs}"
-  # end
+   end
 
   # Archiver settings
-  # conf.archiver do |archiver|
+   conf.archiver do |archiver|
   #   archiver.command = ENV['AR'] || 'ar'
+     archiver.command = '/usr/bin/ar'
   #   archiver.archive_options = 'rs %{outfile} %{objs}'
-  # end
+   end
 
   # Parser generator settings
-  # conf.yacc do |yacc|
+   conf.yacc do |yacc|
+     yacc.command = '/usr/local/bin/bison'
   #   yacc.command = ENV['YACC'] || 'bison'
   #   yacc.compile_options = '-o %{outfile} %{infile}'
-  # end
+   end
 
   # gperf settings
   # conf.gperf do |gperf|
@@ -92,6 +96,10 @@
     toolchain :gcc
   end
 
+  conf.yacc do |yacc|
+    yacc.command = '/usr/local/bin/bison'
+  end
+
   enable_debug
 
   # include the default GEMs
@@ -115,6 +123,10 @@
     toolchain :gcc
   end
 
+  conf.yacc do |yacc|
+    yacc.command = '/usr/local/bin/bison'
+  end
+
   enable_debug
   conf.enable_bintest
   conf.enable_test
@@ -136,3 +148,16 @@
 #   conf.test_runner.command = 'env'
 #
 # end
+
+MRuby::CrossBuild.new('zrouter') do |conf|
+  toolchain :gcc
+  conf.yacc do |yacc|
+    yacc.command = '/usr/local/bin/bison'
+  end
+  conf.git do |git|
+    git.command = '/usr/local/bin/git'
+  end
+
+  conf.gembox 'default'
+
+end
