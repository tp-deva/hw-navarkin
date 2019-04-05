/*********************************
 * Наваркин Алексей - АПО-12
 *
 * **Time limit:**   14 s
 * **Memory limit:** 64 M
 *
 * Требуется написать программу, которая способна вычислять арифметические
 *выражения.
 * Выражения могут содержать:
 * 1) Знаки операций '+', '-', '/', '*'
 * 2) Скобки '(', ')'
 * 3) Целые и вещественные числа, в нотации '123', '123.345', все операции
 *должны быть вещественны, а результаты выведены с точностю до двух знаков после
 *запятой в том числе целые '2.00'
 * 4) Необходимо учитывать приоритеты операций, и возможность унарного минуса,
 *пробелы ничего не значат
 * 5) Если в выражении встретилась ошибка требуется вывести в стандартный поток
 *вывода "[error]" (без кавычек)
 *
 *********************************/

#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PERROR fputs("[error]\n", stdout)

#define LEXEME_STACK_INIT_SIZE 40
#define LEXEME_STACK_GROW_FACTOR 2

typedef enum {
  NONE,
  OPERATOR,
  OPERAND,
  L_BR,
  R_BR,
  END_OF_EXPR,
  END_OF_FILE,
} lexeme_type;

typedef enum { SUM, SUB, DIV, MUL } operator_type;

typedef union {
  float operand;
  operator_type operator;
} lexeme_data;

typedef struct {
  lexeme_type type;
  lexeme_data data;
} lexeme;

typedef struct {
  lexeme *lexemes;
  size_t size;
  size_t top;
} lexeme_stack;

typedef struct {
  lexeme_stack *operators;
  lexeme_stack *operands;
} reverse_notation;

/*!
 * @brief function that evaluates arithmetical expression
 * @param raw_str - null terminated string containing expression
 * @param result - variable where result would be stored
 * @return status code
 */
int evaluate(const char *raw_str, float *result);

/*!
 * @brief function that inits reverse notation structure
 * @param notation - structure ptr to init
 * @return status code
 */
int reverse_notation_init(reverse_notation *notation);
void reverse_notation_free(reverse_notation *notation);
bool reverse_notation_add_token(reverse_notation *dest, lexeme *lex);
bool reverse_notation_reduce(reverse_notation *dest);
int reverse_notation_get_prior(const lexeme *lex);
static bool reverse_notation_can_reduce(const reverse_notation *origin,
                                        const lexeme *next_lex);

int lexeme_stack_init(lexeme_stack *stack);
void lexeme_stack_free(lexeme_stack *stack);
lexeme lexeme_stack_pop(lexeme_stack *stack);
const lexeme *lexeme_stack_peek(const lexeme_stack *stack);
void lexeme_stack_push(lexeme_stack *stack, lexeme *lex);
bool lexeme_stack_empty(const lexeme_stack *stack);
void free_lexeme(lexeme *lex);

lexeme *get_lexeme(const char *src_str, size_t *str_iter);
bool get_operand(const char *src_str, size_t *str_iter, lexeme *lex);
bool get_operator(const char sym, lexeme *lex);

/************
 *** MAIN ***
 ************/
int main() {
  while (!feof(stdin)) {
    char *input_str = NULL;
    size_t str_size = 0;
    ssize_t input_length = getline(&input_str, &str_size, stdin);
    if (!input_str) {
      PERROR;
      return 0;
    }
    if (input_length == -1) {
      free(input_str);
      return 0;
    }

    float result = 0.0f;

    if (evaluate(input_str, &result)) {
      PERROR;
      free(input_str);
      return 0;
    }

    printf("%.2f\n", result);

    free(input_str);
  }

  return 0;
}
/************
 *** MAIN ***
 ************/

int evaluate(const char *raw_str, float *result) {
  if (!raw_str) {
    return -1;
  }

  reverse_notation notation;
  if (reverse_notation_init(&notation)) {
    return -1;
  }

  size_t str_iter = 0;
  lexeme_type prev = L_BR;
  while (str_iter <= strlen(raw_str)) {
    lexeme *lex = get_lexeme(raw_str, &str_iter);
    if (!lex) {
      reverse_notation_free(&notation);
      return -1;
    }

    // Unary minus
    if (prev == L_BR && lex->type == OPERATOR && lex->data.operator== SUB) {
      lexeme zero_token = {OPERAND, {0.0f}};
      lexeme_stack_push(notation.operands, &zero_token);
    }

    if (!reverse_notation_add_token(&notation, lex)) {
      reverse_notation_free(&notation);
      free(lex);
      return -1;
    }

    prev = lex->type;

    free(lex);
  }

  if (lexeme_stack_empty(notation.operands)) {
    reverse_notation_free(&notation);
    return -1;
  }

  *result = lexeme_stack_pop(notation.operands).data.operand;

  if (!lexeme_stack_empty(notation.operators)) {
    reverse_notation_free(&notation);
    return -1;
  }

  reverse_notation_free(&notation);

  return 0;
}

