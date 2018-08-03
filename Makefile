UNAME := $(shell uname)
ARCH := $(shell getconf LONG_BIT)
DEBIAN := $(shell uname -a | grep Debian)
CC=gcc
exec = buildsupport

all: build

build:
ifeq ($(UNAME), Linux)
	@echo "package Buildsupport_Version  is" > ada/buildsupport_version.ads.new
	@echo "   pragma Style_Checks (Off);" >> ada/buildsupport_version.ads.new
	@echo "   Buildsupport_Release : constant String :=" >> ada/buildsupport_version.ads.new
	@echo -n "      \"" >> ada/buildsupport_version.ads.new
	@git log --oneline | head -1 | cut -f1 -d' ' | tr -d '\012' >> ada/buildsupport_version.ads.new
	@echo " ; Commit " | tr -d '\r\n' >> ada/buildsupport_version.ads.new
	@git log | head -3 | tail -1 | cut -f1 -d"+" | tr -d '\r\n' >>  ada/buildsupport_version.ads.new
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
	@#[ "$(DEBIAN)" != "" ] && [ $(ARCH) == 64 ] && EXTRAFLAG="--target=x86_64-linux" ; \

	OCARINA_PATH=`ocarina-config --prefix` \
            $(gnatpath)gprbuild -x -g $(exec) -p -P buildsupport.gpr -XBUILD="debug" $$EXTRAFLAG -j4

install:
	$(MAKE)
	cp buildsupport `ocarina-config --prefix`/bin/
	cp misc/driveGnuPlotsStreams.pl `ocarina-config --prefix`/bin/

clean:
	rm -rf tmpBuild $(exec) *~

.PHONY: install clean build
