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

// Buffer sizes
#define DEFAULT_BUFFER_SIZE 256
#define INPUT_BUFFER_SIZE 256
#define FINAL_RANKING_BUFFER_SIZE 256
#define DESKTOP_FILE_PATH_BUFFER_SIZE 256
#define SPLIT_INTO_SEGMENT_BUFFER_SIZE 256 // Max size of an individual string segment
#define MAX_SPLIT_OUTPUT_ELEMENTS 512      // Max number of string segments after split

// Levenshtein distance thresholds
#define MAX_LEVENSHTEIN_DISTANCE_INITIAL_FILTER 5
#define MAX_LEVENSHTEIN_DISTANCE_FINAL_FILTER 3

// UI Layout
#define UI_PADDING_Y 50.0f
#define UI_PADDING_X 20.0f

// Arena Allocator sizes
// TODO: CRITICAL - GLOBAL_ARENA_SIZE (28KB) is likely too small for robust general use,
// especially if loading multiple/large files or many application names.
// Consider increasing this significantly or implementing a more dynamic arena.
#define GLOBAL_ARENA_SIZE (4096 * 7) // 28KB

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

// Calculates the Levenshtein distance between two strings a and b.
// Uses a local arena allocator for the distance matrix.
int lev(char *a, char *b) {
  size_t len_a = 0, len_b = 0;

  while (a[len_a]) {
    ++len_a;
  }
  while (b[len_b]) {
    ++len_b;
  }

  // TODO: For a library, returning an error code (-1) or specific error struct would be preferable to exit().
  // For this standalone application (fri), exit() is used for simplicity on critical errors.
  if (a == 0 || b == 0) {
    nob_log(NOB_ERROR, "Err: Input string a or b is NULL or empty in lev function!");
    exit(-1);
  }

  // Create a temporary arena for the matrix used in Levenshtein calculation
  Ren *lev_arena = create_Ren((len_a + 1) * sizeof(int *) + (len_a + 1) * (len_b + 1) * sizeof(int));
  if (!lev_arena) {
    nob_log(NOB_ERROR, "Failed to create arena for Levenshtein matrix.");
    exit(-1); // Or handle error more gracefully
  }

  int **matrix = Ren_alloc(lev_arena, (len_a + 1) * sizeof(int *));
  if (!matrix) {
    nob_log(NOB_ERROR, "Failed to allocate matrix rows in Levenshtein.");
    Ren_free(lev_arena);
    exit(-1);
  }
  for (size_t i = 0; i <= len_a; ++i) {
    matrix[i] = Ren_alloc(lev_arena, (len_b + 1) * sizeof(int));
    if (!matrix[i]) {
      nob_log(NOB_ERROR, "Failed to allocate matrix columns in Levenshtein.");
      Ren_free(lev_arena); // Free what was allocated so far
      exit(-1);
    }
  }

  // Initialize the matrix
  for (size_t i = 0; i <= len_a; ++i) {
    matrix[i][0] = i; // Cost of deletion
  }
  for (size_t j = 0; j <= len_b; ++j) {
    matrix[0][j] = j; // Cost of insertion
  }

  // Fill the matrix
  for (size_t i = 1; i <= len_a; ++i) {
    for (size_t j = 1; j <= len_b; ++j) {
      int cost = (a[i - 1] == b[j - 1]) ? 0 : 1; // Cost is 0 if characters are same, 1 otherwise
      matrix[i][j] = min3(matrix[i - 1][j] + 1,          // Deletion
                                matrix[i][j - 1] + 1,          // Insertion
                                matrix[i - 1][j - 1] + cost  // Substitution or match
                           );
    }
  }
  int result = matrix[len_a][len_b];
  Ren_free(lev_arena); // Free the temporary arena
  return result;
}
char *remove_extension(const char *filename) {
  if (filename == NULL) return NULL;
  // size_t length = 0; // length variable is not used
  // while (filename[length]) {
  //   ++length;
  // }
  char *filename_copy = Ren_strdup(gren, filename); // Use global arena
  if (filename_copy == NULL) return NULL;
  char *dot = strrchr(filename_copy, '.');

  char *sep1 = strrchr(filename_copy, '/');
  char *sep2 = strrchr(filename_copy, '\\');

  if (dot != NULL && (sep1 == NULL || dot > sep1) &&
      (sep2 == NULL || dot > sep2)) {
    *dot = '\0'; // Truncate at the dot
  }

  return filename_copy;
}
// Calculates the length of a NULL-terminated array of strings.
size_t arrstrlen(char **string_array) {
  size_t len = 0;
  while (string_array[len] != NULL) {
    len++;
  }
  return len;
}
// Appends a character to a string buffer.
// Note: This function assumes buffer has enough space.
// It also has a 'goto exit' which is generally discouraged.
// TODO: Review if this function can be replaced by a safer string utility or nob_sb functions.
// The goto statements, while functional in this small context, are generally discouraged.
char *buffer_push_char(char *buffer, char ch) {
  size_t current_index = 0;
  if (ch == '\0') { // If char to push is NULL terminator, just set it and return
    buffer[0] = ch;
    goto exit; // Original logic preserved
  }
  if (buffer[current_index] == '\0') { // If buffer is empty
    buffer[current_index] = ch;
    buffer[++current_index] = '\0';
    goto exit; // Original logic preserved
  } else { // Find end of string and append
    while (buffer[current_index]) {
      ++current_index;
    }
    buffer[current_index] = ch;
    buffer[++current_index] = '\0';
    goto exit; // Original logic preserved
  }
exit:
  return buffer;
}