/******************************
 *
 * *****REVERSE NOTATION
 *          BEGIN
 ******************************/
int reverse_notation_init(reverse_notation *notation) {  // TODO: FREE STACK
  if (!notation) {
    return -1;
  }

  lexeme_stack *stack = malloc(sizeof(lexeme_stack));
  if (!stack) {
    return -1;
  }

  notation->operands = stack;

  if (lexeme_stack_init(notation->operands)) {
    return -1;
  }

  stack = malloc(sizeof(lexeme_stack));
  if (!stack) {
    return -1;
  }

  notation->operators = stack;

  if (lexeme_stack_init(notation->operators)) {
    return -1;
  }

  return 0;
}

void reverse_notation_free(reverse_notation *notation) {
  if (!notation) {
    return;
  }

  if (notation->operators) {
    lexeme_stack_free(notation->operators);
    free(notation->operators);
  }

  if (notation->operands) {
    lexeme_stack_free(notation->operands);
    free(notation->operands);
  }
}

bool reverse_notation_reduce(reverse_notation *dest) {
  char action = lexeme_stack_pop(dest->operators).data.operator;

  if (lexeme_stack_empty(dest->operands)) {
    return false;
  }
  float right = lexeme_stack_pop(dest->operands).data.operand;

  if (lexeme_stack_empty(dest->operands)) {
    return false;
  }
  float left = lexeme_stack_pop(dest->operands).data.operand;

  lexeme result;
  result.type = OPERAND;

  switch (action) {
    case SUM:
      result.data.operand = left + right;
      break;
    case SUB:
      result.data.operand = left - right;
      break;
    case DIV:
      if (right == 0) {
        return false;
      }
      result.data.operand = left / right;
      break;
    case MUL:
      result.data.operand = left * right;
      break;
    default:
      return false;
  }

  lexeme_stack_push(dest->operands, &result);

  return true;
}

int reverse_notation_get_prior(const lexeme *lex) {
  if (!lex) {
    return 0;
  }

  switch (lex->type) {
    case OPERATOR:
      break;
    case L_BR:
      return -1;
    case END_OF_EXPR:
    case END_OF_FILE:
      return 3;
    default:
      break;
  }

  if (lex->type == OPERATOR) {
    switch (lex->data.operator) {
      case MUL:
      case DIV:
        return 1;
      case SUM:
      case SUB:
        return 2;
    }
  }

  return 0;
}

static bool reverse_notation_can_reduce(const reverse_notation *origin,
                                        const lexeme *next_lex) {
  if (lexeme_stack_empty(origin->operators)) {
    return false;
  }

  int priority_curr =
      reverse_notation_get_prior(lexeme_stack_peek(origin->operators));
  int priority_new = reverse_notation_get_prior(next_lex);

  return (priority_curr >= 0 && priority_new >= 0 &&
          priority_new >= priority_curr);
}

bool reverse_notation_add_token(reverse_notation *dest, lexeme *lex) {
  if (!lex) {
    return false;
  }

  if (lex->type == R_BR) {
    while (!lexeme_stack_empty(dest->operators) &&
           lexeme_stack_peek(dest->operators)->type != L_BR) {
      if (!reverse_notation_reduce(dest)) {
        return false;
      }
    }
    lexeme_stack_pop(dest->operators);

    return true;
  }

  while (reverse_notation_can_reduce(dest, lex)) {
    if (!reverse_notation_reduce(dest)) {
      return false;
    }
  }

  switch (lex->type) {
    case NONE:
    case END_OF_EXPR:
    case END_OF_FILE:
      break;
    case OPERAND:
      lexeme_stack_push(dest->operands, lex);
      break;
    default:
      lexeme_stack_push(dest->operators, lex);
      break;
  }

  return true;
}

