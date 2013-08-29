Name:       wrath
Summary:    libWRATH_Qt
Version:    1.0
Release:    1
Group:      System/Libraries
License:    GPL
URL:        http://www.nomovok.com
Source0:    wrath-1.0.tar.gz
Patch0: boost_locale.patch

BuildRequires: flex
BuildRequires: qt-devel >= 4.8
BuildRequires: pkgconfig(QtOpenGL)
BuildRequires: freetype
BuildRequires: freetype-devel 
BuildRequires: boost-devel
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(fontconfig)


%description
WRATH Qt library

%prep
%setup -q -n %{name}-%{version}
%patch0 -p1

%build
#make wrath-lib-qt
make wrath-lib-qt %{?_smp_mflags}

%install
#rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/lib
install release/libWRATH_Qt_Release.so* $RPM_BUILD_ROOT/usr/lib

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
/usr/lib/libWRATH_Qt_Release.so*
