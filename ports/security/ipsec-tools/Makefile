# Created by: vanhu
# $FreeBSD: head/security/ipsec-tools/Makefile 485900 2018-11-25 20:35:22Z olivier $

# TODO: - libipsec issue ?
#       - cleanup...
#       - SYSCONFDIR
#       - $LOCALBASE/sbin/setkey Vs /usr/sbin/setkey

PORTNAME=	ipsec-tools
PORTVERSION=	0.8.2
PORTREVISION=	7
CATEGORIES=	security
MASTER_SITES=	SF
EXTRACT_SUFX=	.tar.bz2

MAINTAINER=	ports@FreeBSD.org
COMMENT=	KAME racoon IKE daemon, ipsec-tools version

#LICENSE=	BSD3CLAUSE

CONFLICTS=	racoon-[0-9]*

INSTALL_TARGET=	install-strip

#USES=		libtool tar:bzip2 ssl
USE_RC_SUBR=	racoon
GNU_CONFIGURE=	yes
USE_LDCONFIG=	yes
CONFIGURE_ARGS=	--enable-shared --sysconfdir=${PREFIX}/etc/racoon \
		--localstatedir=${STATEDIR:S/\/racoon//} \
		--with-pkgversion=freebsd-${PORTVERSION}
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

STATEDIR=	/var/db/racoon
SUB_LIST+=	STATEDIR=${STATEDIR}
PLIST_SUB+=	STATEDIR=${STATEDIR}

#OPTIONS_DEFINE=	DEBUG IPV6 ADMINPORT STATS DPD NATT NATTF FRAG HYBRID PAM \
#		RADIUS LDAP GSSAPI SAUNSPEC RC5 IDEA DOCS EXAMPLES WCPSKEY
#OPTIONS_DEFAULT=	ADMINPORT DEBUG DPD NATT FRAG HYBRID WCPSKEY

ADMINPORT_DESC=	Enable Admin port
STATS_DESC=	Statistics logging function
DPD_DESC=	Dead Peer Detection
NATT_DESC=	NAT-Traversal (kernel-patch required before 11.1)
NATTF_DESC=	require NAT-Traversal (fail without kernel-patch)
FRAG_DESC=	IKE fragmentation payload support
HYBRID_DESC=	Hybrid, Xauth and Mode-cfg support
SAUNSPEC_DESC=	Unspecified SA mode
RC5_DESC=	RC5 encryption (patented)
IDEA_DESC=	IDEA encryption (patented)
PAM_DESC=	PAM authentication (Xauth server)
RADIUS_DESC=	Radius authentication (Xauth server)
LDAP_DESC=	LDAP authentication (Xauth server)
WCPSKEY_DESC=	Allow wildcard matching for pre-shared keys

PORTDOCS=	*
PORTEXAMPLES=	*

DEBUG_CONFIGURE_ENABLE=	debug
IPV6_CONFIGURE_ENABLE=	ipv6
ADMINPORT_CONFIGURE_ENABLE=adminport
STATS_CONFIGURE_ENABLE=	stats
DPD_CONFIGURE_ENABLE=	dpd
NATTF_VARS=		NATT=yes
NATTF_VARS_OFF=		NATT=kernel
NATT_CONFIGURE_ON=	--enable-natt=${NATT} --enable-natt-versions=rfc
NATT_CONFIGURE_OFF=	--disable-natt
FRAG_CONFIGURE_ENABLE=	frag
HYBRID_CONFIGURE_ENABLE=hybrid
PAM_CONFIGURE_WITH=	libpam
GSSAPI_USES=		iconv
GSSAPI_CFLAGS=		-I${LOCALBASE}/include
GSSAPI_LDFLAGS=		-L${LOCALBASE}/lib
GSSAPI_CONFIGURE_ENABLE=gssapi
RADIUS_CONFIGURE_WITH=	libradius
LDAP_USE=		OPENLDAP=yes
LDAP_CONFIGURE_ON=	--with-libldap=${LOCALBASE}
LDAP_CONFIGURE_OFF=	--without-libldap
SAUNSPEC_CONFIGURE_ENABLE=	samode-unspec
RC5_CONFIGURE_ENABLE=		rc5
IDEA_CONFIGURE_ENABLE=		idea
WCPSKEY_EXTRA_PATCHES=		${FILESDIR}/wildcard-psk.diff
NATT_EXTRA_PATCHES=		${FILESDIR}/natt.diff
EXTRA_PATCHES+=		${FILESDIR}/wildcard-psk.diff
EXTRA_PATCHES+=		${FILESDIR}/natt.diff

CONFIGURE_ARGS+=--enable-adminport --enable-dpd --enable-natt=yes --enable-natt-versions=rfc --enable-frag  --enable-hybrid

#.include <bsd.port.pre.mk>

# Need to be patched for openssl-1.1.1 (default after 1200080)
#.if ${OPSYS} == FreeBSD
#.  if ${OSVERSION} >= 1200085 && ${SSL_DEFAULT} == base
BUILD_DEPENDS=	automake>=0:devel/automake
#.  endif
#.endif

post-patch:
	@${REINPLACE_CMD} -e "s/-Werror//g ; s/-R$$libdir/-Wl,-rpath=$$libdir/g" ${WRKSRC}/configure

# Need to be patched for openssl-1.1.1 (default after 1200080)
#.if ${OPSYS} == FreeBSD
#.  if ${OSVERSION} >= 1200085 && ${SSL_DEFAULT} == base
post-configure:
	@${REINPLACE_CMD} -e "s/automake-1.14/automake-1.16/g" ${WRKSRC}/Makefile ${WRKSRC}/*/Makefile \
				${WRKSRC}/*/*/Makefile
	@${REINPLACE_CMD} -e "s/aclocal-1.14/aclocal-1.16/g" ${WRKSRC}/Makefile ${WRKSRC}/*/Makefile \
				${WRKSRC}/*/*/Makefile
#.  endif
#.endif

#post-install:
#	@${MKDIR} ${STAGEDIR}/${PREFIX}/etc/racoon
#	@if [ -z `/sbin/sysctl -a | ${GREP} -q ipsec && ${ECHO_CMD} ipsec` ]; then \
#	    ${ECHO_MSG} "WARNING: IPsec feature is disabled on this host"; \
#	    ${ECHO_MSG} "         You must build the kernel if you want to run racoon on the host"; \
#	fi ;

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/sbin/racoon ${ZPREFIX}/sbin/racoon
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/sbin/racoonctl ${ZPREFIX}/sbin/racoonctl
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/sbin/setkey ${ZPREFIX}/sbin/setkey
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libracoon.so.0 ${ZPREFIX}/lib/libracoon.so.0
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libipsec.so.0 ${ZPREFIX}/lib/libipsec.so.0
	cd ${ZPREFIX}/lib;ln -s libracoon.so.0 libracoon.so;ln -s libipsec.so.0 libipsec.so

.include <bsd.port.mk>
post-install-EXAMPLES-on:
	@${MKDIR} ${STAGEDIR}/${EXAMPLESDIR}
	@${RM} ${WRKSRC}/src/racoon/samples/*.in
	@${CP} -r ${WRKSRC}/src/racoon/samples/* ${STAGEDIR}/${EXAMPLESDIR}

post-install-DOCS-on:
	@${MKDIR} ${STAGEDIR}/${DOCSDIR}
	${INSTALL_DATA} ${WRKSRC}/src/racoon/doc/* ${STAGEDIR}/${DOCSDIR}

#.if ${OPSYS} == FreeBSD
#.  if ${OSVERSION} >= 1200085 && ${SSL_DEFAULT} == base
EXTRA_PATCHES+=	${FILESDIR}/extra-patch-ipsec-tools
PLIST_FILES+=	include/racoon/openssl_compat.h
#.  endif
#.endif

#.include <bsd.port.post.mk>
