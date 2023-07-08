
# source and target files
CFILES   := $(wildcard source/*.c) $(wildcard source/api/*.c) \
            $(wildcard source/injector/*.c) $(wildcard source/injector/states/*.c) \
            $(wildcard source/static/*.c)

CFILES   += include/vendor/mongoose.c include/vendor/mjson.c
CFLAGS   := -O2 -Wall -fPIC -Iinclude -include include/vendor/glibc_2.27.h -pthread
CFLAGS   += `pkg-config --cflags --libs libpulse`
TARGET   := ts3-qs4sd.so

# toolchain and flags
CC       := gcc

.PHONY : clean vendor plugin run

all: clean $(TARGET)

install: $(TARGET)
	@mkdir -p ~/.ts3client/plugins/
	@cp -f $(TARGET) ~/.ts3client/plugins/

vendor:
	@mkdir -p include/vendor/teamspeak/teamlog
	@wget https://github.com/wheybags/glibc_version_header/raw/master/version_headers/x64/force_link_glibc_2.27.h -qO include/vendor/glibc_2.27.h
	@wget https://github.com/graphitemaster/incbin/raw/main/incbin.h -qO include/vendor/incbin.h
	@wget https://github.com/cesanta/mjson/raw/1.2.7/src/mjson.c -qO include/vendor/mjson.c
	@wget https://github.com/cesanta/mjson/raw/1.2.7/src/mjson.h -qO include/vendor/mjson.h
	@wget https://github.com/cesanta/mongoose/raw/7.8/mongoose.c -qO include/vendor/mongoose.c
	@wget https://github.com/cesanta/mongoose/raw/7.8/mongoose.h -qO include/vendor/mongoose.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/plugin_definitions.h -qO include/vendor/plugin_definitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/ts3_functions.h -qO include/vendor/ts3_functions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamlog/logtypes.h -qO include/vendor/teamspeak/teamlog/logtypes.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/clientlib_publicdefinitions.h -qO include/vendor/teamspeak/clientlib_publicdefinitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_definitions.h -qO include/vendor/teamspeak/public_definitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_rare_definitions.h -qO include/vendor/teamspeak/public_rare_definitions.h

plugin:
	@npm --prefix plugin/ run build
	@zip plugin.zip plugin/package.json plugin/plugin.json plugin/main.py plugin/dist/index.js

run:
	teamspeak3

clean:
	@rm -rf $(TARGET)

%.so:
	@$(CC) $(CFLAGS) $(CFILES) -o $@ -shared
