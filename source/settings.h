#ifndef SETTINGS_H
#define SETTINGS_H

#define _str(x) #x

#define CORS_ORIGIN "https://steamloopback.host" // change for testing to: "http://localhost:8080"

#define SERVER_HOST "localhost"
#define SERVER_PORT 8000

#define STEAM_DECK_HOST "localhost"
#define STEAM_DECK_PORT 8081 // change for testing to 8080

#define SETTINGS_STR(x) _str(x)
#define SETTINGS_ADDR(x) x##_HOST ":" SETTINGS_STR(x##_PORT)

#endif
