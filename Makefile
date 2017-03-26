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
	mkdir -p tmpBuild
# We have to compile in C99 to support "long long" integers, as imposed by the "aadlinteger" type
# -Wall -Werror -Wextra -Wconversion -Wno-deprecated -Winit-self -Wsign-conversion -Wredundant-decls -Wvla -Wshadow -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wlogical-op -Wmissing-include-dirs -Winit-self -Wpointer-arith -Wcast-qual -Wcast-align -Wold-style-cast -Wno-error=old-style-cast -Wsign-promo -Wundef
	#clang -c -Wall -Werror -Iinclude c/*.c
	$(CC) -c -W -g3 -g -Wall -Werror -Wextra -Werror=format-security -Wconversion -Wno-deprecated -Winit-self -Wsign-conversion -Wredundant-decls -Wvla -Wshadow -Wlogical-op -Wmissing-include-dirs -Winit-self -Wpointer-arith -Wcast-qual -Wcast-align -Wno-error=old-style-cast -Wundef -std=c99 -pedantic -Iinclude c/*.c
	mv *.o tmpBuild/
	#ADA_PROJECT_PATH=`ocarina-config --prefix`/lib/gnat:$$ADA_PROJECT_PATH $(gnatpath)gnatmake -x -g $(exec) -p -P buildsupport.gpr -XBUILD="debug"
	ADA_PROJECT_PATH=`ocarina-config --prefix`/lib/gnat:$$ADA_PROJECT_PATH $(gnatpath)gprbuild -x -g $(exec) -p -P buildsupport.gpr -XBUILD="debug"
#	strip $(exec)

install:
	$(MAKE)
	cp buildsupport `ocarina-config --prefix`/bin/
	cp misc/driveGnuPlotsStreams.pl `ocarina-config --prefix`/bin/

clean:
	rm -rf tmpBuild
	rm -f $(exec)
	rm -f *~


release: clean
	rm -rf tmp/ release/
	mkdir release/
	mkdir -p tmp/buildsupport-$(VERSION)
	mkdir -p tmp/buildsupport-$(VERSION)/c
	mkdir -p tmp/buildsupport-$(VERSION)/ada
	mkdir -p tmp/buildsupport-$(VERSION)/include
	mkdir -p tmp/buildsupport-$(VERSION)/misc
	cp -f c/*.c tmp/buildsupport-$(VERSION)/c/
	cp -f ada/*.ads tmp/buildsupport-$(VERSION)/ada/
	cp -f ada/*.adb tmp/buildsupport-$(VERSION)/ada/
	cp -f include/*.h tmp/buildsupport-$(VERSION)/include/
	cp -f Makefile tmp/buildsupport-$(VERSION)/
	cp -f buildsupport.gpr tmp/buildsupport-$(VERSION)/
	cp -f misc/driveGnuPlotsStreams.pl tmp/buildsupport-$(VERSION)/misc/driveGnuPlotsStreams.pl
	( cd tmp && tar cvvfz buildsupport-$(VERSION).tar.gz buildsupport-$(VERSION) )
	mv tmp/buildsupport-$(VERSION).tar.gz release/
	mkdir -p tmp/buildsupport-`uname -m`-$(VERSION)/bin
	mkdir -p tmp/buildsupport-`uname -m`-$(VERSION)/misc/
	$(MAKE)
	cp -f buildsupport tmp/buildsupport-`uname -m`-$(VERSION)/bin
	cp -f misc/driveGnuPlotsStreams.pl tmp/buildsupport-`uname -m`-$(VERSION)/misc/
	( cd tmp && tar cvvfz buildsupport-`uname -m`-$(VERSION).tgz buildsupport-`uname -m`-$(VERSION)/)
	( cp -f tmp/buildsupport-`uname -m`-$(VERSION).tgz release/)
	rm -rf tmp/

.PHONY: release
