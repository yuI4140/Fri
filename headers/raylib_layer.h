#include "./raylib.h"
#include "defines.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct Vector2 Vec2_t;
typedef struct Vector3 Vec3_t;
typedef struct Vector4 Vec4_t;
typedef struct {
  Vec2_t v1; // Vertex 1
  Vec2_t v2; // Vertex 2
  Vec2_t v3; // Vertex 3
  Color color;
} Triangle;
typedef struct {
  Vec2_t start;
  Vec2_t end;
  Color color;
} Rec;
typedef struct {
  Vec2_t area;
  str title;
} Win;
typedef struct {
  union {
    Font f;
    Texture2D f2;
  } Options;
} fontManager;
// op available:'+','-','*','/'
Vec2_t newVec2_t(f32 x, f32 y);
Vec2_t opVec2_t(Vec2_t *a, Vec2_t *b, char op);
Vec3_t newVec3_t(f32 x, f32 y, f32 z);
Vec3_t opVec3_t(Vec3_t *a, Vec3_t *b, char op);
Vec4_t newVec4_t(f32 x, f32 y, f32 z, f32 w);
Vec4_t opVec4_t(Vec4_t *a, Vec4_t *b, char op);
fontManager *initFontM(b8 op, void *font);
void drawTextF(fontManager *manager, int fontSize, const char *format, ...);
Color to_hex(const char *hex);
Win *initWin(Vec2_t area, str title);
Rec *newRec(Vec2_t start, Vec2_t end, Color color);
Rectangle newRectangle(f32 x, f32 y, f32 width, f32 height);
Triangle *newTriangle(Vec2_t v1, Vec2_t v2, Vec2_t v3, Color color);
void drawTriangle_s(Triangle *tri);
void drawRec(Rec *rc);
Rectangle newRectangle(f32 x, f32 y, f32 width, f32 height);
void drawTextureToRectangle(Texture2D texture, Rectangle rect, Color tint);
Texture2D newVerticalGradientTexture(u32 width, u32 height, Color startColor,
                                     Color endColor);
Color newGradientColor(Color topColor, Color bottomColor, f32 gradientPositionY,
                       Win *win);
Texture2D newVerticalGradientTexture_v(u32 width, u32 height, int numColors,
                                       ...);
Texture2D newGradientTexture(u32 width, u32 height, u32 numColors,
                             Color *colors, b8 isVertical, f32 angle);
Color *newColorArray(int numColors, ...);
str getRandomColor_hex();
Color getRandomColor();
void clearBg(Texture2D texture);
void setBgRectangle(Texture2D bg, Rectangle rct);
#ifdef RAYLIB_IMP
fontManager *initFontM(b8 op, void *font) {
  fontManager *f = (fontManager *)malloc(sizeof(fontManager));

  if (op) {
    f->Options.f = LoadFont((const char *)font);
    f->Options.f2 = (Texture2D){0};
  } else {
    f->Options.f2 = LoadTexture((const char *)font);
    f->Options.f = (Font){0};
  }
  return f;
}
Win *initWin(Vec2_t area, str title) {
  InitWindow((i32)area.x, (i32)area.y, title);
  Win *win = (Win *)malloc(sizeof(Win));
  return win;
}
Rec *newRec(Vec2_t start, Vec2_t end, Color color) {
  Rec *rc = (Rec *)malloc(sizeof(Rec));
  rc->start = start;
  rc->end = end;
  rc->color = color;
  return rc;
}
void drawRec(Rec *rc) { DrawLineV(rc->start, rc->end, rc->color); }
Vec2_t newVec2_t(f32 x, f32 y) { return (Vec2_t){x, y}; }
Vec2_t opVec2_t(Vec2_t *a, Vec2_t *b, char op) {
  Vec2_t ret = {0};
  switch (op) {
  case '+':
    ret.x = a->x + b->x;
    ret.y = a->y + b->y;
    break;
  case '-':
    ret.x = a->x - b->x;
    ret.y = a->y - b->y;
    break;
  case '*':
    ret.x = a->x * b->x;
    ret.y = a->y * b->y;
    break;
  case '/':
    ret.x = (float)a->x / (float)b->x;
    ret.y = (float)a->y / (float)b->y;
    break;
  default:
    return (Vec2_t){0, 0};
    break;
  }
  return ret;
}
Vec3_t newVec3_t(f32 x, f32 y, f32 z) { return (Vec3_t){x, y, z}; }
Vec3_t opVec3_t(Vec3_t *a, Vec3_t *b, char op) {
  Vec3_t ret = {0};
  switch (op) {
  case '+':
    ret.x = a->x + b->x;
    ret.y = a->y + b->y;
    ret.z = a->z + b->z;
    break;
  case '-':
    ret.x = a->x - b->x;
    ret.y = a->y - b->y;
    ret.z = a->z - b->z;
    break;
  case '*':
    ret.x = a->x * b->x;
    ret.y = a->y * b->y;
    ret.z = a->z * b->z;
    break;
  case '/':
    ret.x = (float)a->x / (float)b->x;
    ret.y = (float)a->y / (float)b->y;
    ret.z = a->z / b->z;
    break;
  default:
    return (Vec3_t){0, 0, 0};
    break;
  }
  return ret;
}
Vec4_t newVec4_t(f32 x, f32 y, f32 z, f32 w) { return (Vec4_t){x, y, z, w}; }
Vec4_t opVec4_t(Vec4_t *a, Vec4_t *b, char op) {
  Vec4_t ret = {0};
  switch (op) {
  case '+':
    ret.x = a->x + b->x;
    ret.y = a->y + b->y;
    ret.z = a->z + b->z;
    ret.w = a->w + b->w;
    break;
  case '-':
    ret.x = a->x - b->x;
    ret.y = a->y - b->y;
    ret.z = a->z - b->z;
    ret.w = a->w - b->w;
    break;
  case '*':
    ret.x = a->x * b->x;
    ret.y = a->y * b->y;
    ret.z = a->z * b->z;
    ret.w = a->w * b->w;
    break;
  case '/':
    ret.x = (float)a->x / (float)b->x;
    ret.y = (float)a->y / (float)b->y;
    ret.z = a->z / b->z;
    ret.w = a->w / b->w;
    break;
  default:
    return (Vec4_t){0, 0, 0, 0};
    break;
  }
  return ret;
}

