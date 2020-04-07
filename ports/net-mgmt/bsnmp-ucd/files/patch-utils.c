--- utils.c.org	2020-03-23 17:30:36.678846000 +0900
+++ utils.c	2020-03-23 17:32:56.544750000 +0900
@@ -38,9 +38,12 @@
 sysctlval(const char *name, u_long *val)
 {
 	size_t len;
+	uint64_t val64;
 
 	*val = 0;
-	len = sizeof(val);
-  	if (sysctlbyname(name, val, &len, NULL, 0) != 0)
+	len = sizeof(val64);
+  	if (sysctlbyname(name, &val64, &len, NULL, 0) != 0)
 		syslog(LOG_WARNING, "%s(\"%s\"): %m", __func__, name);
+	else
+		*val = (u_long)val64;
 }