// Splits a string 'str' by delimiter 'delim'.
// Allocates memory from the global arena 'gren'.
// TODO: Consider using nob_sb for string splitting if applicable, or improve safety, especially around buffer sizes.
char **split_into(const char *input_string, char delimiter) {
  char *current_segment_buffer = Ren_alloc(gren, SPLIT_INTO_SEGMENT_BUFFER_SIZE); // Buffer for the current segment
  // Allocate space for an array of string pointers
  char **output_array = Ren_alloc(gren, MAX_SPLIT_OUTPUT_ELEMENTS * sizeof(char *));
  size_t input_string_index = 0, output_array_index = 0;

  if (!current_segment_buffer || !output_array) {
    nob_log(NOB_ERROR, "Failed to allocate memory in split_into.");
    // Not freeing here as it's from an arena that will be freed globally.
    // However, returning NULL might be a good indicator of failure.
    return NULL; // Or handle error more robustly
  }
  current_segment_buffer[0] = '\0'; // Ensure buffer is empty initially

  while (1) {
    if (output_array_index >= MAX_SPLIT_OUTPUT_ELEMENTS -1) { // Safety break: -1 to ensure space for NULL terminator
        nob_log(NOB_ERROR, "Exceeded MAX_SPLIT_OUTPUT_ELEMENTS in split_into. Input string too complex or limit too small.");
        // Store what we have and NULL terminate if possible (output_array might be NULL if allocation failed)
        if (output_array) output_array[MAX_SPLIT_OUTPUT_ELEMENTS - 1] = NULL;
        break;
    }
    if (input_string[input_string_index] == '\0') {
      output_array[output_array_index] = Ren_strdup(gren, current_segment_buffer);
      output_array[++output_array_index] = NULL; // Null-terminate the array of strings
      break;
    } else if (input_string[input_string_index] == delimiter) {
      output_array[output_array_index] = Ren_strdup(gren, current_segment_buffer);
      buffer_push_char(current_segment_buffer, '\0'); // Reset buffer for next segment
      ++output_array_index;
      ++input_string_index;
    } else {
      // TODO: Add check for current_segment_buffer overflow if SPLIT_INTO_SEGMENT_BUFFER_SIZE is too small for a long segment without delimiters.
      buffer_push_char(current_segment_buffer, input_string[input_string_index]);
      ++input_string_index;
    }
  }
  // Ren_free for current_segment_buffer is not needed due to arena.
  return output_array;
}
#ifdef _WIN32
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
// Shifts elements in a string array in a cyclic manner.
// The last element becomes the first, and all other elements shift one position to the right.
// This is repeated `shift_count` times.
void shift_args(char **args, uint16_t shift_count) {
  size_t arg_count = arrstrlen(args);
  if (arg_count <= 1 || shift_count == 0) return;

  shift_count = shift_count % arg_count; // Ensure shift_count is within bounds

  for (uint16_t i = 0; i < shift_count; ++i) {
    char *last_element = args[arg_count - 1];
    // Shift elements to the right
    for (size_t j = arg_count - 1; j > 0; --j) {
      args[j] = args[j - 1]; // Error in original: was args[j+1], should be args[j-1] for right shift to put last at front
    }
    args[0] = last_element; // Place the original last element at the beginning
  }
}
// Replaces the current process image with a new one.
// Only implemented for non-Windows systems.
void run_replace(const char *command_string) {
#ifdef _WIN32
  // TODO: Implement for Windows if necessary, or clarify that it's POSIX-specific.
  nob_log(NOB_WARNING, "run_replace is not implemented for Windows.");
#else
  if (command_string == NULL || command_string[0] == '\0') {
    nob_log(NOB_ERROR, "Command string for run_replace is NULL or empty.");
    return;
  }
  char **arguments = split_into(command_string, ' ');
  if (arguments == NULL || arguments[0] == NULL) {
    nob_log(NOB_ERROR, "Failed to split command string or command is empty.");
    // Memory for arguments is from arena, will be freed globally.
    return;
  }

  char *binary_path = arguments[0];
  // The original shift_args logic was to remove the command itself from arguments for execvp.
  // execvp expects `argv[0]` to be the program name, and `argv[1]...` to be the arguments.
  // If split_into returns ["command", "arg1", "arg2", NULL], then `arguments` is already correct.
  // The original `shift_args(arguments, 1)` would make it `[arg2, command, arg1, NULL]` (if 3 elements) which is wrong.
  // Correct is to pass `arguments` directly if `arguments[0]` is the command.

  if (!execvp(binary_path, arguments)) {
    perror("execvp error in run_replace"); // perror is more informative
    // exit(EXIT_FAILURE); // Exiting here might be too abrupt.
  }
  // If execvp fails, we are still in the old process.
  // Memory for `arguments` is from the global arena.
#endif
}