Color to_hex(const char *hex) {
  u32 hexValue = (u32)strtol(&hex[1], NULL, 16);
  u32 red = (hexValue >> 16) & 0xFF;
  u32 green = (hexValue >> 8) & 0xFF;
  u32 blue = hexValue & 0xFF;
  Color _color_ = {red, green, blue, 255};
  return _color_;
}
Rectangle newRectangle(f32 x, f32 y, f32 width, f32 height) {
  Rectangle rec = {x, y, width, height};
  return rec;
}
void drawTextureToRectangle(Texture2D texture, Rectangle rect, Color tint) {
  DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                 rect, (Vector2){0, 0}, 0.0f, tint);
}
Triangle *newTriangle(Vec2_t v1, Vec2_t v2, Vec2_t v3, Color color) {
  Triangle *triangle = (Triangle *)malloc(sizeof(Triangle));
  triangle->v1 = v1;
  triangle->v2 = v2;
  triangle->v3 = v3;
  triangle->color = color;
  return triangle;
}
void drawTriangle_s(Triangle *tri) {
  DrawTriangle(tri->v1, tri->v2, tri->v3, tri->color);
}
Texture2D newVerticalGradientTexture(u32 width, u32 height, Color startColor,
                                     Color endColor) {
  Image defaultImage = GenImageColor(width, height, WHITE);
  for (u32 y = 0; y < height; y++) {
    f32 progress = (f32)y / height; // Calculate the progress value from 0 to 1
    Color gradientColor =
        (Color){(u8)(startColor.r + (endColor.r - startColor.r) * progress),
                (u8)(startColor.g + (endColor.g - startColor.g) * progress),
                (u8)(startColor.b + (endColor.b - startColor.b) * progress),
                (u8)(startColor.a + (endColor.a - startColor.a) * progress)};
    Color *data = (Color *)defaultImage.data;
    for (u32 x = 0; x < width; x++) {
      data[y * width + x] = gradientColor;
    }
  }
  Texture2D gradientTexture = LoadTextureFromImage(defaultImage);
  UnloadImage(defaultImage);
  return gradientTexture;
}
void DrawTextureToRectangle(Texture2D texture, Rectangle rect, Color tint) {
  DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                 rect, (Vector2){0, 0}, 0.0f, tint);
}
Color newGradientColor(Color topColor, Color bottomColor, f32 gradientPositionY,
                       Win *win) {
  float t = gradientPositionY / win->area.y;
  int r = (int)(topColor.r * (1 - t) + bottomColor.r * t);
  int g = (int)(topColor.g * (1 - t) + bottomColor.g * t);
  int b = (int)(topColor.b * (1 - t) + bottomColor.b * t);

  Color gradientColor = {r, g, b, 255};
  return gradientColor;
}
// variantic version
Texture2D newVerticalGradientTexture_v(u32 width, u32 height, int numColors,
                                       ...) {
  va_list args;
  va_start(args, numColors);

  Color colors[numColors];
  for (int i = 0; i < numColors; i++) {
    colors[i] = va_arg(args, Color);
  }

  va_end(args);

  Image defaultImage = GenImageColor(width, height, WHITE);
  for (u32 y = 0; y < height; y++) {
    float progress =
        (float)y / (height - 1); // Calculate the progress value from 0 to 1
    int segment = progress * (numColors - 1);
    float segmentProgress = progress * (numColors - 1) - segment;

    Color startColor = colors[segment];
    Color endColor = colors[segment + 1];

    Color gradientColor = {
        (u8)(startColor.r + (endColor.r - startColor.r) * segmentProgress),
        (u8)(startColor.g + (endColor.g - startColor.g) * segmentProgress),
        (u8)(startColor.b + (endColor.b - startColor.b) * segmentProgress),
        (u8)(startColor.a + (endColor.a - startColor.a) * segmentProgress)};

    Color *data = (Color *)defaultImage.data;
    for (u32 x = 0; x < width; x++) {
      data[y * width + x] = gradientColor;
    }
  }
  Texture2D gradientTexture = LoadTextureFromImage(defaultImage);
  UnloadImage(defaultImage);
  return gradientTexture;
}
// for all types of gradient
Texture2D newGradientTexture(u32 width, u32 height, u32 numColors,
                             Color *colors, b8 isVertical, f32 angle) {
  Image defaultImage = GenImageColor(width, height, WHITE);
  float radians = DEG2RAD * angle;

  for (u32 y = 0; y < height; y++) {
    for (u32 x = 0; x < width; x++) {
      float progress;

      if (isVertical) {
        progress = (float)y / (height - 1);
      } else {
        progress = (float)x / (width - 1);
      }

      if (!isVertical) {
        float yFactor = tanf(radians);
        progress = (float)y / (height - 1);
        float offsetX = progress * yFactor * height;
        progress = (float)(x + offsetX) / (width - 1);
      }

      int segment = progress * (numColors - 1);
      float segmentProgress = progress * (numColors - 1) - segment;

      Color startColor = colors[segment];
      Color endColor = colors[segment + 1];

      Color gradientColor = {
          (u8)(startColor.r + (endColor.r - startColor.r) * segmentProgress),
          (u8)(startColor.g + (endColor.g - startColor.g) * segmentProgress),
          (u8)(startColor.b + (endColor.b - startColor.b) * segmentProgress),
          (u8)(startColor.a + (endColor.a - startColor.a) * segmentProgress)};

      Color *data = (Color *)defaultImage.data;
      data[y * width + x] = gradientColor;
    }
  }

  Texture2D gradientTexture = LoadTextureFromImage(defaultImage);
  UnloadImage(defaultImage);
  return gradientTexture;
}
Color *newColorArray(int numColors, ...) {
  va_list args;
  va_start(args, numColors);
  Color *colors = (Color *)malloc(numColors * sizeof(Color));
  if (!colors) {
    printf("Memory allocation error for gradient colors.\n");
    return NULL;
  }
  for (int i = 0; i < numColors; i++) {
    colors[i] = va_arg(args, Color);
  }
  va_end(args);
  return colors;
}
// get random color in type hex
str getRandomColor_hex() {
  srand((unsigned int)time(NULL));
  int r = rand() % 256;
  int g = rand() % 256;
  int b = rand() % 256;
  char *hexColor = (char *)malloc(8 * sizeof(char));
  if (!hexColor) {
    printf("Memory allocation error for hex color string.\n");
    return NULL;
  }
  snprintf(hexColor, 8, "#%02X%02X%02X", r, g, b);
  return hexColor;
}
Color getRandomColor() {
  str color = getRandomColor_hex();
  return to_hex(color);
}
void clearBg(Texture2D texture) { DrawTexture(texture, 0, 0, WHITE); }
void setBgRectangle(Texture2D bg, Rectangle rct) {
  DrawRectangleRec(rct, WHITE);
  drawTextureToRectangle(bg, rct, WHITE);
}
#define CRIMSON to_hex("#DC143C")
#endif // RAYLIB_IMP
