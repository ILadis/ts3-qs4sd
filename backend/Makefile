
# source and target files
CFILES   := $(wildcard src/*.c) $(wildcard src/api/*.c)
CFILES   += vnd/mongoose.c

CFLAGS   := -O2 -Wall -fPIC -include vnd/glibc_2.27.h -pthread
CFLAGS   += `pkg-config --cflags --libs libpulse sqlite3 uuid`

TARGET   := ts3-qs4sd.so

# toolchain and flags
CC       := gcc

.PHONY : clean vendor run

all: clean $(TARGET)
	@mkdir -p out/
	@mv $(TARGET) out/

install:
	@mkdir -p ~/.ts3client/plugins/
	@cp -f out/$(TARGET) ~/.ts3client/plugins/

vendor:
	@mkdir -p vnd/teamspeak/teamlog
	@curl https://github.com/wheybags/glibc_version_header/raw/master/version_headers/x64/force_link_glibc_2.27.h -Lso vnd/glibc_2.27.h
	@curl https://github.com/cesanta/mongoose/raw/7.13/mongoose.c -Lso vnd/mongoose.c
	@curl https://github.com/cesanta/mongoose/raw/7.13/mongoose.h -Lso vnd/mongoose.h
	@curl https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/plugin_definitions.h -Lso vnd/plugin_definitions.h
	@curl https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/ts3_functions.h -Lso vnd/ts3_functions.h
	@curl https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamlog/logtypes.h -Lso vnd/teamspeak/teamlog/logtypes.h
	@curl https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_definitions.h -Lso vnd/teamspeak/public_definitions.h
	@curl https://github.com/TeamSpeak-Systems/ts3client-pluginsdk/raw/master/include/teamspeak/public_rare_definitions.h -Lso vnd/teamspeak/public_rare_definitions.h

run:
	teamspeak3

clean:
	@rm -f out/*

purge:
	@rm -rf vnd/ out/

%.so:
	@$(CC) $(CFLAGS) $(CFILES) -o $@ -shared
