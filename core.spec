Summary:	A set of configuration and setup files
Name:		core
Version:	1.1.0
Release:	0.1
License:	Public Domain, partially BSD-like
Group:		Base
Source0:	%{name}-%{version}.tar.xz
# Source0-md5:	d8a8e3293ed30dcbffd52aa2284d9721
BuildRequires:	glibc-static
AutoReqProv:	no
Provides:	/sbin/postshell
Obsoletes:	issue
Obsoletes:	setup
Provides:	issue
Provides:	setup
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%define		distname	Autumn Twilight
%define		distversion	2.1

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

%{__make} install \
	DESTDIR=$RPM_BUILD_ROOT

# /etc/issue
cat > $RPM_BUILD_ROOT%{_sysconfdir}/issue <<EOF
Freddix \r \m (\l)

EOF

# /etc/os-release
cat > $RPM_BUILD_ROOT%{_sysconfdir}/os-release <<EOF
# Operating system identification
NAME=Freddix
VERSION="%{distversion} (%{distname})"
ID=freddix
VERSION_ID="%{distversion}"
PRETTY_NAME="Freddix %{distversion} (%{distname})"
ANSI_COLOR="1;37"
HOME_URL="https://freddix.org/"

EOF

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
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/hostname
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/hosts
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/locale.conf
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/passwd
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/profile
%config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/vconsole.conf
%config(noreplace) %{_sysconfdir}/issue
%config(noreplace) %{_sysconfdir}/os-release
%config(noreplace,missingok) %verify(not md5 mtime size) %{_sysconfdir}/motd
%config(noreplace,missingok) %verify(not md5 mtime size) %{_sysconfdir}/resolv.conf
%attr(600,root,root) %config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/securetty
%ghost %{_sysconfdir}/shells
%{_sysconfdir}/mtab

