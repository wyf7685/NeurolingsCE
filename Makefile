include common.mk

SHIJIMA_USE_QTMULTIMEDIA ?= 1

PREFIX ?= /usr/local

SOURCES = src/app/main.cc \
	src/app/Asset.cc \
	src/app/MascotData.cc \
	src/app/AssetLoader.cc \
	src/app/ShijimaContextMenu.cc \
	src/app/ShijimaManager.cc \
	src/app/runtime/ShijimaManagerEnvironment.cc \
	src/app/runtime/ShijimaManagerImport.cc \
	src/app/runtime/ShijimaManagerMascots.cc \
	src/app/ShijimaWidget.cc \
	src/app/SoundEffectManager.cc \
	DefaultMascot.cc \
	src/app/ShijimaHttpApi.cc \
	src/app/ui/dialogs/common/ForcedProgressDialog.cc \
	src/app/ui/dialogs/inspector/ShimejiInspectorDialog.cc \
	src/app/ui/dialogs/inspector/ShimejiInspectorDialogRows.cc \
	src/app/ui/dialogs/licenses/ShijimaLicensesDialog.cc \
	src/app/ui/ShijimaManagerActions.cc \
	src/app/ui/ShijimaManagerTray.cc \
	src/app/ui/interface/ShijimaManagerAboutPage.cc \
	src/app/ui/interface/ShijimaManagerHomePage.cc \
	src/app/ui/interface/ShijimaManagerInterface.cc \
	src/app/ui/interface/ShijimaManagerSettingsPage.cc \
	src/app/cli.cc \
	src/app/SpeechBubbleWidget.cc \
	src/app/SimpleZipImporter.cc \
	miniz/miniz.c \
	src/resources/resources.rc \
	qrc_resources.cc \
	qrc_i18n.cc

DEFAULT_MASCOT_FILES := $(addsuffix .png,$(addprefix src/assets/DefaultMascot/img/shime,$(shell seq -s ' ' 1 1 46))) \
	src/assets/DefaultMascot/behaviors.xml src/assets/DefaultMascot/actions.xml

TS_FILES = translations/shijima-qt_zh_CN.ts
QM_FILES = $(TS_FILES:.ts=.qm)

LICENSE_FILES := Shijima-Qt.LICENSE.txt \
	duktape.LICENSE.txt \
	duktape.AUTHORS.rst \
	libarchive.LICENSE.txt \
	libshijima.LICENSE.txt \
	libshimejifinder.LICENSE.txt \
	unarr.LICENSE.txt \
	unarr.AUTHORS.txt \
	Qt.LICENSE.txt \
	rapidxml.LICENSE.txt

LICENSE_FILES := $(addprefix licenses/,$(LICENSE_FILES))

QT_LIBS = Widgets Core Gui Concurrent

TARGET_LDFLAGS := -Llibshimejifinder/build/unarr -lunarr

ifeq ($(PLATFORM),Linux)
QT_LIBS += DBus
PKG_LIBS := x11
TARGET_LDFLAGS += -Wl,-R -Wl,$(shell pwd)/publish/Linux/$(CONFIG)
endif

ifeq ($(PLATFORM),Windows)
TARGET_LDFLAGS += -lws2_32
endif

ifeq ($(SHIJIMA_USE_QTMULTIMEDIA),1)
QT_LIBS += Multimedia
CXXFLAGS += -DSHIJIMA_USE_QTMULTIMEDIA=1
else
CXXFLAGS += -DSHIJIMA_USE_QTMULTIMEDIA=0
endif

CXXFLAGS += -Iinclude -Isrc/platform -Ilibshijima -Ilibshimejifinder -Icpp-httplib -IElaWidgetTools/ElaWidgetTools -I. -DNEUROLINGSCE_VERSION='"$(NEUROLINGSCE_VERSION)"'
CPPFLAGS += -Iminiz
PKG_LIBS += libarchive
PUBLISH_DLL = $(addprefix Qt6,$(QT_LIBS))

define download_linuxdeploy
@uname_m="$$(uname -m)"; \
if [ "$${uname_m}" = "$(1)" -o "$${uname_m}" = "$(2)" ]; then \
	url="https://github.com/linuxdeploy/$(3)/releases/latest/download/$(3)-$(2).AppImage"; \
	name="$${url##*/}"; \
	echo "==> $${url}"; \
	wget -O "$${name}" -c --no-verbose "$${url}"; \
	touch "$${name}"; \
	chmod +x "$${name}"; \
	name2="$${name%-$(2).AppImage}.AppImage"; \
	ln -s "$${name}" "$${name2}"; \
fi
endef

