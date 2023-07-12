
# source and target files
CFILES   := $(wildcard src/*.c) $(wildcard src/api/*.c) \
            $(wildcard src/injector/*.c) $(wildcard src/injector/states/*.c) \
            $(wildcard src/static/*.c)

CFILES   += vnd/mongoose.c vnd/mjson.c
CFLAGS   := -O2 -Wall -fPIC -include vnd/glibc_2.27.h -pthread
CFLAGS   += `pkg-config --cflags --libs libpulse`
TARGET   := ts3-qs4sd.so

# toolchain and flags
CC       := gcc

.PHONY : clean vendor plugin run

all: clean $(TARGET)
	@mkdir -p out/
	@mv $(TARGET) out/

install:
	@mkdir -p ~/.ts3client/plugins/
	@cp -f out/$(TARGET) ~/.ts3client/plugins/

vendor:
	@mkdir -p vnd/teamspeak/teamlog
	@wget https://github.com/wheybags/glibc_version_header/raw/master/version_headers/x64/force_link_glibc_2.27.h -qO vnd/glibc_2.27.h
	@wget https://github.com/graphitemaster/incbin/raw/main/incbin.h -qO vnd/incbin.h
	@wget https://github.com/cesanta/mjson/raw/1.2.7/src/mjson.c -qO vnd/mjson.c
	@wget https://github.com/cesanta/mjson/raw/1.2.7/src/mjson.h -qO vnd/mjson.h
	@wget https://github.com/cesanta/mongoose/raw/7.8/mongoose.c -qO vnd/mongoose.c
	@wget https://github.com/cesanta/mongoose/raw/7.8/mongoose.h -qO vnd/mongoose.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/plugin_definitions.h -qO vnd/plugin_definitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/ts3_functions.h -qO vnd/ts3_functions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamlog/logtypes.h -qO vnd/teamspeak/teamlog/logtypes.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/clientlib_publicdefinitions.h -qO vnd/teamspeak/clientlib_publicdefinitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_definitions.h -qO vnd/teamspeak/public_definitions.h
	@wget https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_rare_definitions.h -qO vnd/teamspeak/public_rare_definitions.h

run:
	teamspeak3

clean:
	@rm -rf out/

%.so:
	@$(CC) $(CFLAGS) $(CFILES) -o $@ -shared
