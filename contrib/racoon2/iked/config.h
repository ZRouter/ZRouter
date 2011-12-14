/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* Define if you have the daemon function.  */
#define HAVE_DAEMON 1

/* Define if you have the getaddrinfo function.  */
#define HAVE_GETADDRINFO 1

/* Define if you have the getnameinfo function.  */
#define HAVE_GETNAMEINFO 1

/* Define if you have the getopt_long function.  */
#define HAVE_GETOPT_LONG 1

/* Define if you have the setprogname function.  */
#define HAVE_SETPROGNAME 1

/* Define if you have the timegm function.  */
#define HAVE_TIMEGM 1

/* Define if you have the <inttypes.h> header file.  */
#define HAVE_INTTYPES_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <net/pfkeyv2.h> header file.  */
#define HAVE_NET_PFKEYV2_H 1

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <netinet/in.h> header file.  */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <netinet6/ipsec.h> header file.  */
/* #undef HAVE_NETINET6_IPSEC_H */

/* Define if you have the <netipsec/ipsec.h> header file.  */
#define HAVE_NETIPSEC_IPSEC_H 1

/* Define if you have the <openssl/aes.h> header file.  */
#define HAVE_OPENSSL_AES_H 1

/* Define if you have the <openssl/evp.h> header file.  */
#define HAVE_OPENSSL_EVP_H 1

/* Define if you have the <openssl/idea.h> header file.  */
/* #undef HAVE_OPENSSL_IDEA_H */

/* Define if you have the <openssl/opensslv.h> header file.  */
#define HAVE_OPENSSL_OPENSSLV_H 1

/* Define if you have the <openssl/pem.h> header file.  */
#define HAVE_OPENSSL_PEM_H 1

/* Define if you have the <openssl/pkcs12.h> header file.  */
#define HAVE_OPENSSL_PKCS12_H 1

/* Define if you have the <openssl/rc5.h> header file.  */
/* #undef HAVE_OPENSSL_RC5_H */

/* Define if you have the <openssl/rsa.h> header file.  */
#define HAVE_OPENSSL_RSA_H 1

/* Define if you have the <openssl/x509.h> header file.  */
#define HAVE_OPENSSL_X509_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdint.h> header file.  */
#define HAVE_STDINT_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/socket.h> header file.  */
#define HAVE_SYS_SOCKET_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the pcap library (-lpcap).  */
/* #undef HAVE_LIBPCAP */

/* path to the pid file */
#define IKED_PID_FILE "/var/run/iked.pid"

/* define to enable IKEv1 protocol support */
#define IKEV1 1

/* define to enable IKEv2 protocol support */
#define IKEV2 1

/* define to listen PF_ROUTE socket */
/* #undef WITH_RTSOCK */

/* define optaining CoA from netlink xfrm messages */
/* #undef WITH_PARSECOA */

/* window size of IPsec SA created by IKEv2 */
/* #undef IKEV2_IPSEC_WINDOW_SIZE */

/* define to enable NAT Traversal support */
#define ENABLE_NATT 1

/* define if __func__ macro is available */
#define HAVE_FUNC_MACRO 1

/* define if struct sockaddr has sa_len field */
#define HAVE_SA_LEN 1

/* define if IPv6 is enabled */
#define INET6 1

/* define if IPv6 advanced API is available */
#define ADVAPI 1

/* define to use ADVAPI for glibc2 */ 
/* #undef _GNU_SOURCE */

/* define to use OpenSSL ENGINE */
#define WITH_OPENSSL_ENGINE 1

/* define if public key encryption is available */
#define HAVE_SIGNING_C 1

/* recent versions of openssl declares argument with 'const' */
#define BPP_const const

/* define if openssl has SHA2 support */
#define HAVE_OPENSSL_SHA2 1

/* define if SHA2 can be used */
#define WITH_SHA2 1