all:: publish/$(PLATFORM)/$(CONFIG)

publish/Windows/$(CONFIG): shijima-qt$(EXE) FORCE
	mkdir -p $@
	@$(call copy_changed,libshimejifinder/build/unarr/libunarr.so.1.1.0,$@)
	@$(call copy_changed,$<,$@)
	@$(call copy_exe_dlls,$<,$@)
	@$(call copy_qt_plugin_dlls,$@)
	if [ $(CONFIG) = release ]; then find $@ -name '*.dll' -exec $(STRIP) -S '{}' \;; fi
	if [ $(CONFIG) = release ]; then $(STRIP)  -S $@/libunarr.so.1.1.0; fi

linuxdeploy-plugin-appimage-x86_64.AppImage:
	$(call download_linuxdeploy,x86_64,x86_64,linuxdeploy-plugin-appimage)

linuxdeploy-plugin-qt-x86_64.AppImage:
	$(call download_linuxdeploy,x86_64,x86_64,linuxdeploy-plugin-qt)

linuxdeploy-x86_64.AppImage: linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-plugin-appimage-x86_64.AppImage
	$(call download_linuxdeploy,x86_64,x86_64,linuxdeploy)

linuxdeploy-plugin-appimage-aarch64.AppImage:
	$(call download_linuxdeploy,arm64,aarch64,linuxdeploy-plugin-appimage)

linuxdeploy-plugin-qt-aarch64.AppImage:
	$(call download_linuxdeploy,arm64,aarch64,linuxdeploy-plugin-qt)

linuxdeploy-aarch64.AppImage: linuxdeploy-plugin-qt-aarch64.AppImage linuxdeploy-plugin-appimage-aarch64.AppImage
	$(call download_linuxdeploy,arm64,aarch64,linuxdeploy)

linuxdeploy.AppImage: linuxdeploy-aarch64.AppImage linuxdeploy-x86_64.AppImage


publish/macOS/$(CONFIG): shijima-qt$(EXE)
	mkdir -p $@
	$(call copy_changed,libshimejifinder/build/unarr/libunarr.1.dylib,$@)
	$(call copy_changed,$<,$@)
	if [ $(CONFIG) = release ]; then $(STRIP) -S $@/libunarr.1.dylib; fi
	install_name_tool -add_rpath "$$(realpath $@)" $@/shijima-qt

publish/Linux/$(CONFIG): shijima-qt$(EXE)
	mkdir -p $@
	@$(call copy_changed,libshimejifinder/build/unarr/libunarr.so.1,$@)
	if [ $(CONFIG) = release ]; then $(STRIP) -S $@/libunarr.so.1; fi
	@$(call copy_changed,$<,$@)

publish/macOS/$(CONFIG)/NeurolingsCE.app: publish/macOS/$(CONFIG)
	rm -rf $@ && [ ! -d $@ ]
	cp -r src/packaging/NeurolingsCE.app $@
	mkdir -p $@/Contents/MacOS
	cp publish/macOS/$(CONFIG)/shijima-qt $@/Contents/MacOS/
	/opt/local/libexec/qt6/bin/macdeployqt $@

publish/Linux/$(CONFIG)/NeurolingsCE.AppImage: publish/Linux/$(CONFIG) linuxdeploy.AppImage
	rm -rf AppDir
	NO_STRIP=1 ./linuxdeploy.AppImage --appdir AppDir --executable publish/Linux/$(CONFIG)/shijima-qt \
		--desktop-file src/packaging/io.github.qingchenyouforcc.NeurolingsCE.desktop --output appimage --plugin qt --icon-file \
		src/packaging/io.github.qingchenyouforcc.NeurolingsCE.png
	mv NeurolingsCE-*.AppImage NeurolingsCE.AppImage
	cp NeurolingsCE.AppImage publish/Linux/$(CONFIG)/

appimage: publish/Linux/$(CONFIG)/NeurolingsCE.AppImage

macapp: publish/macOS/$(CONFIG)/NeurolingsCE.app

$(APP_EXECUTABLE)$(EXE): src/platform/Platform/Platform.a libshimejifinder/build/libshimejifinder.a \
	libshijima/build/libshijima.a ElaWidgetTools/build/ElaWidgetTools/libElaWidgetTools.a $(APP_EXECUTABLE).a \
	src/packaging/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml \
	src/resources/resources.rc \
	src/platform/Platform/Linux/gnome_script/metadata.json
	$(CXX) -o $@ $(LD_COPY_NEEDED) $(LD_WHOLE_ARCHIVE) $^ $(LD_NO_WHOLE_ARCHIVE) \
		$(TARGET_LDFLAGS) $(LDFLAGS)
	if [ $(CONFIG) = "release" ]; then $(STRIP) $@; fi

