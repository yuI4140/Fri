#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define DEV_FEATURE 0
#define COMPILER "gcc"
#define SRC_NAME "fri"
#define INSTALL_FOLDER "template"
#define NOB_IMPLEMENTATION
#include "./headers/nob.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mkdir(dir, mode) _mkdir(dir)
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif
int remove_file(const char *path) {
#ifdef _WIN32
  return _unlink(path);
#else
  return remove(path);
#endif
}
int remove_dir(const char *path) {
#ifdef _WIN32
  return _rmdir(path);
#else
  return rmdir(path);
#endif
}
int move_file(const char *src, const char *dest) {
  int result = 0;
#ifdef _WIN32
  result = MoveFile(src, dest);
#else
  result = rename(src, dest);
#endif
  return result;
}
void help(void) {
  nob_log(NOB_INFO, "Flags available:");
  nob_log(NOB_INFO, "help\nHelp flag for showing this.");
  nob_log(NOB_INFO, "--run\n run.");
  nob_log(NOB_INFO, "--crun\n compile and run.");
  nob_log(NOB_INFO, "killy\nkill the main execute.");
  nob_log(NOB_INFO, "clean\nremove generated files/folders.");
  exit(1);
}
int main(int argc, char *argv[]) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *program_name = nob_shift_args(&argc, &argv); // Keep program_name for logs if needed, shifts argc/argv

  // Source file existence check
  char src_file_path[256];
  snprintf(src_file_path, sizeof(src_file_path), "./src/%s.c", SRC_NAME);
  if (!nob_file_exists(src_file_path)) {
    nob_log(NOB_ERROR, "Source file '%s' does not exist.", src_file_path);
    return 1; // Consistent return for errors
  }

  // Default flags and variables
  bool debug_build = true;
  bool compile_and_run = false;
  bool run_only = false;

  // Special mode for "test" target: compile only, bypass other arg parsing.
  if (strcmp(SRC_NAME, "test") == 0) {
    nob_log(NOB_INFO, "Target is 'test', proceeding directly to compilation.");
    goto compile_section; // Using goto for this specific control flow common in nob examples
  }

  // Command-line argument parsing
  if (argc > 0) {
    char *first_arg = nob_shift_args(&argc, &argv);
    if (strcmp(first_arg, "help") == 0) {
      help(); // Exits
    } else if (strcmp(first_arg, "killy") == 0) {
      if (DEV_FEATURE) {
        nob_log(NOB_INFO, "Attempting to remove self ('./nob')...");
        if (remove_file("./nob") == 0) {
          nob_log(NOB_INFO, "Successfully removed './nob'.");
          exit(0);
        } else {
          nob_log(NOB_ERROR, "Failed to remove './nob'.");
          exit(1);
        }
      } else {
        nob_log(NOB_INFO, "The 'killy' feature is currently disabled (DEV_FEATURE is 0).");
        exit(0);
      }
    } else if (strcmp(first_arg, "clean") == 0) {
      // TODO: Implement a more robust clean using nob_rmrf or similar
      nob_log(NOB_INFO, "Cleaning build artifacts...");
      if (system("rm -rf ./build/* ./obj/*") != 0) { // Example, adjust paths as needed
          nob_log(NOB_WARNING, "Clean command might have failed or partially failed.");
      } else {
          nob_log(NOB_INFO, "Cleaned build artifacts.");
      }
      // Consider also removing INSTALL_FOLDER/fri if that's desired for 'clean'
      exit(0);
    } else if (strcmp(first_arg, "--no-debug") == 0) {
      debug_build = false;
    } else if (strcmp(first_arg, "--install") == 0) {
      char install_path[256];
      snprintf(install_path, sizeof(install_path), "%s/fri", INSTALL_FOLDER);
      // Ensure install folder exists
      if (!nob_mkdir_if_not_exists(INSTALL_FOLDER)) {
          nob_log(NOB_ERROR, "Failed to create install directory '%s'.", INSTALL_FOLDER);
          return 1;
      }
      nob_log(NOB_INFO, "Installing './build/fri' to '%s'...", install_path);
      // Consider removing old version first
      // remove_file(install_path); // Optional: ignore error if it doesn't exist
      if (!nob_copy_file("./build/fri", install_path)) {
        nob_log(NOB_ERROR, "Failed to install 'fri' to '%s'. Ensure './build/fri' exists.", install_path);
        return 1;
      }
      nob_log(NOB_INFO, "'fri' installed successfully to '%s'.", install_path);
      goto exit_section; // Using goto for this specific control flow
    } else if (strcmp(first_arg, "--run") == 0) {
      run_only = true;
    } else if (strcmp(first_arg, "--crun") == 0) {
      compile_and_run = true;
    } else {
      nob_log(NOB_ERROR, "Unknown argument: %s", first_arg);
      help(); // Show help and exit for unknown args
    }
  } else {
    // Default behavior if no arguments are provided (after NOB_GO_REBUILD_URSELF)
    // Could print help, or proceed to default build. For now, let it proceed to build.
    nob_log(NOB_INFO, "No specific arguments provided. Proceeding with default build process.");
  }

  // Preparations for build
  if (nob_file_exists("./nob.old")) {
    nob_log(NOB_INFO, "Removing old './nob.old' file...");
    if (remove_file("./nob.old") != 0) {
      nob_log(NOB_WARNING, "Failed to remove './nob.old'. Continuing anyway.");
    }
  }

  if (!nob_mkdir_if_not_exists("./build")) {
    nob_log(NOB_ERROR, "Failed to create build directory './build'.");
    return 1;
  }

  // Raylib library handling
  if (nob_file_exists("./lib/src/libraylib.a")) {
    nob_log(NOB_INFO, "Copying './lib/src/libraylib.a' to './build/libraylib.a'...");
    if (!nob_copy_file("./lib/src/libraylib.a", "./build/libraylib.a")) {
      nob_log(NOB_ERROR, "Failed to copy libraylib.a to build directory.");
      return 1;
    }
  } else {
    nob_log(NOB_ERROR, "'./lib/src/libraylib.a' is missing.");
    nob_log(NOB_INFO, "Attempting to create './lib/src/' for manual build of Raylib.");
    if (!nob_mkdir_if_not_exists("./lib/src/")) {
      nob_log(NOB_ERROR, "Failed to create directory './lib/src/'.");
      return 1;
    }
    nob_log(NOB_INFO, "Please ensure Raylib is built manually in './lib/src/'.");
    exit(0); // Exiting as Raylib is a critical dependency for compilation.
  }

  // Run pre-compiled binary if --run is passed
  if (run_only) {
    char build_file_path[256];
    snprintf(build_file_path, sizeof(build_file_path), "./build/%s", SRC_NAME);
    nob_log(NOB_INFO, "----------------------------------------");
    nob_log(NOB_INFO, "Running pre-compiled binary: %s", build_file_path);
    nob_log(NOB_INFO, "----------------------------------------");
    Nob_Cmd runner_cmd = {0};
    nob_cmd_append(&runner_cmd, build_file_path);
    if (!nob_cmd_run_sync(runner_cmd)) {
        nob_log(NOB_ERROR, "Failed to run '%s'.", build_file_path);
        return 1;
    }
    // If run_only is true, we assume it's not an error to exit here.
    // The original code had `if (is_run) { exit(-1); }` after this block,
    // which seems contradictory if running was successful.
    // Assuming successful run means exit 0.
    exit(0);
  }

  // Compile bundle.c (if it exists and is part of the build logic)
  // This section seems specific; ensure bundle.c is relevant.
  if (nob_file_exists("./src/bundle.c")) {
      nob_log(NOB_INFO, "Compiling bundle...");
      Nob_Cmd bundle_cmd = {0};
      nob_cmd_append(&bundle_cmd, COMPILER);
      nob_cmd_append(&bundle_cmd, "-Oz", "-s"); // Optimization and strip symbols
      nob_cmd_append(&bundle_cmd, "-o", "./build/bundle", "./src/bundle.c");
      if (!nob_cmd_run_sync(bundle_cmd)) {
          nob_log(NOB_ERROR, "Failed to compile bundle.c.");
          // Decide if this is a fatal error. For now, let's assume it might be optional.
          // return 1; 
      }

      // Run compiled bundle (if its purpose is to be run during build)
      // This implies bundle is a tool used in the build process.
      nob_log(NOB_INFO, "Running compiled bundle utility...");
      Nob_Cmd run_bundle_cmd = {0};
      nob_cmd_append(&run_bundle_cmd, "./build/bundle");
      if (!nob_cmd_run_sync(run_bundle_cmd)) {
          nob_log(NOB_ERROR, "Failed to run compiled bundle utility from './build/bundle'.");
          // Decide if this is fatal.
          // return 1;
      }
  } else {
      nob_log(NOB_INFO, "./src/bundle.c not found, skipping bundle compilation and execution.");
  }
  nob_log(NOB_INFO, "----------------------------------------");

