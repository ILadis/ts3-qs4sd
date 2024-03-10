#ifndef SETTINGS_H
#define SETTINGS_H

#define _str(x) #x

/* For testing on Steam Deck setup port forwarding between
 * development machine and Steam Deck via SSH:
 *
 * $ ssh -T -R 52259:127.0.0.1:52259 deck
 * $ ssh -T -L 8080:127.0.0.1:8080 deck
 * $ ssh -T -R 1337:127.0.0.1:1337 deck
 */

#define CORS_ORIGIN "https://steamloopback.host"

#define SERVER_HOST "localhost"
#define SERVER_PORT 52259

#define STEAM_DECK_HOST "localhost"
#define STEAM_DECK_PORT 8080

#define SETTINGS_STR(x) _str(x)
#define SETTINGS_ADDR(x) x##_HOST ":" SETTINGS_STR(x##_PORT)

#endif
