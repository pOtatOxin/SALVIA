/**************************
NEVER CHANGE THIS FILE MANUALLY!

This File Was Generated By enums_gen.py Automatically.
************************/

#ifndef SOFTART_ENUMS_INPUT_REGISTER_USAGE_H
#define SOFTART_ENUMS_INPUT_REGISTER_USAGE_H

#ifdef _DEBUG
#include <set>
#endif

class input_register_usage{

#ifdef _DEBUG
	static std::set<int>& get_input_register_usage_table();
#endif
	int val;
	explicit input_register_usage(int v);
	
public:
	static input_register_usage cast(int val);
	
	input_register_usage(const input_register_usage& rhs):val(rhs.val){}
	
	input_register_usage& operator =(const input_register_usage& rhs){ val=rhs.val; }
	
	operator int(){ return val; }
	
	friend inline bool operator == (const input_register_usage& lhs, const input_register_usage& rhs)
	{ return lhs.val == rhs.val; }
	
	friend inline bool operator != (const input_register_usage& lhs, const input_register_usage& rhs)
	{ return lhs.val != rhs.val; }
	
	static const input_register_usage invalid;
	static const input_register_usage position;
	static const input_register_usage attribute;
	static const input_register_usage max;
};

template<class T> T enum_cast(int val);

template<> inline input_register_usage enum_cast<input_register_usage>(int val){
	return input_register_usage::cast(val);
}

#endif