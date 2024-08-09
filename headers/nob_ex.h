#pragma once
typedef struct Nob_String_View_List {
  Nob_String_View *list;
  size_t count;
} Nob_String_View_List;
Nob_String_View_List nob_sv_list_from_cstr_list(char **list);
Nob_String_View nob_sv_chop_by_pos(Nob_String_View *sv, size_t pos);
void nob_system(const char *exe, Nob_String_View_List *list);
#ifdef NOB_EX_IMPLEMENTATION
Nob_String_View_List nob_sv_list_from_cstr_list(char **list) {
  int i = 0;
  Nob_String_View_List sv_list = {0};
  while (list[i] != NULL) {
    sv_list.count++;
    i++;
  }
  sv_list.list =
      (Nob_String_View *)malloc(sv_list.count * sizeof(Nob_String_View));
  if (sv_list.list == NULL) {
    nob_log(NOB_ERROR, "Memory allocation error at line %d", __LINE__);
    return sv_list;
  }
  for (i = 0; i < sv_list.count; i++) {
    sv_list.list[i].data = strdup(list[i]);
    sv_list.list[i].count = strlen(list[i]);
  }
  return sv_list;
}
Nob_String_View nob_sv_chop_by_pos(Nob_String_View *sv, size_t pos) {
  Nob_String_View chopped_sv = {0};
  if (pos >= sv->count) {
    nob_log(NOB_ERROR, "Position out of range.");
    return chopped_sv;
  }
  char *data = malloc(sizeof(char) * 256);
  char *data2 = (char *)sv->data;
  int j = 0;
  for (size_t i = pos; i < sv->count; i++) {
    data[j++] = data2[i];
  }
  chopped_sv = nob_sv_from_cstr(data);
  nob_log(NOB_INFO, "Chopped string: %s", chopped_sv.data);
  exit(0);
  return chopped_sv;
}
void nob_system(const char *exe, Nob_String_View_List *list) {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, exe);
  for (size_t i = 0; i < list->count; ++i) {
    nob_cmd_append(&cmd, list->list[i].data);
  }
  nob_cmd_run_sync(cmd);
}
#endif // NOB_EX_IMPLEMENTATION
