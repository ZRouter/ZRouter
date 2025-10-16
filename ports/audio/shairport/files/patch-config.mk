--- /dev/null	2025-07-06 17:40:36.053763000 +0900
+++ config.mk	2025-07-06 17:39:54.072375000 +0900
@@ -0,0 +1,3 @@
+CONFIG_OUT123=yes
+CFLAGS+= -I${ZPREFIX}/include 
+LDFLAGS+= -lm -lpthread -L${ZPREFIX}/lib -lout123