/*****************************
 *
 * ******LEXEME STACK
 *
 *****************************/
int lexeme_stack_init(lexeme_stack *stack) {
  if (!stack) {
    return -1;
  }

  stack->size = LEXEME_STACK_INIT_SIZE;
  stack->top = 0;

  stack->lexemes = (lexeme *)malloc(sizeof(lexeme) * LEXEME_STACK_INIT_SIZE);
  if (!stack->lexemes) {
    return -1;
  }

  return 0;
}

void lexeme_stack_free(lexeme_stack *stack) {
  if (!stack) {
    return;
  }

  if (!stack->lexemes) {
    return;
  }

  free(stack->lexemes);
}

bool lexeme_stack_empty(const lexeme_stack *stack) {
  if (!stack) {
    return true;
  }

  if (stack->top == 0) {
    return true;
  }

  return false;
}

size_t lexeme_stack_grow(lexeme_stack *stack) {
  if (!stack) {
    return 0;
  }

  size_t new_size = stack->size * LEXEME_STACK_GROW_FACTOR;

  lexeme *new_lexemes = (lexeme *)realloc(stack->lexemes, new_size);
  if (!new_lexemes) {
    return 0;
  }

  free(stack->lexemes);

  stack->lexemes = new_lexemes;
  stack->size = new_size;

  return new_size;
}

lexeme lexeme_stack_pop(lexeme_stack *stack) {
  if (!stack || stack->top == 0) {
    lexeme lex = {NONE, {0.0f}};
    return lex;
  }

  return stack->lexemes[--stack->top];
}

const lexeme *lexeme_stack_peek(const lexeme_stack *stack) {
  if (!stack || stack->top == 0) {
    return NULL;
  }

  return &(stack->lexemes[stack->top - 1]);
}

void lexeme_stack_push(lexeme_stack *stack, lexeme *lex) {
  if (!stack || !lex) {
    return;
  }

  if (stack->top == stack->size) {
    lexeme_stack_grow(stack);
  }

  stack->lexemes[stack->top++] = *lex;
}

void free_lexeme(lexeme *lex) {
  if (!lex) {
    return;
  }
  free(lex);
}

/*****************************
 *
 *****************************/

bool get_operand(const char *src_str, size_t *str_iter, lexeme *lex) {
  if (!src_str || !str_iter || !lex) {
    return false;
  }

  if (sscanf(src_str + *str_iter, "%f", &lex->data.operand) != 1) {
    lex->type = NONE;
    return false;
  }
  lex->type = OPERAND;

  for (; isdigit(src_str[*str_iter]); ++(*str_iter))
    ;
  if (src_str[*str_iter] == '.') {
    ++(*str_iter);
    for (; isdigit(src_str[*str_iter]); ++(*str_iter))
      ;
  }

  return true;
}

bool get_operator(const char sym, lexeme *lex) {
  if (!lex) {
    return false;
  }

  switch (sym) {
    case '+':
      lex->type = OPERATOR;
      lex->data.operator= SUM;
      return true;
    case '-':
      lex->type = OPERATOR;
      lex->data.operator= SUB;
      return true;
    case '*':
      lex->type = OPERATOR;
      lex->data.operator= MUL;
      return true;
    case '/':
      lex->type = OPERATOR;
      lex->data.operator= DIV;
      return true;
    case '(':
      lex->type = OPERATOR;
      lex->type = L_BR;
      return true;
    case ')':
      lex->type = OPERATOR;
      lex->type = R_BR;
      return true;
    case '\n':
    case '\0':
      lex->type = END_OF_EXPR;
      return true;
    case EOF:
      lex->type = END_OF_FILE;
      return true;
    default:
      lex->type = NONE;
      return false;
  }
}

lexeme *get_lexeme(const char *src_str, size_t *str_iter) {
  if (!src_str || !str_iter) {
    return NULL;
  }

  lexeme *lex = (lexeme *)malloc(sizeof(lexeme));
  if (!lex) {
    return NULL;
  }

  char sym = src_str[*str_iter];
  ++(*str_iter);
  while (sym == ' ') {
    sym = src_str[*str_iter];
    ++(*str_iter);
  }

  if (get_operator(sym, lex)) {
    return lex;
  }

  --(*str_iter);

  if (get_operand(src_str, str_iter, lex)) {
    return lex;
  }

  free_lexeme(lex);
  return NULL;
}
