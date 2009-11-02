/**************************
NEVER CHANGE THIS FILE MANUALLY!

This File Was Generated By enums_gen.py Automatically.
************************/

#include "../../include/enums/cubemap_faces.h"

#include "eflib/include/eflib.h"

#ifdef _DEBUG
std::set<int>& cubemap_faces::get_cubemap_faces_table()
{
	static std::set<int> ret;
	return ret;
}
#endif

cubemap_faces::cubemap_faces(int v):val(v)
{
#ifdef _DEBUG
	if(get_cubemap_faces_table().find(v) != get_cubemap_faces_table().end()) {
		custom_assert(false, "");
	} else {
		get_cubemap_faces_table().insert(v);
	}
#endif
}

cubemap_faces cubemap_faces::cast(int val)
{
	#ifdef _DEBUG
	if(get_cubemap_faces_table().find(val) == get_cubemap_faces_table().end()){
		custom_assert(false, "");
		return cubemap_faces(0);
	}
	#endif
	return cubemap_faces(val);
}

const cubemap_faces cubemap_faces::invalid(-1);
const cubemap_faces cubemap_faces::pos_x(0);
const cubemap_faces cubemap_faces::neg_x(1);
const cubemap_faces cubemap_faces::pos_y(2);
const cubemap_faces cubemap_faces::neg_y(3);
const cubemap_faces cubemap_faces::pos_z(4);
const cubemap_faces cubemap_faces::neg_z(5);
const cubemap_faces cubemap_faces::max(6);
