#ifndef NV_SETTINGS_LEX_H
#define NV_SETTINGS_LEX_H

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum nvs_token_type
  {
    NVS_TOKEN_INT,
    NVS_TOKEN_FLOAT,
    NVS_TOKEN_STRING,
    NVS_TOKEN_IDENT, // identifier
  } nvs_token_type;

#ifdef __cplusplus
}
#endif

#endif