#include "cvi_json_struct_comm.h"
#include "cvi_json_object.h"

JSON_EXPORT int cvi_json_object_put(struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT int cvi_json_object_is_type(const struct cvi_json_object *obj, enum cvi_json_type type)
{
	return 0;
}

JSON_EXPORT enum cvi_json_type cvi_json_object_get_type(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT const char *cvi_json_object_to_cvi_json_string(struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_object(void)
{
	return 0;
}

JSON_EXPORT int cvi_json_object_object_add(struct cvi_json_object *obj, const char *key,
                                       struct cvi_json_object *val)
{
	return 0;
}

JSON_EXPORT int cvi_json_object_object_add_ex(struct cvi_json_object *obj, const char *const key,
                                          struct cvi_json_object *const val, const unsigned opts)
{
	return 0;
}

JSON_EXPORT cvi_json_bool cvi_json_object_object_get_ex(const struct cvi_json_object *obj, const char *key,
                                                struct cvi_json_object **value)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_array(void)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_array_ext(int initial_size)
{
	return 0;
}

JSON_EXPORT size_t cvi_json_object_array_length(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT int cvi_json_object_array_add(struct cvi_json_object *obj, struct cvi_json_object *val)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_array_get_idx(const struct cvi_json_object *obj,
                                                          size_t idx)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_int(int32_t i)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_int64(int64_t i)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_uint64(uint64_t i)
{
	return 0;
}

JSON_EXPORT int32_t cvi_json_object_get_int(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT int64_t cvi_json_object_get_int64(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT uint64_t cvi_json_object_get_uint64(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_double(double d)
{
	return 0;
}

#if 0
JSON_EXPORT struct cvi_json_object *cvi_json_object_new_double_s(double d, const char *ds)
{
	return 0;
}
#endif

JSON_EXPORT double cvi_json_object_get_double(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_object_new_string_len(const char *s, const int len)
{
	return 0;
}

JSON_EXPORT const char *cvi_json_object_get_string(struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT int cvi_json_object_get_string_len(const struct cvi_json_object *obj)
{
	return 0;
}

JSON_EXPORT struct cvi_json_object *cvi_json_tokener_parse(const char *str)
{
	return 0;
}
