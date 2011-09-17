Name:		ser2net
Version:	2.3
Release:	1
License:	GPL
Summary:	Serial to network proxy
Group:		System Environment/Daemons
Packager:	Ivan F. Martinez <ivanfm@ecodigit.com.br>
Source0:	http://prdownloads.sourceforge.net/ser2net/ser2net-%{version}.tar.gz
URL:		http://sourceforge.net/projects/ser2net/
BuildRoot:	/var/tmp/%{name}-%{version}-root
AutoReqProv:	no

%description
Make serial ports available to network via TCP/IP connection.

%prep

%setup

%build
./configure --prefix="/usr" --mandir="/usr/share/man"
make

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/etc/rc.d/init.d
install ser2net.conf $RPM_BUILD_ROOT/etc
install ser2net.init $RPM_BUILD_ROOT/etc/rc.d/init.d/ser2net
make DESTDIR=$RPM_BUILD_ROOT install

%files
%defattr(0644,root,root)
%attr(0755,root,root) /etc/rc.d/init.d/ser2net
%config(noreplace) /etc/ser2net.conf
%doc README NEWS ChangeLog COPYING INSTALL AUTHORS
%attr(0755,root,root) /usr/sbin/*
/usr/share/man/man8/*


%changelog
* Tue Dec  5 2003 Corey Minyard <minyard@acm.org>
- Moved to version 2.1.
* Tue Oct 14 2003 Corey Minyard <minyard@acm.org>
- Moved to version 2.0.
* Tue Apr 22 2003 Corey Minyard <minyard@acm.org>
- Moved to version 1.9.
* Fri Oct 10 2001 Corey Minyard <minyard@acm.org>
- Applied patches from Przemyslaw Czerpak (druzus@polbox.com), which added init
  and cleaned up a few other problems.
* Tue Jul  3 2001 Corey Minyard <minyard@acm.org>
- Fixed everything to install in the right place.
- Updated to 1.4
* Fri Jun 29 2001 Corey Minyard <minyard@acm.org>
- Updated to 1.3
- Set the prefix to "/usr" to install at root.
* Tue Jun 19 2001 Ivan F. Martinez <ivanfm@ecodigit.com.br>
- package created
