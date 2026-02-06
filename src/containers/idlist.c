#include "../../include/containers/idlist.h"

#include "../../include/alloc.h"
#include "../../include/string.h"

nv_error
nv_id_list_init(size_t type_size, size_t init_capacity, nv_id_list_t* idlist)
{
  nv_bzero(idlist, sizeof(*idlist));

  if (init_capacity == 0) { init_capacity = 2; }

  /* Combined allocation for both the ID table and the elements. */
  void*   data          = nv_zmalloc(init_capacity * type_size);
  size_t* id_to_indices = (size_t*)nv_zmalloc(init_capacity * sizeof(size_t));
  size_t* indices_to_id = (size_t*)nv_zmalloc(init_capacity * sizeof(size_t));
  if (!data || !id_to_indices || !indices_to_id)
  {
    nv_free(data);
    nv_free(id_to_indices);
    nv_free(indices_to_id);
    return NV_ERROR_MALLOC_FAILED;
  }

  idlist->canary      = 0xFEF6324;
  idlist->capacity    = init_capacity;
  idlist->data        = data;
  idlist->id_to_index = id_to_indices;
  idlist->index_to_id = indices_to_id;
  idlist->type_size   = type_size;

  return NV_SUCCESS;
}

void
nv_id_list_destroy(nv_id_list_t* idlist)
{
  if (!idlist) return;
  if (idlist->canary != 0xFEF6324) return;

  nv_free(idlist->id_to_index);
  nv_free(idlist->index_to_id);
  nv_free(idlist->data);

  nv_memset(idlist, 0, sizeof(*idlist));
}

void
nv_id_list_resize(size_t new_capacity, nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, );

  size_t old_size = idlist->capacity * (idlist->type_size + sizeof(size_t));
  size_t new_size = new_capacity * (idlist->type_size + sizeof(size_t));

  void*   new_data        = nv_realloc(idlist->data, new_capacity * idlist->type_size);
  size_t* new_id_to_index = (size_t*)nv_realloc(idlist->id_to_index, new_capacity * sizeof(size_t));
  size_t* new_index_to_id = (size_t*)nv_realloc(idlist->index_to_id, new_capacity * sizeof(size_t));

  if (NV_UNLIKELY(!new_data || !new_id_to_index || !new_index_to_id)) { return; }

  idlist->capacity    = new_capacity;
  idlist->id_to_index = new_id_to_index;
  idlist->index_to_id = new_index_to_id;
  idlist->data        = new_data;
}

void*
nv_id_list_iter(size_t* ctx, nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, NULL);

  if (NV_UNLIKELY(*ctx >= idlist->size)) return NULL;

  void* elem = ((uchar*)idlist->data + (*ctx * idlist->type_size));
  (*ctx)++;

  return elem;
}

void*
nv_id_list_get(size_t id, const nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, NULL);
  if (id >= idlist->size) return NULL;

  size_t idx  = idlist->id_to_index[id];
  void*  elem = ((uchar*)idlist->data + (idx * idlist->type_size));
  return elem;
}

size_t
nv_id_list_push(const void* elem, nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, SIZE_MAX);

  if (idlist->size + 1 >= idlist->capacity || !idlist->data || !idlist->id_to_index) { nv_id_list_resize(NV_MAX(idlist->capacity * 2, 1), idlist); }

  size_t id       = idlist->size;
  uchar* offseted = (uchar*)idlist->data + (idlist->size * idlist->type_size);
  nv_memcpy(offseted, elem, idlist->type_size);

  idlist->id_to_index[id]           = idlist->size;
  idlist->index_to_id[idlist->size] = id;
  idlist->size++;

  return id;
}

void
nv_id_list_pop(nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, );

  if (idlist->size - 1 <= idlist->capacity / 2) { nv_id_list_resize(NV_MAX((idlist->capacity + 1) / 2, 1), idlist); }

  if (idlist->size == 0) return;
  idlist->size--;
}

void
swap(void* a, void* b, size_t size)
{
  u8* ba = a;
  u8* bb = b;
  for (size_t i = 0; i < size; i++)
  {
    u8 tmp = ba[i];
    ba[i]  = bb[i];
    bb[i]  = tmp;
  }
}

bool
nv_id_list_delete(size_t id, nv_id_list_t* idlist)
{
  nv_assert_else_return(idlist && idlist->canary == 0xFEF6324, false);

  if (idlist->size == 0 || id >= idlist->size) return false;

  size_t i    = idlist->id_to_index[id];
  size_t last = idlist->size - 1;

  if (i != last)
  {
    uchar* elem      = (uchar*)idlist->data + (i * idlist->type_size);
    uchar* last_elem = (uchar*)idlist->data + (last * idlist->type_size);
    swap(elem, last_elem, idlist->type_size);

    size_t moved_id               = idlist->index_to_id[last];
    idlist->id_to_index[moved_id] = i;
    idlist->index_to_id[i]        = moved_id;
  }

  idlist->id_to_index[id]   = SIZE_MAX;
  idlist->index_to_id[last] = SIZE_MAX;

  idlist->size--;

  // shrink if too big
  if (idlist->size <= idlist->capacity / 2) { nv_id_list_resize(NV_MAX((idlist->capacity + 1) / 2, 1), idlist); }

  return true;
}
