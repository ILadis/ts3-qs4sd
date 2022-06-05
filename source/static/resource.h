#ifndef RESOURCE_H
#define RESOURCE_H

#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include <vendor/incbin.h>

struct mg_resource {
  const char *path;
  const char *media;
  const unsigned char *data;
  const unsigned int *size;
};

#define mg_resource_of(path, media, incbin) ((struct mg_resource){ \
  path, media, g_##incbin##_data, &g_##incbin##_size \
})

#define mg_resource_empty() ((struct mg_resource){ \
  .size = &(const unsigned int){0}, \
  .media = "text/plain" \
})

#endif
