GYP=./tools/gyp/gyp

ifeq ($(shell uname),Darwin)
	PROJECT_FILES += ./openh264.xcodeproj
	BUILD_CMD = xcodebuild
endif

all: $(PROJECT_FILES)
	$(BUILD_CMD)

openh264.xcodeproj: $(GYP) openh264.gyp
	$(GYP) --depth $<

dependencies: $(GYP)

tools:
	mkdir tools

$(GYP): tools
	cd tools && rm -rf gyp && svn co http://gyp.googlecode.com/svn/trunk gyp

clean:
	rm -rf $(PROJECT_FILES) *~ ./build

dist-clean: clean
	rm -rf ./tools