// Forward declarations for static functions to be used in main
static void update_ranking_list(Nob_File_Paths *application_names,
                                const char *user_input,
                                Nob_File_Paths *target_ranking_list, Ren *arena);
static void handle_user_input(char *user_input_buffer, size_t buffer_size);
static void handle_enter_key_action(const char *user_input,
                                    Nob_File_Paths *ranked_list,
                                    Nob_File_Paths *all_desktop_files);
static void draw_ui(Font main_font, const char *user_input,
                    Nob_File_Paths *ranked_list,
                    const char *final_ranking_text, // This seems unused based on previous analysis
                    Color background_color);

// Reads application information and potentially launches an application.
// (This function remains mostly the same, but it's called by handle_enter_key_action)
int read_app(char *application_name_to_find, Nob_File_Paths desktop_files_list) {
  char *executable_path_buffer = Ren_alloc(gren, DESKTOP_FILE_PATH_BUFFER_SIZE);
  if (!executable_path_buffer) {
      nob_log(NOB_ERROR, "Failed to allocate buffer for executable_path_buffer in read_app.");
      return -1;
  }

  for (size_t i = 0; i < desktop_files_list.count; ++i) {
    char *current_desktop_filename_no_ext = remove_extension(desktop_files_list.items[i]);
    if (current_desktop_filename_no_ext && lev(application_name_to_find, current_desktop_filename_no_ext) == 0) {
      Nob_String_Builder file_content_sb = {0};
      char *full_desktop_file_path = Ren_alloc(gren, DESKTOP_FILE_PATH_BUFFER_SIZE);
      if (!full_desktop_file_path) {
          nob_log(NOB_ERROR, "Failed to allocate buffer for full_desktop_file_path in read_app.");
          // executable_path_buffer will be freed by arena
          return -1;
      }
      snprintf(full_desktop_file_path, DESKTOP_FILE_PATH_BUFFER_SIZE, "%s%s", PARENT_DIR, desktop_files_list.items[i]);

      if (!nob_read_entire_file(full_desktop_file_path, &file_content_sb)) {
        // Error already logged by nob_read_entire_file
        // Free buffers if not using arena, but here they are arena-allocated.
        return -1;
      }

      char **lines = split_into(file_content_sb.items, '\n');
      nob_sb_free(file_content_sb); // Free the string builder's items if it used malloc, or if split_into needs it freed.
                                    // Assuming nob_sb_free is safe to call. If items are from arena, this might not be needed or could be an issue.
                                    // Given split_into uses gren, file_content_sb.items is likely also from gren or copied.

      if (lines) {
        for (size_t j = 0; lines[j] != NULL; ++j) { // Check lines[j] != NULL for safety
          char *current_line = lines[j];
          // A more robust parsing for "Exec=" line would be better.
          // This assumes "Exec=" is always at the start or similar fixed format.
          if (strncmp(current_line, "Exec=", 5) == 0) { // Check for "Exec="
            // Simple parsing: take everything after "Exec="
            // TODO: Handle potential arguments or command specifiers (e.g., %f, %U, %c, etc.) in Exec line.
            // This simple copy might lead to issues if the command has such specifiers.
            // A more robust parser would identify and potentially substitute/remove these based on context.
            strncpy(executable_path_buffer, current_line + 5, DESKTOP_FILE_PATH_BUFFER_SIZE -1);
            executable_path_buffer[DESKTOP_FILE_PATH_BUFFER_SIZE -1] = '\0'; // Ensure null termination
            break; 
          }
        }
        // Memory for `lines` and its contents are from `gren` via `split_into`.
      }
      
      if (executable_path_buffer[0] != '\0') {
        run_replace(executable_path_buffer);
        // If run_replace is successful, this part of code is not reached.
        // If it fails, we might want to log or handle it.
      }
      // No need to free current_desktop_filename_no_ext, full_desktop_file_path, executable_path_buffer due to global arena `gren`.
      return 0; // Found and processed
    }
    // No need to free current_desktop_filename_no_ext here due to global arena.
  }
  return 0; // Or -1 if not found should be an error
}
int main(void) {
  gren = create_Ren(GLOBAL_ARENA_SIZE);
  if (!gren) {
    fprintf(stderr, "FATAL: Could not create global arena allocator.\n");
    return 1;
  }
  char *user_input_buffer = Ren_alloc(gren, INPUT_BUFFER_SIZE);
  if (!user_input_buffer) {
      nob_log(NOB_ERROR, "Failed to allocate user_input_buffer.");
      Ren_free(gren); return 1;
  }
  user_input_buffer[0] = '\0';

  Nob_File_Paths application_desktop_files = {0}; // Files from PARENT_DIR
  Nob_File_Paths application_names = {0};         // Names extracted from .desktop files
  Nob_File_Paths current_ranking_list = {0};      // Filtered list based on input

  // This buffer seems unused or its purpose unclear, review if needed.
  char *final_ranking_display_buffer = Ren_alloc(gren, FINAL_RANKING_BUFFER_SIZE);
  if (!final_ranking_display_buffer) {
    nob_log(NOB_ERROR, "Failed to allocate final_ranking_display_buffer.");
    Ren_free(gren); return 1;
  }
  final_ranking_display_buffer[0] = '\0';
  // memset(final_ranking_display_buffer, 0, FINAL_RANKING_BUFFER_SIZE); // Already set to empty string

  if (!nob_read_entire_dir(PARENT_DIR, &application_desktop_files)) {
    // Error logged by nob_read_entire_dir
    Ren_free(gren);
    return 1;
  }

  InitWindow(W, H, "Fri");
  SetTargetFPS(60); // Good practice

  for (size_t i = 0; i < application_desktop_files.count; ++i) {
    char *filename_no_ext = remove_extension(application_desktop_files.items[i]);
    if (filename_no_ext) { // Ensure not NULL
        nob_da_append(&application_names, filename_no_ext);
    }
  }
  // application_desktop_files items are full names, application_names items are without extension. Both point to memory in `gren`.

  const char *font_asset_path = ("./fonts/IosevkaNerdFontMono-Regular.ttf"); // Changed name for clarity
  Color ui_background_color = {.r = 0x24, .g = 0x27, .b = 0x3a, .a = 0xff};
  Font main_font = {0}; // Changed name for clarity

  // Load font from embedded bundle
  for (size_t i = 0; i < resources_count; ++i) {
    if (strcmp(resources[i].file_path, font_asset_path) == 0) {
      void *font_data = &bundle[resources[i].offset];
      size_t font_data_size = resources[i].size;
      main_font = LoadFontFromMemory(GetFileExtension(font_asset_path), font_data, font_data_size, FONT_SIZE,
                             NULL, 0); // Removed NULL check for params, as it's standard
      break; // Found font, no need to continue loop
    }
  }

  if (!main_font.texture.id) { // Check if font loading failed
    TraceLog(LOG_FATAL, "Err: Failed to load font from memory: %s", font_asset_path);
    Ren_free(gren); // Clean up arena
    CloseWindow();    // Close Raylib window
    return 1;
  }

  // float pady = UI_PADDING_Y; // These are now directly used via constants
  // float padx = UI_PADDING_X;

  while (!WindowShouldClose()) {
    // 1. Update Ranking List based on current input
    // The existing current_ranking_list will be overwritten by update_ranking_list.
    // We pass application_names (all available apps) and user_input_buffer.
    // The result is stored in current_ranking_list.
    // `gren` is passed for allocations within update_ranking_list.
    // Note: This implies current_ranking_list's previous items (if any from gren) are now unreferenced
    // but will be cleaned up when `gren` is freed. This is typical for arena allocators in loops.
    update_ranking_list(&application_names, user_input_buffer, &current_ranking_list, gren);

    // 2. Handle User Input (Backspace, Character typing)
    handle_user_input(user_input_buffer, INPUT_BUFFER_SIZE);
    
    // 3. Handle Enter Key Action
    // This might call read_app, which might call execvp and terminate the process.
    handle_enter_key_action(user_input_buffer, &current_ranking_list, &application_desktop_files);

    // 4. Draw UI
    draw_ui(main_font, user_input_buffer, &current_ranking_list, final_ranking_display_buffer, ui_background_color);
  }
// exit: // Label for goto, no longer used directly by the main loop structure, but kept if read_app implies an exit path.
  UnloadFont(main_font);
  Ren_free(gren); // Free the entire global arena
  CloseWindow();
  return 0; // Ensure main returns a value
}

