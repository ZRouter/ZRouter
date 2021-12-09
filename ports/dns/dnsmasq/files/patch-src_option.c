--- src/option.c.org	2021-09-09 05:21:22.000000000 +0900
+++ src/option.c	2021-12-09 17:12:51.550289000 +0900
@@ -2442,7 +2442,8 @@
           arg += 9;
           if (strlen(arg) != 16)
               ret_err(gen_err);
-          for (char *p = arg; *p; p++) {
+          char *p;
+          for (p = arg; *p; p++) {
             if (!isxdigit((int)*p))
               ret_err(gen_err);
           }
@@ -2450,7 +2451,8 @@
 
           u8 *u = daemon->umbrella_device;
           char word[3];
-          for (u8 i = 0; i < sizeof(daemon->umbrella_device); i++, arg+=2) {
+          u8 i;
+          for (i = 0; i < sizeof(daemon->umbrella_device); i++, arg+=2) {
             memcpy(word, &(arg[0]), 2);
             *u++ = strtoul(word, NULL, 16);
           }
