#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_ARR_ALLOC_SIZE 10

#define PERROR puts("ERROR")

/*
 * Navarkin Alexey АПО-12 A-8
 * Составить программу построчной обработки текста,
 * в результате которой каждая группа повторяющихся
 * пробелов заменяется на один пробел.
 *
 * Текстовые строки подаются на стандартный ввод программы,
 * результат программы должен подаваться на стандартный вывод.
 *
 * Процедура обработки должна быть оформлена в виде отдельной функции,
 * которой подается на вход массив строк,
 * выделенных в динамической памяти, и его длина.
 *
 * На выход функция должна возвращать массив обработанных строк.
 * */

/*! @brief Get lines from stream and store them into lines ptr
 *
 *  @param ptr lines arrays of strings write to can be NULL
 *  @param stream read from, if NULL stdin will be used
 *  @return number of read lines
 */
size_t get_lines(char ***lines, FILE *stream);

/*! @brief Removes extra whitespace from line
 *
 *  @param line ptr to line
 *  @return line without extra ws
 */
char *trunc_extra_ws(const char *line);

/*! @brief Removes extra whitespaces from lines in array
 *
 * @param lines array of lines
 * @param n number of lines in array
 * @return new allocated array of lines without extra whitespaces
 */
char **trunc_extra_ws_arr(const char *const *lines, size_t n);

/*! @brief Frees memory previously allocated for array of lines
 *
 * @param string_arr array of string previously allocated in dyn mem
 * @param n number of lines in array
 */
void free_lines_arr(char **string_arr, size_t n);

int main() {
  char **lines = NULL;
  size_t size = get_lines(&lines, stdin);
  if (!lines) {
    PERROR;
    return 0;
  }

  char **trunc_lines = trunc_extra_ws_arr((const char **)lines, size);
  if (!trunc_lines) {
    free_lines_arr(lines, size);
    PERROR;
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
  if (!string_arr) {
    return;
  }
  for (size_t i = 0; i < n; ++i) {
    if (string_arr[i] != NULL) {
      free(string_arr[i]);
    }
  }
  free(string_arr);
}

char **trunc_extra_ws_arr(const char *const *lines, size_t n) {
  if (!lines) {
    return NULL;
  }

  char **lines_trunc = (char **)malloc(n * sizeof(char *));
  if (!lines_trunc) {
    return NULL;
  }

  char *line = NULL;
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

char *trunc_extra_ws(const char *line) {
  if (!line) {
    return NULL;
  }

  size_t size = strlen(line) + 1;
  char *res_line = (char *)malloc(size);
  if (!res_line) {
    return NULL;
  }

  size_t pos = 0;
  for (size_t i = 0; line[i] != '\0'; ++i) {
    if (line[i] == ' ' && line[i] == line[i + 1]) {
      continue;
    }
    res_line[pos++] = line[i];
  }
  res_line[pos++] = '\0';

  if (pos != size) {
    char *tmp_line = realloc(res_line, pos);
    if (!tmp_line) {
      free(res_line);
      return NULL;
      // should it throw error on fault?
      // res_line ptr is not corrupted we can return it
    }
    res_line = tmp_line;
  }

  return res_line;
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

  if (pos != LINE_ARR_ALLOC_SIZE * n_alloc) {
    size_t size = sizeof(char *) * pos;
    char **tmp_lines = (char **)realloc(*lines, size);
    // should it throw error on fault?
    // lines ptr is not corrupted we can return it
    if (!tmp_lines) {
      free_lines_arr(*lines, pos);
      return 0;
    }
    (*lines) = tmp_lines;
  }

  return pos;
}
