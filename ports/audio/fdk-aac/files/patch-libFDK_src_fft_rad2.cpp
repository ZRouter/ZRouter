--- libFDK/src/fft_rad2.cpp.org	2023-02-07 11:11:19.449976000 +0900
+++ libFDK/src/fft_rad2.cpp	2023-02-07 11:06:59.277666000 +0900
@@ -109,7 +109,8 @@
 #if defined(__arm__)
 #include "arm/fft_rad2_arm.cpp"
 
-#elif defined(__GNUC__) && defined(__mips__) && defined(__mips_dsp) && !defined(__mips16)
+//#elif defined(__GNUC__) && defined(__mips__) && defined(__mips_dsp) && !defined(__mips16)
+#elif defined(__mips__)
 #include "mips/fft_rad2_mips.cpp"
 
 #endif