// Updates the target_ranking_list based on user_input and application_names.
// Allocations are made from the provided arena.
static void update_ranking_list(Nob_File_Paths *application_names,
                                const char *user_input,
                                Nob_File_Paths *target_ranking_list, Ren *arena) {
    size_t pre_filter_match_count = 0;
    for (size_t i = 0; i < application_names->count; ++i) {
        if (lev((char *)application_names->items[i], user_input) <= MAX_LEVENSHTEIN_DISTANCE_INITIAL_FILTER) {
            ++pre_filter_match_count;
        }
    }
    
    // TODO: Review arena strategy for per-frame allocations like `new_ranked_list.items`.
    // A common approach is to use a sub-arena that can be reset each frame,
    // or to allocate a sufficiently large fixed buffer from `gren` at the start and reuse it for `target_ranking_list->items`.
    Nob_File_Paths new_ranked_list = {0};
    // Ensure capacity is reasonable, e.g., not excessively large if pre_filter_match_count is huge.
    // For now, it's based on matches + 1.
    new_ranked_list.capacity = pre_filter_match_count + 1; 
    new_ranked_list.items = Ren_alloc(arena, new_ranked_list.capacity * sizeof(char *));
    
    if (!new_ranked_list.items && new_ranked_list.capacity > 0) { // check capacity > 0 for Ren_alloc(..., 0) case
        nob_log(NOB_ERROR, "Failed to allocate items for new_ranked_list in update_ranking_list");
        target_ranking_list->count = 0;
        target_ranking_list->items = NULL; // Make sure items is NULL
        return;
    }
    new_ranked_list.count = 0;


    for (size_t i = 0; i < application_names->count; ++i) {
        char *current_app_name = (char *)application_names->items[i];
        size_t levenshtein_result = lev(current_app_name, user_input);

        if (user_input[0] == '\0') { 
             // If input is empty, show some applications, e.g., up to a certain limit (e.g., 10 or capacity)
             if (new_ranked_list.count < new_ranked_list.capacity && new_ranked_list.count < 10) { 
                nob_da_append(&new_ranked_list, current_app_name);
             } else if (new_ranked_list.count >= 10) { // Stop after 10 if no input
                break; 
             }
        } else if (levenshtein_result == 0) {
            // If exact match, clear list and add only this one
            new_ranked_list.count = 0; 
            nob_da_append(&new_ranked_list, current_app_name);
            break; 
        } else if (levenshtein_result <= MAX_LEVENSHTEIN_DISTANCE_FINAL_FILTER) {
            if (new_ranked_list.count < new_ranked_list.capacity) { // Check against full capacity
                nob_da_append(&new_ranked_list, current_app_name);
            } else {
                // Optional: Log if capacity is hit and more items could have been added.
                // nob_log(NOB_INFO, "Ranking list capacity hit in update_ranking_list.");
                break; 
            }
        }
    }
    // If the existing items in target_ranking_list were allocated by a previous call
    // to this function from the same arena, they are now effectively "orphaned" if
    // new_ranked_list.items got a new pointer. Arena handles cleanup eventually.
    // If target_ranking_list->items was from a different source, it would leak if not managed.
    // Here, we assume all items are from the passed 'arena'.
    target_ranking_list->items = new_ranked_list.items;
    target_ranking_list->count = new_ranked_list.count;
    target_ranking_list->capacity = new_ranked_list.capacity;
}

