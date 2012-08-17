Summary:	A set of configuration and setup files
Name:		core
Version:	1.0
Release:	0.4
License:	Public Domain, partially BSD-like
Group:		Base
Source0:	%{name}-%{version}.tar.xz
# Source0-md5:	8418bc9d53a61758f4321e23bb30c7c7
BuildRequires:	glibc-static
AutoReqProv:	no
Provides:	/sbin/postshell
Provides:	setup
Obsoletes:	setup
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%define		specflags	-Os

%description
A set of configuration and setup files.

%prep
%setup -q

%build
%{__make} \
	CC="%{__cc}"					\
	LDFLAGS="%{rpmcflags} %{rpmldflags} -static"	\
	OPT_FLAGS="%{rpmcflags}"

%install
rm -rf $RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT/sbin

%{__make} install \
	DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%triggerpostun -p /usr/sbin/postshell -- %{name} < %{version}-%{release}
/usr/sbin/joinpasswd

%files
%defattr(644,root,root,755)
%doc README.asciidoc
%attr(755,root,root) %{_sbindir}/delpasswd
%attr(755,root,root) %{_sbindir}/joinpasswd
%attr(755,root,root) %{_sbindir}/postshell
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/fstab
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/group
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/host.conf
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/hosts
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/passwd
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/profile
%config(noreplace,missingok) %verify(not md5 mtime size) %{_sysconfdir}/filesystems
%config(noreplace,missingok) %verify(not md5 mtime size) %{_sysconfdir}/motd
%config(noreplace,missingok) %verify(not md5 mtime size) %{_sysconfdir}/resolv.conf
%attr(600,root,root) %config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/securetty
%ghost %{_sysconfdir}/shells
%{_sysconfdir}/mtab

