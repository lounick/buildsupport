GCCVERSION := $(shell gcc -v 2>&1 | grep ada | wc -l)
UNAME := $(shell uname)
VERSION=0.1

CC=gcc
exec = buildsupport

sources = $(wildcard ada/*.ad? c/*.c include/*.h)

all: checkVersion $(exec)

checkVersion:
#ifeq ($(UNAME),Linux)
#	@if [ $(GCCVERSION) -ne 1 ] ; then echo gcc must point to GNAT... check your PATH ; exit 1 ; fi
#endif

$(exec): $(sources)
ifeq ($(UNAME), Linux)
	@echo "package Buildsupport_Version is" > ada/buildsupport_version.ads.new
	@echo -n "   Buildsupport_Release : constant String := \"" >> ada/buildsupport_version.ads.new
	@git log --oneline | head -1 | cut -f1 -d' ' | tr -d '\012' >> ada/buildsupport_version.ads.new
	@echo "\";" >> ada/buildsupport_version.ads.new
	@echo -n "end Buildsupport_Version;" >> ada/buildsupport_version.ads.new
	@if [ ! -f "ada/buildsupport_version.ads" ] ; then                \
		mv ada/buildsupport_version.ads.new ada/buildsupport_version.ads;          \
	else                                            \
		MD1=`cat ada/buildsupport_version.ads | md5sum` ;         \
		MD2=`cat ada/buildsupport_version.ads.new | md5sum` ;     \
		if [ "$$MD1" != "$$MD2" ] ; then        \
			mv ada/buildsupport_version.ads.new ada/buildsupport_version.ads ;  \
		else                                    \
			rm ada/buildsupport_version.ads.new ;             \
		fi ;                                    \
	fi
endif
	ADA_PROJECT_PATH=`ocarina-config --prefix`/lib/gnat:$$ADA_PROJECT_PATH $(gnatpath)gprbuild -x -g $(exec) -p -P buildsupport.gpr -XBUILD="debug"

install:
	$(MAKE)
	cp buildsupport `ocarina-config --prefix`/bin/
	cp misc/driveGnuPlotsStreams.pl `ocarina-config --prefix`/bin/

clean:
	rm -rf tmpBuild $(exec) *~

.PHONY: install clean
