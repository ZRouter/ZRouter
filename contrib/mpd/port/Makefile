
VERSION!=	cat ../src/Makefile | grep ^VERSION | awk '{ print $$2 }'
HTMLNUM!=	( cd ../mpd-${VERSION}/doc && ls *[0-9]*.html | 	\
		sed -e 's/mpd//' -e 's/\.html//' | sort -n | 		\
		tail -n 1 | sed -e 's/^/jot /' -e 's/$$/ 1/' )

PDIR=		port.temp

MAKEFILE=	${PDIR}/Makefile
DESCR=		${PDIR}/pkg-descr
DISTINFO=	${PDIR}/distinfo
RCSCRIPT=	${PDIR}/files/mpd5.sh.in

port:
		rm -rf ${PDIR}
		mkdir -p ${PDIR}
		mkdir -p ${PDIR}/files
		cp DESCR ${DESCR}
		cp mpd5.sh ${RCSCRIPT}
		( cd .. && stat -f "SIZE (mpd5/mpd-${VERSION}.tar.gz) = %z" \
		  mpd-${VERSION}.tar.gz ) > ${DISTINFO}
		( cd .. && md5 mpd-${VERSION}.tar.gz ) \
		  | sed 's,mpd-,mpd5/mpd-,g' >> ${DISTINFO}
		( cd .. && sha256 mpd-${VERSION}.tar.gz ) \
		  | sed 's,mpd-,mpd5/mpd-,g' >> ${DISTINFO}
		sed -e 's/@VERSION@/${VERSION}/g' \
		    -e 's/@HTMLNUM@/${HTMLNUM}/g' \
		  < Makefile.in > ${MAKEFILE}
		( cd ${PDIR} && tar cvf - * | gzip --best ) > ../port.tgz
		rm -rf ${PDIR}

clean cleandir:
		rm -f ../port.tgz

