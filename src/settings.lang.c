#include "../include/settings.h"

#include "../include/chrclass.h"
#include "../include/containers/list.h"
#include "../include/strconv.h"

typedef enum nvs_expr_type
{
  NVS_EXPR_TYPE_ROOT,
  NVS_EXPR_TYPE_BINARY_OP,
  NVS_EXPR_TYPE_UNARY_OP,
  NVS_EXPR_VARIABLE,
  NVS_EXPR_ENCLOSED_EXPRESSION, // ( expr )

  NVS_EXPR_INT,
  NVS_EXPR_FLOAT,
  NVS_EXPR_STRING,

  NVS_EXPR_ERROR,
} nvs_expr_type;

typedef enum nvs_operator
{
  NVS_OPERATOR_ADD,  // +
  NVS_OPERATOR_SUB,  // -
  NVS_OPERATOR_MUL,  // *
  NVS_OPERATOR_DIV,  // /
  NVS_OPERATOR_MOD,  // %
  NVS_OPERATOR_EXP,  // exponent **
  NVS_OPERATOR_AND,  // binary &&
  NVS_OPERATOR_OR,   // binary ||
  NVS_OPERATOR_NOT,  // binary !
  NVS_OPERATOR_BAND, // bitwise &
  NVS_OPERATOR_BOR,  // bitwise OR
  NVS_OPERATOR_BNOT, // bitwise ~
  NVS_OPERATOR_BXOR, // bitwise ^
} nvs_operator;

struct nvs_expr_node
{
  nvs_expr_type type;
  union
  {
    nv_list_t exprs;
    char*     identifier;
    i64       integer;
    double    floating;
    struct
    {
      nvs_expr_node* left;
      nvs_expr_node* right;
      nvs_operator   op;
    } binaryop;
    struct
    {
      nvs_expr_node* right;
      nvs_operator   op;
    } unary;
  } value;
};

static inline nvs_expr_node
parse_expr(const char* begin, const char** end)
{
  if (nv_isalpha(*begin) || *begin == '_')
  {
    *end = begin;
    while (nv_isalpha(**end) || **end == '_') (*end)++;

    char* str = nv_substr(begin, 0, *end - begin);

    return (nvs_expr_node){ .type = NVS_EXPR_VARIABLE, .value.identifier = str };
  }
  else if (nv_isdigit(*begin))
  {
    *end = begin;
    while (nv_isdigit(**end) || **end == '.' || **end == 'e') (*end)++;

    bool has_decimal = nv_strchr(begin, '.') != NULL;

    if (has_decimal) // floating point
    {
      double floating = nv_atof2(begin, SIZE_MAX, (char**)end);
      return (nvs_expr_node){ .type = NVS_EXPR_FLOAT, .value.floating = floating };
    }
    else
    {
      i64 integer = nv_atoi2(begin, SIZE_MAX, (char**)end);
      return (nvs_expr_node){ .type = NVS_EXPR_INT, .value.integer = integer };
    }
  }
  else if (*begin == '(')
  {
    nvs_expr_node evaluated = parse_expr(begin, end);
    if (**end != ')')
    {
      nv_log_error("Expected closing parenthesis\n");
      return (nvs_expr_node){ NVS_EXPR_ERROR };
    }
    return evaluated;
  }
}

nv_error
nvs_parse(nvs_handle* handle, const char* expression, nvs_setting* result)
{
}