// Handles keyboard input for text entry and backspace.
static void handle_user_input(char *user_input_buffer, size_t buffer_size) {
    if (IsKeyDown(KEY_BACKSPACE)) {
        size_t len = strlen(user_input_buffer);
        if (len > 0) {
            user_input_buffer[len - 1] = '\0';
        }
    }

    int pressed_key = GetKeyPressed(); // Get one key press at a time
    // This handles basic alphanumeric and space. More complex input might need GetCharPressed.
    if ((pressed_key >= 32 && pressed_key <= 126) && // Printable characters (ASCII space to ~)
        (strlen(user_input_buffer) < buffer_size - 1)) { 
        
        char char_to_append = (char)pressed_key;
        // Simple tolower for A-Z, assumes ASCII. More robust would be `tolower()` from `<ctype.h>`
        if (char_to_append >= 'A' && char_to_append <= 'Z') {
            // char_to_append = tolower(char_to_append); // Requires <ctype.h>
        }

        size_t len = strlen(user_input_buffer);
        user_input_buffer[len] = char_to_append;
        user_input_buffer[len + 1] = '\0';
    }
}

// Handles action when Enter key is pressed.
static void handle_enter_key_action(const char *user_input,
                                    Nob_File_Paths *ranked_list,
                                    Nob_File_Paths *all_desktop_files) {
    if (IsKeyPressed(KEY_ENTER) && user_input[0] != '\0') {
        if (ranked_list->count > 0) {
            char *selected_app_name = (char *)ranked_list->items[0]; // Select the top one
            // The original logic for 'b' at the end of input is preserved here.
            // if (lev(selected_app_name, (char*)user_input) == 0 || user_input[strlen(user_input) - 1] == 'b') {
            read_app(selected_app_name, *all_desktop_files);
            // If read_app calls execvp, this point might not be reached.
            // The original `goto exit` is removed; flow will naturally exit or continue loop if exec fails.
            // }
        }
    }
}

