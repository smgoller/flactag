VERSION=1.0-RC1

CXXFLAGS=-Wall -Werror -DVERSION=\"${VERSION}\"

FLACTAGOBJS=flactag.o Album.o Track.o AlbumWindow.o TrackWindow.o FlacInfo.o \
						TagName.o TagsWindow.o CuesheetTrack.o Cuesheet.o DiskIDCalculate.o \
						sha1.o base64.o ScrollableWindow.o ConfigFile.o MusicBrainzInfo.o \
						FileNameBuilder.o ErrorLog.o CommandLine.o CoverArt.o
						
DISCIDOBJS=discid.o

SRCS=$(FLACTAGOBJS:.o=.cc) $(DISCIDOBJS:.o=.cc) 

all: flactag discid flactag.txt flactag.html flactag.man

flactag.man: manpage.sgml Makefile
	sgml2txt -man manpage.sgml
	mv manpage.man flactag.man
	
flactag.txt: flactag.sgml Makefile
	sgml2txt --pass="-P-bc" --blanks=1 flactag.sgml
	
flactag.html: flactag.sgml Makefile
	sgml2html --split=0 --toc=1 flactag.sgml
	
clean:
	rm -f $(FLACTAGOBJS) $(DISCIDOBJS) flactag.txt flactag.html *.d *.bak *~ *.tar.gz flactag discid

flactag-$(VERSION).tar.gz: all
	svn update && cd .. && tar zcf flactag/flactag-$(VERSION).tar.gz \
							flactag/*.cc flactag/*.h flactag/Makefile flactag/flactag.txt \
							flactag/flactag.html flactag/flactag.sgml flactag/COPYING \
							flactag/ripflac.sh flactag/checkflac.sh flactag/fixtoc.sed

install-webpages: flactag-$(VERSION).tar.gz flactag.html
	mkdir -p /auto/gentlyweb/flactag
	cp flactag.html /auto/gentlyweb/flactag/index.html
	cp flactag.html /auto/gentlyweb/flactag/
	cp flactag-$(VERSION).tar.gz  /auto/gentlyweb/flactag
	
%.d: %.cc
	@echo DEPEND $< $@
	@$(CXX) -MM $(CXXFLAGS) $< | \
        sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

flactag: $(FLACTAGOBJS)
	g++ -o $@ -lslang -lmusicbrainz -lFLAC++ -lhttp_fetcher -lunac $^
	
discid: $(DISCIDOBJS)
	g++ -o $@ -lmusicbrainz $^
	
include $(SRCS:.cc=.d)
