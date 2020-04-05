# implementing the prefab packager (see https://google.github.io/prefab/ and https://developer.android.com/studio/preview/features#native-dependencies)

.PHONY: prefab prefab-clean $(PROJECT_NAME).aar # always rebuild
prefab: $(PROJECT_NAME).aar

prefab-clean:
	-$(QUIET)rm -r aar

$(PROJECT_NAME).aar: aar/AndroidManifest.xml aar/META-INF/LICENSE aar/prefab
	$(QUIET)rm $@ 2>/dev/null
	$(QUIET)cd aar && zip -r ../$@ *

aar/AndroidManifest.xml:
	@mkdir -p $(@D)
	$(QUIET)echo '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.wels.openh264" android:versionCode="1" android:versionName="1.0">' >$@
	$(QUIET)echo '  <uses-sdk android:minSdkVersion="$(NDKLEVEL)" android:targetSdkVersion="$(NDKLEVEL)"/>' >>$@
	$(QUIET)echo '</manifest>' >>$@

aar/META-INF/LICENSE: ./LICENSE
	@mkdir -p $(@D)
	$(QUIET)cp $< $@

.PHONY: prefab.%
prefab.%: ARCH=$(@:prefab.%=%)
prefab.%:
	$(QUIET)$(MAKE) --no-print-directory prefab-abi ARCH=$(ARCH)

prefab-abi: DESTDIR=aar/prefab/modules/$(PROJECT_NAME)/libs/$(OS).$(ARCH)
prefab-abi:
	$(QUIET)$(MAKE) --no-print-directory clean install-shared $(DESTDIR)/abi.json \
 		SHAREDLIB_DIR=/. PREFIX=/. DESTDIR=$(DESTDIR) SRC_PATH=aar/../ # tune the destination for install, and trick clean to skip Android_clean the apps that are not there
	-$(QUIET)rm -r $(DESTDIR)/lib # we don't need the pc file

%/abi.json:
	$(QUIET)echo '{"abi":"$(APP_ABI)","api":$(NDKLEVEL),"ndk":20,"stl":"none"}' >$@

aar/prefab: prefab.arm prefab.arm64 prefab.x86 prefab.x86_64
	@:

aar/prefab: aar/prefab/prefab.json
aar/prefab/prefab.json:
	@mkdir -p $(@D)
	$(QUIET)echo '{"name":"$(PROJECT_NAME)","schema_version":1,"dependencies":[],"version":"'$(FULL_VERSION)'"}' >$@

aar/prefab: aar/prefab/modules/$(PROJECT_NAME)/module.json
aar/prefab/modules/$(PROJECT_NAME)/module.json:
	@mkdir -p $(@D)
	$(QUIET)echo '{"export_libraries":[],"library_name":"lib$(PROJECT_NAME)"}' > $@
