--- mini_httpd.c.org	2018-10-27 04:47:50.000000000 +0900
+++ mini_httpd.c	2023-02-26 08:23:47.324542000 +0900
@@ -1842,7 +1842,7 @@
 	    /* Interposer process. */
 	    (void) close( p[0] );
 	    cgi_interpose_input( p[1] );
-	    finish_request( 0 );
+	    exit ( 0 );
 	    }
 	(void) close( p[1] );
 	if ( p[0] != STDIN_FILENO )
