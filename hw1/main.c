#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_ARR_ALLOC_SIZE 10

//TODO: maybe realloc on EOF and \n but it harms on speed

// Get lines from stream and store them into lines ptr
size_t get_lines(char ***lines, FILE *stream);

// Removes extra whitespace from line
char *trunc_extra_ws(const char *line);

// Removes extra whitespaces from lines in array
char **trunc_extra_ws_arr(char **lines, size_t n);

// Frees memory previously allocated for array of lines
void free_lines_arr(char **string_arr, size_t n);

int main() {
  char **lines = NULL;
  size_t size = get_lines(&lines, stdin);
  if (!lines || size <= 0) {
    puts("[error]");
    return 0;
  }

  char **trunc_lines = trunc_extra_ws_arr(lines, size);
  if (!trunc_lines) {
    free_lines_arr(lines, size);
    puts("[error]");
    return 0;
  }

  for (size_t i = 0; i < size; i++) {
    fputs(trunc_lines[i], stdout);
  }

  free_lines_arr(lines, size);
  free_lines_arr(trunc_lines, size);

  return 0;
}

void free_lines_arr(char **string_arr, size_t n) {
  if (!string_arr || n <= 0) {
    return;
  }
  for (size_t i = 0; i < n; ++i) {
    if (string_arr[i] != NULL) {
      free(string_arr[i]);
    }
  }
  free(string_arr);
}

char **trunc_extra_ws_arr(char **lines, size_t n) {
  if (!lines || n <= 0) {
    return NULL;
  }

  char **lines_trunc = (char **)malloc(n * sizeof(char *));
  if (!lines_trunc) {
    return NULL;
  }

  char *line;
  for (size_t i = 0; i < n; ++i) {
    line = trunc_extra_ws(lines[i]);
    if (!line) {
      free_lines_arr(lines_trunc, i + 1);
      return NULL;
    }
    lines_trunc[i] = line;
  }
  return lines_trunc;
}

size_t get_lines(char ***lines, FILE *stream) {
  if (!stream) {
    stream = stdin;
  }

  size_t pos = 0;
  size_t n_alloc = 0;
  size_t line_len = 0;
  char *line = NULL;
  while (!feof(stream)) {
    if (getline(&line, &line_len, stream) == -1) {
      if (errno == ENOMEM || errno == EINVAL) {
        free_lines_arr(*lines, pos);
        return 0;
      }
      if (feof(stream)) {
        line[0] = '\0';
      }
    }

    if (pos >= LINE_ARR_ALLOC_SIZE * n_alloc) {
      size_t size = sizeof(char *) * LINE_ARR_ALLOC_SIZE * ++n_alloc;
      char **lines_new = (char **)realloc(*lines, size);
      if (!lines_new) {
        free_lines_arr(*lines, ++pos);
        return 0;
      }
      *lines = lines_new;
    }
    (*lines)[pos++] = line;
    line_len = 0;
    line = NULL;
  }
  return pos;
}

char *trunc_extra_ws(const char *line) {
  if (!line) {
    return NULL;
  }

  size_t pos = 0;
  char *res_line = (char *)malloc(strlen(line) + 1);
  if (!res_line) {
    return NULL;
  }
  for (size_t i = 0; line[i] != '\0'; ++i) {
    if (line[i] == ' ' && line[i] == line[i + 1]) {
      continue;
    }
    res_line[pos++] = line[i];
  }
  res_line[pos] = '\0';
  return res_line;
}