libshijima/build/libshijima.a: libshijima/build/Makefile
	$(MAKE) -C libshijima/build

DefaultMascot.cc: $(DEFAULT_MASCOT_FILES) Makefile src/tools/bundle-default.sh
	./src/tools/bundle-default.sh $(DEFAULT_MASCOT_FILES) > '$@-'
	mv '$@-' '$@'

src/app/ui/dialogs/licenses/ShijimaLicensesDialog.cc: licenses_generated.hpp
	touch src/app/ui/dialogs/licenses/ShijimaLicensesDialog.cc

licenses_generated.hpp: $(LICENSE_FILES) Makefile
	echo 'static const char *shijima_licenses = R"(' > licenses_generated.hpp
	echo 'Licenses for the software components used in NeurolingsCE are listed below.' >> licenses_generated.hpp
	for file in $^; do \
		[ "$$file" != "Makefile" ] || continue; \
		(echo; echo) >> licenses_generated.hpp; \
		echo "~~~~~~~~~~ $$(basename $$file) ~~~~~~~~~~" >> licenses_generated.hpp; \
		echo >> licenses_generated.hpp; \
		cat $$file >> licenses_generated.hpp; \
	done
	echo ')";' >> licenses_generated.hpp

src/packaging/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml: src/packaging/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml.in VERSION.txt
	chmod +x src/tools/generate-metainfo.sh && ./src/tools/generate-metainfo.sh $< $@

