#include "../headers/raylib.h"
#define NOB_IMPLEMENTATION
#include "../headers/nob.h"
#define MEM_IMP
#include "../headers/mem.h"
#define RAYLIB_IMP
#include "../headers/raylib_layer.h"
#define W 570
#define H 500
#define FONT_SIZE 47
#define PARENT_DIR "/usr/share/applications/"
#include "../build/bundle.h"
Ren *gren;

int min3(int a, int b, int c) {
  int min = a;
  if (b < min) {
    min = b;
  }
  if (c < min) {
    min = c;
  }
  return min;
}

int lev(char *a, char *b) {
  size_t al=0,bl=0;

  while (a[al]) { ++al; }
  while (b[bl]) { ++bl; }

  if (a==0||b==0) {
    nob_log(NOB_ERROR,"Err: A or B are NULL or empty!");
    exit(-1);
  }
  Ren *m_ren = create_Ren((al + 1) * sizeof(int *));
  int **matrix = Ren_alloc(m_ren, (al + 1) * sizeof(int *));
  for (size_t i = 0; i <= al; ++i) {
    matrix[i] = Ren_alloc(m_ren, (bl + 1) * sizeof(int));
  }

  for (size_t i = 0; i <= al; ++i) {
    matrix[i][0] = i;
  }
  for (size_t j = 0; j <= bl; ++j) {
    matrix[0][j] = j;
  }

  for (size_t i = 1; i <= al; ++i) {
    for (size_t j = 1; j <= bl; ++j) {
      if (a[i - 1] == b[j - 1]) {
        matrix[i][j] =
            matrix[i - 1][j - 1]; // Characters are the same, no additional cost
      } else {
        matrix[i][j] = 1 + min3(matrix[i - 1][j],    // Deletion
                                matrix[i][j - 1],    // Insertion
                                matrix[i - 1][j - 1] // Substitution
                           );
      }
    }
  }
  int result = matrix[al][bl];
  Ren_free(m_ren);
  return result;
}
char *remove_extension(const char *filename) {
  if (filename == NULL)
    return NULL;
  size_t length=0;
  while (filename[length]) {
      ++length;
  }
  char *new_filename = Ren_strdup(gren,filename);
  if (new_filename == NULL)
    return NULL;
  char *dot = strrchr(new_filename, '.');

  char *sep1 = strrchr(new_filename, '/');
  char *sep2 = strrchr(new_filename, '\\');

  if (dot != NULL && (sep1 == NULL || dot > sep1) &&
      (sep2 == NULL || dot > sep2)) {
    *dot = '\0';
  }

  return new_filename;
}
size_t arrstrlen(char **arrstr) {
  size_t len = 0;
  while (arrstr[len] != NULL) {
    len++;
  }
  return len;
}
char *buffer_push_char(char *buffer, char ch) {
  size_t idx_tmp = 0;
  if (ch == '\0') {
    buffer[0] = ch;
    goto exit;
  }
  if (buffer[idx_tmp] == '\0') {
    buffer[idx_tmp] = ch;
    buffer[++idx_tmp] = '\0';
    goto exit;
  } else {
    while (buffer[idx_tmp]) {
      ++idx_tmp;
    }
    buffer[idx_tmp] = ch;
    buffer[++idx_tmp] = '\0';
    goto exit;
  }
exit:
  return buffer;
}

