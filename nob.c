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
  char *src_file="./src/"SRC_NAME".c";
  if (!nob_file_exists(src_file)) {
     nob_log(NOB_INFO,"Src File doesn't exists: %s",src_file);
     exit(-1);
  }
  bool debug=true;
  bool is_crun=false;
   Nob_Cmd cmd = {0};
  bool is_run=false;
  if (strcmp(SRC_NAME,"test")==0) {
      goto compile;
  }
  if (argv[1] != NULL) {
    if ((strcmp(argv[1], "help")) == 0) {
      help();
    } else if ((strcmp(argv[1], "killy")) == 0) {
      if (DEV_FEATURE) {
        if (remove("./nob")) {
          nob_log(NOB_INFO, "Killing youself -> OK.");
          exit(0);
        }
      } else {
        nob_log(NOB_INFO, "The killy feature is not implemented yet");
        exit(0);
      }
    } else if ((strcmp(argv[1], "clean")) == 0) {
      nob_log(NOB_INFO, "The clean feature is not implemented yet");
      exit(0);
    } else if ((strcmp(argv[1], "--no-debug")) == 0) {
        debug=false;
    }else if ((strcmp(argv[1], "--install")) == 0) {
        system("rm -rf "INSTALL_FOLDER"/fri"); 
        if(!nob_copy_file("./build/fri",INSTALL_FOLDER"/fri")){
            return -1;
        };
        goto exit;
    }else if ((strcmp(argv[1], "--run")) == 0) {
        is_run=true;
    } else if ((strcmp(argv[1], "--crun")) == 0) {
        is_crun=true;
    } else {
      nob_log(NOB_INFO, "Type 'help' help you out to know available flags.");
      exit(1);
    }
  } else {
    nob_log(NOB_INFO, "Type 'help' help you out to know available flags.");
  }
  if (nob_file_exists("./nob.old")) {
    remove("./nob.old");
  }
  if (!nob_mkdir_if_not_exists("./build"))
    return -1;
  if (nob_file_exists("./lib/src/libraylib.a")) {
    nob_copy_file("./lib/src/libraylib.a", "./build/libraylib.a");
  } else {
    nob_log(NOB_ERROR, "'./lib/src/raylib.a' is misssing.");
    nob_log(NOB_INFO, "Creating './lib/src/' for manual build.");
    if (!nob_mkdir_if_not_exists("./lib/src/"))
      return -1;
    exit(0);
  }
  if (is_run) {
  char *build_file="./build/"SRC_NAME;
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
  nob_log(NOB_INFO,"Running: %s",build_file);
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
   Nob_Cmd runner={0}; 
   nob_cmd_append(&runner,build_file);
   nob_cmd_run_sync(runner);
 }
  if (is_run) {
      exit(-1);
  }
  nob_log(NOB_INFO,"Compiling with debug ( %s ) ....",debug==true?"Yes":"No");
  Nob_Cmd bundle = {0};
  Nob_Cmd run_bundle = {0};
  nob_cmd_append(&bundle,COMPILER); 
  nob_cmd_append(&bundle, "-Oz","-s");
  nob_cmd_append(&bundle, "-o","./build/bundle","./src/bundle.c");
  nob_cmd_run_sync(bundle);
  nob_cmd_append(&run_bundle,"./build/bundle");
  nob_cmd_run_sync(run_bundle);
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
compile:
  nob_cmd_append(&cmd, COMPILER);
  if (debug) {
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-Wpedantic","-g");
  }else {
    nob_cmd_append(&cmd, "-Oz","-s");
  }
#ifdef _WIN32
  nob_cmd_append(&cmd, "-I./headers");
  nob_cmd_append(&cmd, "-o", "./build/" SRC_NAME, "./src/" SRC_NAME ".c");
  nob_cmd_append(&cmd, "-L./build/", "-l:libraylib.a");
  nob_cmd_append(&cmd, "-lopengl32", "-lgdi32", "-lwinmm");
#endif
#ifdef __linux__
  nob_cmd_append(&cmd, "-I./headers");
  nob_cmd_append(&cmd, "-o", "./build/" SRC_NAME, "./src/" SRC_NAME ".c");
  nob_cmd_append(&cmd, "-L./build/", "-l:libraylib.a");
  nob_cmd_append(&cmd, "-lm", "-ldl", "-lpthread");
#endif
  nob_cmd_run_sync(cmd);
  if (is_crun) {
  char *run="./build/"SRC_NAME;
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
  nob_log(NOB_INFO,"Running: %s",run);
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
   Nob_Cmd runner={0}; 
   nob_cmd_append(&runner,run);
   nob_cmd_run_sync(runner);
 }
exit:
  nob_log(NOB_INFO,"-|-|-|-|-|-|-");
  return EXIT_SUCCESS;
}
