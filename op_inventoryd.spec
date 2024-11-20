%global debug_package %{nil}

Name:           op_inventoryd
Version:        0.0.1
Release:        1%{?dist}
Summary:        InventoryHIPA daemon

License:        GPL
URL:            https://github.com/mboll1988/op_inventoryd.git
Source0:        %{name}-%{version}.tar.gz

Requires(pre):  shadow-utils

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  cmake
BuildRequires:  systemd
BuildRequires:  systemd-rpm-macros


%description
This package contains a simple daemon for the InventoryHIPA applicaiton ("Bestandesaufnahme")


%pre
#getent group %{groupname} >/dev/null || groupadd -r %{groupname}
#getent passwd %{username} >/dev/null || \
#    useradd -r -g %{groupname} -d %{homedir} -s /sbin/nologin \
#    -c "User used for running op_inventoryd" %{username}
#exit 0

# Section for preparation of build
%prep

# Following macro just has to be here. It unpacks the original source from
# tag.gz archive.
%setup -q


# Build section
%build


%cmake -DCMAKE_BUILD_TYPE="Release"  ../
%cmake_build


# Install section


%install
# Remove previous build results


rm -rf $RPM_BUILD_ROOT
# This macro runs make -f Makefile install and it installs
# all files to $RPM_BUILD_ROOT


%cmake_install



%postun
rm -rf /var/log/op_inventoryd
rm -rf /run/op_inventoryd

# Files section
%files
#%defattr(-,root,root,0755)

# Files and directories owned by root:root
%attr(755,root,root) %{_bindir}/op_inventoryd
%attr(644,root,root) %{_unitdir}/op_inventoryd.service

# File owned by root, but group can read it
%attr(640,root,%{groupname}) %{_sysconfdir}/op_inventoryd.conf

# Files and directories owned by op_inventoryd:op_inventoryd user
%attr(755,%{username},%{groupname}) %{_var}/log/op_inventoryd
%attr(755,%{username},%{groupname}) %{_rundir}/op_inventoryd


# This is section, where you should describe all important changes
# in RPM
%changelog
* Tue Nov 19 2024 boll_m <marco.boll@psi.ch>
-- created op_inventoryd RPM