src/resources/resources.rc: src/resources/resources.rc.in VERSION.txt
ifeq ($(PLATFORM),Windows)
	@echo "Generating resources.rc for Windows..."
	@VERSION=$$(grep "^VERSION=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0.1.0"); \
   VERSION_MAJOR=$$(grep "^VERSION_MAJOR=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0"); \
   VERSION_MINOR=$$(grep "^VERSION_MINOR=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "1"); \
   VERSION_PATCH=$$(grep "^VERSION_PATCH=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0"); \
	 sed -e "s/@VERSION@/$$VERSION/g" \
     -e "s/@VERSION_MAJOR@/$$VERSION_MAJOR/g" \
     -e "s/@VERSION_MINOR@/$$VERSION_MINOR/g" \
     -e "s/@VERSION_PATCH@/$$VERSION_PATCH/g" \
     $< > $@
endif

src/packaging/NeurolingsCE.app/Contents/Info.plist: src/packaging/NeurolingsCE.app/Contents/Info.plist.in VERSION.txt
ifeq ($(PLATFORM),macOS)
	@echo "Generating Info.plist for macOS..."
	@VERSION=$$(grep "^VERSION=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0.1.0"); \
	 sed -e "s/@VERSION@/$$VERSION/g" $< > $@
endif

src/platform/Platform/Linux/gnome_script/metadata.json: src/platform/Platform/Linux/gnome_script/metadata.json.in VERSION.txt
ifeq ($(PLATFORM),Linux)
	@echo "Generating metadata.json for GNOME extension..."
	@VERSION=$$(grep "^VERSION=" VERSION.txt 2>/dev/null | cut -d= -f2 || echo "0.1.0"); \
	 sed -e "s/@VERSION@/$$VERSION/g" $< > $@
endif

qrc_resources.cc: src/resources/resources.qrc src/packaging/neurolingsce.ico src/packaging/io.github.qingchenyouforcc.NeurolingsCE.png
	$(RCC) --name resources -o $@ $<

translations/%.qm: translations/%.ts
	$(LRELEASE) $< -qm $@

qrc_i18n.cc: translations/i18n.qrc $(QM_FILES)
	$(RCC) --name i18n -o $@ $<
libshijima/build/Makefile: libshijima/CMakeLists.txt FORCE
	mkdir -p libshijima/build && cd libshijima/build && $(CMAKE) $(CMAKEFLAGS) -DSHIJIMA_BUILD_EXAMPLES=NO ..

libshimejifinder/build/Makefile: libshimejifinder/CMakeLists.txt FORCE
	mkdir -p libshimejifinder/build && cd libshimejifinder/build && $(CMAKE) $(CMAKEFLAGS) \
		-DSHIMEJIFINDER_BUILD_LIBARCHIVE=NO -DSHIMEJIFINDER_BUILD_EXAMPLES=NO ..

libshimejifinder/build/libshimejifinder.a: libshimejifinder/build/Makefile
	$(MAKE) -C libshimejifinder/build
	if [ $(PLATFORM) = "Windows" ]; then cp libshimejifinder/build/unarr/libunarr.so.1.1.0 \
		libshimejifinder/build/unarr/libunarr.dll; fi

ElaWidgetTools/build/Makefile: ElaWidgetTools/ElaWidgetTools/CMakeLists.txt cmake/ElaWidgetToolsBuild/CMakeLists.txt FORCE
ifeq ($(PLATFORM),Windows)
	# For MinGW cross-compilation in Docker, override the hardcoded QT_SDK_DIR
	mkdir -p ElaWidgetTools/build && cd ElaWidgetTools/build && $(CMAKE) $(CMAKEFLAGS) -DQT_SDK_DIR=/usr/x86_64-w64-mingw32/sys-root/mingw -DELAWIDGETTOOLS_BUILD_STATIC_LIB=ON -DELAWIDGETTOOLS_SOURCE_DIR=$(shell pwd)/ElaWidgetTools/ElaWidgetTools ../../cmake/ElaWidgetToolsBuild
else
	mkdir -p ElaWidgetTools/build && cd ElaWidgetTools/build && $(CMAKE) $(CMAKEFLAGS) -DELAWIDGETTOOLS_BUILD_STATIC_LIB=ON -DELAWIDGETTOOLS_SOURCE_DIR=$(shell pwd)/ElaWidgetTools/ElaWidgetTools ../../cmake/ElaWidgetToolsBuild
endif

ElaWidgetTools/build/ElaWidgetTools/libElaWidgetTools.a: ElaWidgetTools/build/Makefile
	$(MAKE) -C ElaWidgetTools/build

clean::
	rm -rf publish/$(PLATFORM)/$(CONFIG) libshijima/build libshimejifinder/build ElaWidgetTools/build
	rm -f $(OBJECTS) $(APP_EXECUTABLE).a $(APP_EXECUTABLE)$(EXE) $(APP_NAME).AppImage qrc_resources.cc qrc_i18n.cc $(QM_FILES)
	find src/app -name '*.moc' -delete
	$(MAKE) -C src/platform/Platform clean

install:
	install -Dm755 publish/Linux/$(CONFIG)/$(APP_EXECUTABLE) $(PREFIX)/bin/$(APP_EXECUTABLE)
	install -Dm755 publish/Linux/$(CONFIG)/libunarr.so.1 $(PREFIX)/lib/libunarr.so.1
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.desktop $(PREFIX)/share/applications/$(APP_BUNDLE_ID).desktop
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml $(PREFIX)/share/metainfo/$(APP_BUNDLE_ID).metainfo.xml
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.png $(PREFIX)/share/icons/hicolor/512x512/apps/$(APP_BUNDLE_ID).png
	install -Dm755 publish/Linux/$(CONFIG)/shijima-qt $(PREFIX)/bin/shijima-qt
	install -Dm755 publish/Linux/$(CONFIG)/libunarr.so.1 $(PREFIX)/lib/libunarr.so.1
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.desktop $(PREFIX)/share/applications/io.github.qingchenyouforcc.NeurolingsCE.desktop
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml $(PREFIX)/share/metainfo/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml
	install -Dm644 src/packaging/io.github.qingchenyouforcc.NeurolingsCE.png $(PREFIX)/share/icons/hicolor/512x512/apps/io.github.qingchenyouforcc.NeurolingsCE.png

uninstall:
	rm -f $(PREFIX)/bin/$(APP_EXECUTABLE)
	rm -f $(PREFIX)/lib/libunarr.so.1
	rm -f $(PREFIX)/share/applications/$(APP_BUNDLE_ID).desktop
	rm -f $(PREFIX)/share/metainfo/$(APP_BUNDLE_ID).metainfo.xml
	rm -f $(PREFIX)/share/icons/hicolor/512x512/apps/$(APP_BUNDLE_ID).png
	rm -f $(PREFIX)/bin/shijima-qt
	rm -f $(PREFIX)/lib/libunarr.so.1
	rm -f $(PREFIX)/share/applications/io.github.qingchenyouforcc.NeurolingsCE.desktop
	rm -f $(PREFIX)/share/metainfo/io.github.qingchenyouforcc.NeurolingsCE.metainfo.xml
	rm -f $(PREFIX)/share/icons/hicolor/512x512/apps/io.github.qingchenyouforcc.NeurolingsCE.png

src/platform/Platform/Platform.a: FORCE
	$(MAKE) -C src/platform/Platform

$(APP_EXECUTABLE).a: $(OBJECTS) Makefile
	ar rcs $@ $(filter %.o,$^)

.PHONY: install uninstall update-translations