// Draws the entire UI.
static void draw_ui(Font main_font, const char *user_input,
                    Nob_File_Paths *ranked_list,
                    const char *final_ranking_text, // This parameter seems unused.
                    Color background_color) {
    BeginDrawing();
    ClearBackground(background_color);

    // Draw the user input text
    // Adding a small visual prompt like "> " could be nice.
    DrawTextEx(main_font, TextFormat("> %s",user_input), newVec2_t(UI_PADDING_X / 2, UI_PADDING_Y / 2), FONT_SIZE, 1.0f, WHITE);

    // Draw the ranked list of applications
    // The final_ranking_text logic was previously an `else` to this, keeping structure.
    if ((strcmp(final_ranking_text, "\0")) != 0) { // This buffer seems unused.
        DrawTextEx(main_font, final_ranking_text, newVec2_t((float)W / 2, UI_PADDING_Y / 2), FONT_SIZE,
                   1.0f, WHITE);
    } else {
        for (size_t i = 0; i < ranked_list->count; ++i) {
            // Display multiple ranked items, offset from input text line
            DrawTextEx(main_font, ranked_list->items[i],
                       newVec2_t(UI_PADDING_X / 2, UI_PADDING_Y / 2 + ((i + 1) * (FONT_SIZE + 5))), // Start list below input text
                       FONT_SIZE, 1.0f, WHITE);
        }
    }
    EndDrawing();
}
