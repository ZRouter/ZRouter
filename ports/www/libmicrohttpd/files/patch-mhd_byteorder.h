--- src/microhttpd/mhd_byteorder.h.org	2018-09-25 14:35:57.912026000 +0900
+++ src/microhttpd/mhd_byteorder.h	2018-09-25 14:36:20.873119000 +0900
@@ -148,6 +148,7 @@
 
 #endif /* !_MHD_BYTE_ORDER */
 
+#if 0
 #ifdef _MHD_BYTE_ORDER
 /* Some safety checks */
 #if defined(WORDS_BIGENDIAN) && _MHD_BYTE_ORDER != _MHD_BIG_ENDIAN
@@ -156,5 +157,6 @@
 #error Configure did not detect big endian byte order but headers specify big endian byte order
 #endif /* !WORDS_BIGENDIAN && _MHD_BYTE_ORDER == _MHD_BIG_ENDIAN */
 #endif /* _MHD_BYTE_ORDER */
+#endif
 
 #endif /* !MHD_BYTEORDER_H */
