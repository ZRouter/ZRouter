#!/bin/sh



for link in `find . -type l` ; do 
	lk=`readlink $link`; 
	TEST=`echo $lk | sed -E "s/^(.).*/\\1/"` ;
	if [ "x${TEST}" = "x/" ] ; then
		new=`echo $lk | sed "s/^\//\.\.\/\.\.\//"`; 
		echo ln -sf $new $link ;
		ln -sf $new $link ;
	fi
done
