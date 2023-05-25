#ifndef SETTINGS_H
#define SETTINGS_H

#define _str(x) #x

/* For testing on Steam Deck setup port forwarding between
 * development machine and Steam Deck via SSH:
 *
 * $ ssh -T -L 8081:localhost:8081 deck
 * $ ssh -T -R 8000:127.0.0.1:8000 deck
 */

#define CORS_ORIGIN "https://steamloopback.host"

#define SERVER_HOST "localhost"
#define SERVER_PORT 8000

#define STEAM_DECK_HOST "localhost"
#define STEAM_DECK_PORT 8081

#define SETTINGS_STR(x) _str(x)
#define SETTINGS_ADDR(x) x##_HOST ":" SETTINGS_STR(x##_PORT)

#endif