compile_section: // Label for goto SRC_NAME == "test"
  nob_log(NOB_INFO, "Compiling main target '%s' with debug symbols: %s", SRC_NAME, debug_build ? "Yes" : "No");
  Nob_Cmd compile_cmd = {0};
  nob_cmd_append(&compile_cmd, COMPILER);
  if (debug_build) {
    nob_cmd_append(&compile_cmd, "-Wall", "-Wextra", "-Wpedantic", "-g");
  } else {
    nob_cmd_append(&compile_cmd, "-Oz", "-s"); // Optimization and strip symbols
  }

#ifdef _WIN32
  nob_cmd_append(&compile_cmd, "-I./headers");
  nob_cmd_append(&compile_cmd, "-o", nob_temp_sprintf("./build/%s", SRC_NAME), src_file_path);
  nob_cmd_append(&compile_cmd, "-L./build/", "-l:libraylib.a");
  nob_cmd_append(&compile_cmd, "-lopengl32", "-lgdi32", "-lwinmm");
#elif __linux__ // Using elif for better structure if more platforms are added
  nob_cmd_append(&compile_cmd, "-I./headers");
  nob_cmd_append(&compile_cmd, "-o", nob_temp_sprintf("./build/%s", SRC_NAME), src_file_path);
  nob_cmd_append(&compile_cmd, "-L./build/", "-l:libraylib.a"); // Corrected -l: to -lraylib if libraylib.a is named that way
  nob_cmd_append(&compile_cmd, "-lm", "-ldl", "-lpthread");
#else
  nob_log(NOB_ERROR, "Unsupported platform for compilation.");
  return 1;
#endif

  if (!nob_cmd_run_sync(compile_cmd)) {
    nob_log(NOB_ERROR, "Failed to compile '%s'.", src_file_path);
    return 1;
  }
  nob_log(NOB_INFO, "Compilation of '%s' successful.", src_file_path);

  // Run compiled binary if --crun is passed
  if (compile_and_run) {
    char build_file_path[256];
    snprintf(build_file_path, sizeof(build_file_path), "./build/%s", SRC_NAME);
    nob_log(NOB_INFO, "----------------------------------------");
    nob_log(NOB_INFO, "Running compiled binary: %s", build_file_path);
    nob_log(NOB_INFO, "----------------------------------------");
    Nob_Cmd crunner_cmd = {0};
    nob_cmd_append(&crunner_cmd, build_file_path);
    if (!nob_cmd_run_sync(crunner_cmd)) {
        nob_log(NOB_ERROR, "Failed to run '%s' after compilation.", build_file_path);
        return 1;
    }
  }

exit_section: // Label for goto --install
  nob_log(NOB_INFO, "----------------------------------------");
  nob_log(NOB_INFO, "Build process finished.");
  return EXIT_SUCCESS;
}
