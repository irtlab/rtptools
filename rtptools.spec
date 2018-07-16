Summary: RTP Tools
Name: rtptools
Version: VERSION
Release: 1
License: BSD
URL: https://github.com/columbia-irt/rtptools
Source0: rtptools-VERSION.tar.gz

%description
The rtptools distribution consists of a number of small applications that
can be used for processing RTP data.

See http://www.cs.columbia.edu/IRT/software/rtptools

Henning Schulzrinne schulzrinne@cs.columbia.edu
Xiaotao Wu
Akira Tsukamoto

%prep
%setup
%build
./configure
make

%install
make BINDIR=$RPM_BUILD_ROOT/usr/local/bin MANDIR=$RPM_BUILD_ROOT%{_mandir} install

%files
%{_mandir}
/usr/local/bin
