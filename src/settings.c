#include "../include/settings.h"

#include "../include/containers/list.h"

nv_error
nvs_init(nvs_handle* handle)
{
}

void
nvs_exit(nvs_handle* handle)
{
}

nv_error
nvs_set(nvs_handle* handle, nvs_setting setto)
{
}

nv_error
nvs_get(nvs_handle* handle, const char* setting_name, nvs_setting* out)
{
}

bool
nvs_exists(const nvs_handle* handle, const char* setting_name)
{
}

nv_error
nvs_parse(nvs_handle* handle, const char* expression, nvs_setting* result)
{
}