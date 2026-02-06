#ifndef NV_SETTINGS_H
#define NV_SETTINGS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "containers/hashmap.h"
#include "stdafx.h"
#include "types.h"

  /* Internal representation of an expression as a tree. */
  typedef struct nvs_expr_node nvs_expr_node;

  typedef enum nvs_type
  {
    NVS_INT,
    NVS_FLOAT, // always stored as double
    NVS_STRING,
    NVS_EXPRESSION,
  } nvs_type;

  typedef union nvs_value
  {
    int            integer;
    double         floating;
    char*          string;
    nvs_expr_node* expr;
  } nvs_value;

  typedef struct nvs_setting
  {
    u32       name_hash; // No need to fill in by the user!
    char*     name;
    nvs_type  type;
    nvs_value value;
  } nvs_setting;

  typedef struct nvs_handle
  {
    nv_hashmap_t settings;
    u64          ud;
  } nvs_handle;

  nv_error nvs_init(nvs_handle* handle);
  void     nvs_exit(nvs_handle* handle);

  nv_error nvs_set(nvs_handle* handle, nvs_setting setto);
  nv_error nvs_get(nvs_handle* handle, const char* setting_name, nvs_setting* out);
  bool     nvs_exists(const nvs_handle* handle, const char* setting_name);

  nv_error nvs_parse(nvs_handle* handle, const char* expression, nvs_setting* result);

#ifdef __cplusplus
}
#endif

#endif // NV_SETTINGS_H