char **split_into(const char *str, char delim) {
  char *buffer = Ren_alloc(gren, 256);
  char **tmp = Ren_alloc(gren, 256 * 4096);
  size_t idxstr = 0, idx_tmp = 0;
  while (1) {
    if (str[idxstr] == '\0') {
      tmp[idx_tmp] = Ren_strdup(gren, buffer);
      buffer_push_char(buffer, '\0');
      break;
    } else if (str[idxstr] == delim) {
      tmp[idx_tmp] = Ren_strdup(gren, buffer);
      buffer_push_char(buffer, '\0');
      ++idx_tmp;
      ++idxstr;
    } else {
      buffer_push_char(buffer, str[idxstr]);
      ++idxstr;
    }
  }
  return tmp;
}
#ifdef _WIN32
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
void shift_args(char **args, uint16_t nshift) {
    size_t argc = arrstrlen(args);
    if (argc <= 1 || nshift == 0) return;

    nshift = nshift % argc;

    for (uint16_t i = 0; i < nshift; ++i) {
        char *last = args[argc - 1]; 
        for (size_t j = argc - 1; j > 0; --j) {
            args[j] = args[j + 1]; 
        }
        args[0] = last;
    }
}
void run_replace(const char *command) {
#ifdef _WIN32
#else
    char **args = split_into(command, ' ');
    char *bin = args[0];
    if (args[1]!=NULL) {
        shift_args(args,1);
    }
    if (!execvp(bin,args)) {
        perror("new Process -> Err -> execv :");
        exit(EXIT_FAILURE);
    } 
#endif
}
int read_app(char *name_app, Nob_File_Paths children) {
  char *exe_path = Ren_alloc(gren, 256);
  for (size_t i = 0; i < children.count; ++i) {
    char *s = remove_extension(children.items[i]);
    if (lev(name_app, s) == 0) {
      Nob_String_Builder reader = {0};
      char *exe = Ren_alloc(gren, 256);
      sprintf(exe, "%s%s", PARENT_DIR, children.items[i]);
      if (!nob_read_entire_file(exe, &reader)) {
        return -1;
      }
      char **lines = split_into(reader.items, '\n');
      for (size_t j = 0; arrstrlen(lines); ++j) {
        if (lines[j][1] == 'x') {
          char *s = lines[j];
          exe_path = split_into(s, '=')[1];
          break;
        }
      }
      run_replace(exe_path);
    }
  }
  return 0;
}
int main(void) {
  gren = create_Ren(4096 * 7);
  char *input = Ren_alloc(gren, 256);
  input[0] = '\0';
  Nob_File_Paths children = {0};
  Nob_File_Paths files = {0};
  Nob_File_Paths ranking = {0};
  char *final_ranking = Ren_alloc(gren, sizeof(char) * 256);
  if (!final_ranking)
    return -1;
  final_ranking[0] = '\0';
  memset(final_ranking, 0, sizeof(char) * 256);
  if (!nob_read_entire_dir("/usr/share/applications/", &children)) {
    return -1;
  }
  InitWindow(W, H, "Fri");
  for (size_t i = 0; i < children.count; ++i) {
    char *filename = remove_extension(children.items[i]);
    nob_da_append(&files, filename);
  }
  const char *font_path=("./fonts/IosevkaNerdFontMono-Regular.ttf");
  Color background = {.r=0x24,.g=0x27,.b=0x3a,.a=0xff};
  Font f={0};
  for (size_t i=0;i<resources_count;++i) {
      if (strcmp(resources[i].file_path,font_path)==0) {
          void *data=&bundle[resources[i].offset]; 
          size_t size=resources[i].size;
          f=LoadFontFromMemory(GetFileExtension(font_path),data,size,FONT_SIZE,NULL,0);
      }
  }
  if (!f.texture.id) {
    TraceLog(LOG_FATAL, "Err: Failed to load font");
    return -1;
  }
  float pady = 50;
  float padx = 20;
  while (!WindowShouldClose()) {
    size_t ranking_pass = 0;
    for (size_t i = 0; i < files.count; ++i) {
      if (lev((char *)files.items[i], input) <= 5) {
        ++ranking_pass;
      }
    }
    Nob_File_Paths *re_ranking = Ren_alloc(gren, sizeof(Nob_File_Paths));
    re_ranking->count = 0;
    re_ranking->capacity = ranking.count + ranking_pass;
    re_ranking->items = Ren_alloc(gren, re_ranking->capacity * sizeof(char *));
    for (size_t i = 0; i < files.count; ++i) {
      char *s = (char *)files.items[i];
      size_t lvr = lev(s, input);
      if (lvr == 0) {
        nob_da_append(re_ranking, files.items[i]);
        break;
      } else if (lvr <= 3) {
        nob_da_append(re_ranking, files.items[i]);
      }
    }
    ranking = *re_ranking;
    if (IsKeyDown(KEY_BACKSPACE)) {
      input[strlen(input) - 1] = '\0';
    }
    int key = GetKeyPressed();
    if (isalpha(key) & IsKeyReleased(KEY_LEFT_SHIFT) && key != 0) {
      input[strlen(input)] = (char)key;
      input[strlen(input) + 1] = '\0';
    } else if (isalpha(key) && !IsKeyReleased(KEY_LEFT_SHIFT) && key != 0) {
      input[strlen(input)] = (char)tolower(key);
      input[strlen(input) + 1] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER)&&input[0]!='\0') {
      for (size_t i = 0; i < ranking.count; ++i) {
        char *s = (char *)ranking.items[i];
        size_t lvr = lev(s, input);
        if (lvr == 0||input[strlen(input)-1]=='b') {
          read_app(s, children);
          goto exit;
        }
      }
    }
    BeginDrawing();
    ClearBackground(background);
    DrawTextEx(f, "     ", newVec2_t(0, 0), FONT_SIZE, 1.0f, WHITE);
    DrawTextEx(f, input, newVec2_t(padx/2, 0), FONT_SIZE, 1.0f, WHITE);
    if ((strcmp(final_ranking, "\0")) != 0) {
      DrawTextEx(f, final_ranking, newVec2_t((float)W / 2, pady / 2), FONT_SIZE, 1.0f,
                 WHITE);
    } else {
      for (size_t i = 0; i < ranking.count; ++i) {
        DrawTextEx(f, ranking.items[i], newVec2_t((float)W / 2, pady / 2 * i*2),
                   FONT_SIZE, 1.0f, WHITE);
      }
    }
    EndDrawing();
  }
exit:
  UnloadFont(f);
  Ren_free(gren);
  CloseWindow();
}
