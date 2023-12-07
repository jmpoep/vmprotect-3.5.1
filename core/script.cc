#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "script.h"
#include "core.h"
#include "pefile.h"
#include "processors.h"
#include "macfile.h"
#include "elffile.h"
#include "dotnetfile.h"
#include "il.h"
#include "intel.h"
#include "../third-party/libffi/ffi.h"

/**
 * lua utils
 */

void register_class(lua_State *L, const char *class_name, const luaL_Reg *methods, const luaL_Reg *meta_methods = NULL)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, class_name);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	if (meta_methods) {
		for (const luaL_Reg *method = meta_methods; method->name != NULL; method++) {
			lua_pushstring(L, method->name);
			lua_pushcfunction(L, method->func);
			lua_settable(L, metatable);
		}
	}
	lua_pop(L, 1);  // drop metatable

	//luaL_register(L, NULL, methods);  // fill methodtable
	luaL_setfuncs(L, methods, 0);
	lua_pop(L, 1);  // drop methodtable

	//lua_register(L, class_name, create_account);
}

void push_object(lua_State *state, const char *class_name, const void *value)
{
	if (!value) {
		lua_pushnil(state);
		return;
	}

	void *data = lua_newuserdata(state, sizeof(value));
	*reinterpret_cast<const void **>(data) = value;
	luaL_getmetatable(state, class_name);
	lua_setmetatable(state, -2);
}

void *check_object(lua_State *state, int index, const char *class_name)
{
	void **data = (void **)(class_name ? luaL_checkudata(state, index, class_name) : lua_touserdata(state, index));
	return *data;
}

void delete_object(lua_State *state, int index, const char *class_name)
{
	void **data = (void **)luaL_checkudata(state, index, class_name);
	delete reinterpret_cast<IObject *>(*data);
	*data = NULL;
}

uint64_t check_uint64(lua_State *L, int index)
{
	int type = lua_type(L, index);
	uint64_t res = 0;
	switch (type) {
	case LUA_TNUMBER: {
		lua_Number d = lua_tonumber(L,index);
		res = static_cast<uint64_t>(d);
		break;
	}
	case LUA_TSTRING: {
		size_t len = 0;
		const uint8_t *str = (const uint8_t *)lua_tolstring(L, index, &len);
		if (len > 8)
			return luaL_error(L, "The string (length = %d) is not an uint64 string", len);
		for (size_t i = 0; i < len; i++) {
			res |= static_cast<uint64_t>(str[i]) << (i * 8);
		}
		break;
	}
	case LUA_TUSERDATA: {
		uint64_t *data = reinterpret_cast<uint64_t *>(luaL_checkudata(L, index, Uint64Binder::class_name()));
		res = *data;
		break;
	}
	default:
		return luaL_error(L, "argument %d error type %s", index, lua_typename(L, type));
	}
	return res;
}

lua_Integer check_integer(lua_State *L, int index)
{
	if (lua_type(L, index) == LUA_TUSERDATA) {
		uint64_t *data = reinterpret_cast<uint64_t *>(luaL_checkudata(L, index, Uint64Binder::class_name()));
		return static_cast<lua_Integer>(*data);
	}
	else
		return luaL_checkinteger(L, index);
}

struct EnumReg {
	const char *name;
	uint32_t value;
};

void register_enum(lua_State *L, const char *enum_name, const EnumReg *values)
{
	lua_newtable(L); int table = lua_gettop(L);

	for (; values->name; values++) {
		lua_pushstring(L, values->name);
		lua_pushnumber(L, values->value);
		lua_settable(L, table);
	}

	lua_setglobal(L, enum_name);
}

void push_uint64(lua_State *state, uint64_t value)
{
	uint64_t *data = reinterpret_cast<uint64_t *>(lua_newuserdata(state, sizeof(value)));
	*data = value;
	luaL_getmetatable(state, Uint64Binder::class_name());
	lua_setmetatable(state, -2);
}

void push_field(lua_State *state, const char *key, int value)
{
	lua_pushinteger(state, value);
	lua_setfield(state, -2, key);
}

void push_date(lua_State *state, LicenseDate date, const char *format)
{
	if (!format)
		lua_pushinteger(state, date.value());
	else if (strcmp(format, "*t") == 0) {
		lua_createtable(state, 0, 3);
		push_field(state, "day", date.Day);
		push_field(state, "month", date.Month);
		push_field(state, "year", date.Year);
	} else {
		tm t = tm();
		t.tm_year = date.Year - 1900;
		t.tm_mon = date.Month - 1;
		t.tm_mday = date.Day;

		char cc[3];
		cc[0] = '%';
		cc[1] = 0;
		cc[2] = 0;

		luaL_Buffer b;
		luaL_buffinit(state, &b);
		while (*format) {
			if (*format != '%')  /* no conversion specifier? */
				luaL_addchar(&b, *format++);
			else {
				format++;
				cc[1] = *format++;
				char buff[200];  /* should be big enough for any conversion result */
				size_t reslen = strftime(buff, sizeof(buff), cc, &t);
				luaL_addlstring(&b, buff, reslen);
			}
		}
		luaL_pushresult(&b);
	}
}

void push_file(lua_State *state, IFile *file)
{
	const char *class_name = NULL;
	if (dynamic_cast<PEFile *>(file)) {
		class_name = PEFileBinder::class_name();
	} else if (dynamic_cast<MacFile *>(file)) {
		class_name = MacFileBinder::class_name();
	} else if (dynamic_cast<ELFFile *>(file)) {
		class_name = ELFFileBinder::class_name();
	}
	else {
		file = NULL;
	}
	push_object(state, class_name, file);
}

void push_architecture(lua_State *state, IArchitecture *arch)
{
	const char *class_name = NULL;
	if (dynamic_cast<PEArchitecture *>(arch)) {
		class_name = PEArchitectureBinder::class_name();
	} else if (dynamic_cast<MacArchitecture *>(arch)) {
		class_name = MacArchitectureBinder::class_name();
	} else if (dynamic_cast<ELFArchitecture *>(arch)) {
		class_name = ELFArchitectureBinder::class_name();
	} else if (dynamic_cast<NETArchitecture *>(arch)) {
		class_name = NETArchitectureBinder::class_name();
	} else {
		arch = NULL;
	}
	push_object(state, class_name, arch);
}

void push_command(lua_State *state, ICommand *command)
{
	const char *class_name = NULL;
	if (dynamic_cast<IntelCommand *>(command)) {
		class_name = IntelCommandBinder::class_name();
	}
	else if (dynamic_cast<ILCommand *>(command)) {
		class_name = ILCommandBinder::class_name();
	}
	else {
		command = NULL;
	}
	push_object(state, class_name, command);
}

static int log_print(lua_State *state) 
{
	std::string res;
	int n = lua_gettop(state);
	int i;
	lua_getglobal(state, "tostring");
	for (i = 1; i <= n; i++) {
		const char *s;
		size_t l;
		lua_pushvalue(state, -1);  /* function to be called */
		lua_pushvalue(state, i);   /* value to print */
		lua_call(state, 1, 1);
		s = lua_tolstring(state, -1, &l);  /* get result */
		if (s == NULL)
			return luaL_error(state, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		if (i > 1)
			res += "\t";
		res += std::string(s, l);
		lua_pop(state, 1);  /* pop result */
	}
	Script::core()->Notify(mtScript, NULL, res);
	return 0;
}

int open_lib(lua_State *state)
{
	std::string name = luaL_checklstring(state, 1, NULL);
	FFILibrary *lib = new FFILibrary(name);
	if (!lib->value()) {
		delete lib;
		lib = NULL;
	}
	push_object(state, FFILibraryBinder::class_name(), lib);
	return 1;
}

template<lua_CFunction func>
int SafeFunction(lua_State *L)
{
	int result = 0;
	try {
		result = func(L);
	}
	catch(std::runtime_error &e){
		luaL_error(L, e.what());
	}
	catch(...){
		luaL_error(L, "unknown exception");
	}
	return result;
}

/**
 * Uint64Binder
 */

void Uint64Binder::Register(lua_State *state) 
{
	static const luaL_Reg methods[] = {
		{"__add", &add},
		{"__sub", &sub},
		{"__mul", &mul},
		{"__div", &div},
		{"__mod", &mod},
		{"__unm", &unm},
		{"__pow", &pow},
		{"__eq", &eq},
		{"__lt", &lt},
		{"__le", &le},
		{"__tostring", &tostring},
		{NULL, NULL},
	};

	luaL_newmetatable(state, class_name());
	luaL_setfuncs(state, methods, 0);
	lua_pop(state, 1);

	lua_newtable(state);
	lua_pushcfunction(state, _new);
	lua_setfield(state, -2, "new");
	lua_pushcfunction(state, tostring);
	lua_setfield(state, -2, "tostring");
	lua_pop(state, 1);
}

int Uint64Binder::add(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	push_uint64(L, a + b);
	return 1;
}

int Uint64Binder::sub(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	push_uint64(L, a - b);
	return 1;
}

int Uint64Binder::mul(lua_State *L) {
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	push_uint64(L, a * b);
	return 1;
}

int Uint64Binder::div(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	if (b == 0)
		return luaL_error(L, "div by zero");
	push_uint64(L, a / b);
	return 1;
}

int Uint64Binder::mod(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	if (b == 0)
		return luaL_error(L, "mod by zero");
	push_uint64(L, a % b);
	return 1;
}

uint64_t _pow64(uint64_t a, uint64_t b)
{
	if (b == 1)
		return a;
	uint64_t a2 = a * a;
	if (b % 2 == 1) {
		return _pow64(a2, b/2) * a;
	} else {
		return _pow64(a2, b/2);
	}
}

int Uint64Binder::pow(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	uint64_t res;
	if (b > 0) {
		res = _pow64(a, b);
	} else if (b == 0) { //-V
		res = 1;
	} else {
		return luaL_error(L, "pow by negative number %d",(int)b);
	} 
	push_uint64(L, res);
	return 1;
}

int Uint64Binder::unm(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	push_uint64(L, 0 - a);
	return 1;
}

int Uint64Binder::eq(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	lua_pushboolean(L, a == b);
	return 1;
}

int Uint64Binder::lt(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	lua_pushboolean(L, a < b);
	return 1;
}

int Uint64Binder::le(lua_State *L)
{
	uint64_t a = check_uint64(L, 1);
	uint64_t b = check_uint64(L, 2);
	lua_pushboolean(L, a <= b);
	return 1;
}

int Uint64Binder::_new(lua_State *L)
{
	int top = lua_gettop(L);
	int64_t res;
	switch(top) {
		case 0:
			res = 0;
			break;
		case 1:
			res = check_uint64(L, 1);
			break;
		default: {
			int base = (int)luaL_checkinteger(L,2);
			if (base < 2)
				luaL_error(L, "base must be >= 2");
			const char *str = luaL_checkstring(L, 1);
			res = _strtoui64(str, NULL, base);
			break;
		}
	}
	push_uint64(L, res);
	return 1;
}

int Uint64Binder::tostring(lua_State *L)
{
	static const char *hex = "0123456789ABCDEF";
	uint64_t n = *reinterpret_cast<uint64_t *>(luaL_checkudata(L, 1, class_name()));
	int base = (lua_gettop(L) == 1) ? 16 : (int)luaL_checkinteger(L, 2);
	int shift, mask;
	switch(base) {
	case 0: {
		unsigned char buffer[8];
		for (int i = 0; i < 8; i++) {
			buffer[i] = (n >> (i * 8)) & 0xff;
		}
		lua_pushlstring(L,(const char *)buffer, 8);
		return 1;
		}
	case 10: {
		char buffer[40], *ptr = buffer + _countof(buffer);
		while (ptr > buffer) {
			*--ptr = hex[n % 10];
			n /= 10;
			if (n == 0)
				break;
		}
		lua_pushlstring(L, ptr, _countof(buffer) - (ptr - buffer));
		return 1;
	}
	case 2:
		shift = 1;
		mask = 1;
		break;
	case 8:
		shift = 3;
		mask = 7;
		break;
	case 16:
		shift = 4;
		mask = 0xf;
		break;
	default:
		luaL_error(L, "Unsupport base %d",base);
		return 0;
	}
	char buffer[64];
	for (int i = 0; i < 64; i += shift) {
		buffer[i / shift] = hex[(n >> (64 - shift - i)) & mask];
	}
	lua_pushlstring(L, buffer, 64 / shift);
	return 1;
}

/**
 * FFILibrary
 */

FFILibrary::FFILibrary(const std::string &name) 
	: IObject()
{
	value_ = os::LibraryOpen(name);
	if (value_)
		name_ = name;
}

void FFILibrary::close()
{
	if (value_)
		os::LibraryClose(value_);
	name_.clear();
}

FFILibrary::~FFILibrary()
{
	close();
}

/**
 * FFIFunction
 */

FFIFunction::FFIFunction(HMODULE module, const std::string &name)
	: IObject(), ret_(0), abi_(0), ffi_ret_(NULL)
{
	memset(&cif_, 0, sizeof(cif_));
	value_ = os::GetFunction(module, name);
	if (value_)
		name_ = name;
}

static ffi_type *ffi_types[] = {
	&ffi_type_void,
	&ffi_type_sint8,
	&ffi_type_uchar,
	&ffi_type_sshort,
	&ffi_type_ushort,
	&ffi_type_sint,
	&ffi_type_uint,
	&ffi_type_slong,
	&ffi_type_ulong,
#if PTRDIFF_MAX == 65535
	&ffi_type_uint16,
#elif PTRDIFF_MAX == 2147483647
	&ffi_type_uint32,
#elif PTRDIFF_MAX == 9223372036854775807
	&ffi_type_uint64,
#elif defined(_WIN64)
	&ffi_type_uint64,
#elif defined(_WIN32)
	&ffi_type_uint32,
#else
	#error "ptrdiff_t size not supported"
#endif
	&ffi_type_float,
	&ffi_type_double,
	&ffi_type_pointer,
	&ffi_type_pointer,
	&ffi_type_pointer,
	&ffi_type_pointer,
	&ffi_type_pointer,
	&ffi_type_pointer,
	&ffi_type_sint64,
	&ffi_type_uint64
};

static const ffi_abi abi_types[] = 
{
	FFI_DEFAULT_ABI,
#if defined(_WIN64)
	FFI_DEFAULT_ABI,
	FFI_DEFAULT_ABI,
#else
	FFI_SYSV,
	FFI_STDCALL,
#endif
};

ffi_status FFIFunction::Prepare()
{
	ffi_ret_ = ffi_types[ret_];
	ffi_params_.clear();
	for (size_t i = 0; i < params_.size(); i++) {
		ffi_params_.push_back(ffi_types[params_.at(i)]);
	}
	return ffi_prep_cif(&cif_, abi_types[abi_], static_cast<unsigned int>(params_.size()), ffi_ret_, ffi_params_.data());
}

void FFIFunction::Call(void *ret, void **args)
{
	ffi_call(&cif_, FFI_FN(value_), ret, args);
}

/**
 * FFILibraryBinder
 */

void FFILibraryBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"tostring", &tostring},
		{"name", &tostring},
		{"address", &address},
		{"close", &close},
		{"getFunction", &get_function},
		{NULL, NULL},
	};

	static const luaL_Reg meta_methods[] = {
		{"__tostring", &tostring},
		{"__gc", &gc},
		{NULL, NULL},
	};

	register_class(state, class_name(), methods, meta_methods);

	FFIFunctionBinder::Register(state);
}

int FFILibraryBinder::tostring(lua_State *state)
{
	FFILibrary *lib = reinterpret_cast<FFILibrary *>(check_object(state, 1, class_name()));
	lua_pushstring(state, lib->name().c_str());
	return 1;
}

int FFILibraryBinder::address(lua_State *state)
{
	FFILibrary *lib = reinterpret_cast<FFILibrary *>(check_object(state, 1, class_name()));
	push_uint64(state, reinterpret_cast<uint64_t>(lib->value()));
	return 1;
}

int FFILibraryBinder::close(lua_State *state)
{
	FFILibrary *lib = reinterpret_cast<FFILibrary *>(check_object(state, 1, class_name()));
	lib->close();
	return 0;
}

int FFILibraryBinder::gc(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

static const char *type_names[] = {
	"void",
	"byte",
	"char",
	"short",
	"ushort",
	"int",
	"uint",
	"long",
	"ulong",
	"size_t",
	"float",
	"double",
	"string",
	"pointer",
	"ref char",
	"ref int",
	"ref uint",
	"ref double",
	"longlong",
	"ulonglong",
	NULL
};

enum ParamType {
	ptVoid,
	ptByte,
	ptChar,
	ptShort,
	ptUShort,
	ptInt,
	ptUInt,
	ptLong,
	ptULong,
	ptSize_t,
	ptFloat,
	ptDouble,
	ptString,
	ptPointer,
	ptRefChar,
	ptRefInt,
	ptRefUInt,
	ptRefDouble,
	ptLongLong,
	ptULongLong
};

static const char *abi_names[] = {
	"default", 
	"cdecl", 
	"stdcall", 
	NULL
};

int FFILibraryBinder::get_function(lua_State *state)
{
	FFILibrary *lib = reinterpret_cast<FFILibrary *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	FFIFunction *func = new FFIFunction(lib->value(), name);
	if (!func->value()) {
		delete func;
		func = NULL;
	} else {
		if(lua_istable(state, 3)) {
			lua_getfield(state, 3, "ret");
			func->set_ret(luaL_checkoption(state, -1, "int", type_names));
			lua_pop(state, 1);
			lua_getfield(state, 3, "abi");
			func->set_abi(luaL_checkoption(state, -1, "default", abi_names));
			lua_pop(state, 1);

			size_t param_count = lua_rawlen(state, 3);
			for (size_t i = 0; i < param_count; i++) {
				lua_rawgeti(state, 3, (int)i + 1);
				func->add_param(luaL_checkoption(state, -1, "int", type_names));
				lua_pop(state, 1);
			}
		} else {
			func->set_ret(luaL_checkoption(state, 3, "int", type_names));

			int param_count = lua_gettop(state) - 3;
			for (int i = 0; i < param_count; i++) {
				func->add_param(luaL_checkoption(state, i + 4, "int", type_names));
			}
		}
		if (func->Prepare() != FFI_OK) {
			delete func;
			return luaL_error(state, "error in libffi preparation");
		}
	}
	push_object(state, FFIFunctionBinder::class_name(), func);
	return 1;
}

/**
 * FFIFunctionBinder
 */

void FFIFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"tostring", &tostring},
		{"name", &tostring},
		{"address", &address},
		{NULL, NULL},
	};

	static const luaL_Reg meta_methods[] = {
		{"__tostring", &tostring},
		{"__call", &call},
		{"__gc", &gc},
		{NULL, NULL},
	};

	register_class(state, class_name(), methods, meta_methods);
}

int FFIFunctionBinder::tostring(lua_State *state)
{
	FFIFunction *func = reinterpret_cast<FFIFunction *>(check_object(state, 1, class_name()));
	lua_pushstring(state, func->name().c_str());
	return 1;
}

int FFIFunctionBinder::gc(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

int FFIFunctionBinder::address(lua_State *state)
{
	FFIFunction *func = reinterpret_cast<FFIFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, reinterpret_cast<uint64_t>(func->value()));
	return 1;
}

int FFIFunctionBinder::call(lua_State *state)
{
	struct FFIValue {
		union {
			size_t i;
			float f;
			double d;
			const char *s;
			void *p;
			long l;
			uint64_t ll;
		};
	};

	FFIFunction *func = reinterpret_cast<FFIFunction *>(check_object(state, 1, class_name()));
	std::vector<void *> args;
	std::vector<FFIValue> values;

	std::vector<int> params = func->params();
	if (!params.empty()) {
		for (size_t i = 0; i < params.size(); i++) {
			FFIValue value;
			int j = 2 + (int)i;
			switch (params[i]) {
			case ptByte:
			case ptChar:
				value.i = static_cast<uint8_t>(lua_tointeger(state, j));
				break;
			case ptShort:
				value.i = static_cast<short>(lua_tointeger(state, j));
				break;
			case ptUShort:
				value.i = static_cast<unsigned short>(lua_tointeger(state, j));
				break;
			case ptInt:
				value.i = static_cast<int>(lua_tointeger(state, j));
				break;
			case ptUInt:
				value.i = static_cast<unsigned int>(lua_tointeger(state, j));
				break;
			case ptLong:
				value.l = static_cast<long>(lua_tointeger(state, j));
				break;
			case ptULong:
				value.l = static_cast<unsigned long>(lua_tointeger(state, j));
				break;
			case ptSize_t:
				value.i = static_cast<size_t>(lua_tointeger(state, j));
				break;
			case ptString:
				value.s = lua_isnil(state, j) ? NULL : lua_tostring(state, j);
				break;
			case ptPointer:
				value.p = lua_isstring(state, j) ? (void*)lua_tostring(state, j) : lua_touserdata(state, j);
				break;
			default:
				return luaL_error(state, "parameter %d is of unknown type", j);
			}
			values.push_back(value);
		}
		for (size_t i = 0; i < values.size(); i++) {
			args.push_back(&values[i]);
		}
	}

	switch(func->ret()) {
	case ptVoid:
		{
			func->Call(NULL, args.data());
			lua_pushnil(state);
		}
		break;
	case ptByte:
	case ptChar:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<unsigned char>(ret));
		}
		break;
	case ptShort: 
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<short>(ret));
		}
		break;
	case ptUShort:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<unsigned short>(ret));
		}
		break;
	case ptInt:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<int>(ret));
		}
		break;
	case ptUInt:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<unsigned int>(ret));
		}
		break;
	case ptLong:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushnumber(state, static_cast<long>(ret));
		}
		break;
	case ptULong:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, static_cast<unsigned long>(ret));
		}
		break;
	case ptSize_t:
		{
			size_t ret;
			func->Call(&ret, args.data());
			lua_pushinteger(state, ret);
		}
		break;
	case ptFloat:
		{
			float ret;
			func->Call(&ret, args.data());
			lua_pushnumber(state, ret);
		}
		break;
	case ptDouble:
		{
			double ret;
			func->Call(&ret, args.data());
			lua_pushnumber(state, ret);
		}
		break;
	case ptString:
		{
			char *ret;
			func->Call(&ret, args.data());
			if (ret)
				lua_pushstring(state, ret);
			else
				lua_pushnil(state);
		}
		break;		
	case ptPointer:
		{
			void *ret;
			func->Call(&ret, args.data());
			if (ret) 
				lua_pushlightuserdata(state, ret);
			else
				lua_pushnil(state);
		}
		break;				
	default:
		return luaL_error(state, "unknown return type for function %s", func->name().c_str());
	}

	return 1;
}

/**
 * OperandTypeBinder
 */

void OperandTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", otNone},
		{"Value", otValue},
		{"Registr", otRegistr},
		{"Memory", otMemory},
		{"SegmentRegistr", otSegmentRegistr},
		{"ControlRegistr", otControlRegistr},
		{"DebugRegistr", otDebugRegistr},
		{"FPURegistr", otFPURegistr},
		{"HiPartRegistr", otHiPartRegistr},
		{"BaseRegistr", otBaseRegistr},
		{"MMXRegistr", otMMXRegistr},
		{"XMMRegistr", otXMMRegistr},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * OperandSizeBinder
 */

void OperandSizeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"Byte", osByte},
		{"Word", osWord},
		{"DWord", osDWord},
		{"QWord", osQWord},
		{"TByte", osTByte},
		{"OWord", osOWord},
		{"FWord", osFWord},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * ObjectTypeBinder
 */

void ObjectTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"Code", otCode},
		{"Data", otData},
		{"Export", otExport},
		{"Marker", otMarker},
		{"APIMarker", otAPIMarker},
		{"Import", otImport},
		{"String", otString},
		{"Unknown", otUnknown},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * CommandOptionBinder
 */

void CommandOptionBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"InverseFlag", roInverseFlag},
		{"LockPrefix", roLockPrefix},
		{"VexPrefix", roVexPrefix},
		{"Far", roFar},
		{"Breaked", roBreaked},
		{"ClearOriginalCode", roClearOriginalCode},
		{"NeedCompile", roNeedCompile},
		{"CreateNewBlock", roCreateNewBlock},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * FoldersBinder
 */

void FoldersBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"add", &SafeFunction<add>},
		{"clear", &SafeFunction<clear>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	FolderBinder::Register(state);
}

int FoldersBinder::item(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FolderBinder::class_name(), object->item(index));
	return 1;
}

int FoldersBinder::count(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FoldersBinder::add(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	std::string name = lua_tostring(state, 2);
	push_object(state, FolderBinder::class_name(), object->Add(name));
	return 1;
}

int FoldersBinder::clear(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

/**
 * FolderBinder
 */

void FolderBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"add", &SafeFunction<add>},
		{"clear", &SafeFunction<clear>},
		{"name", &SafeFunction<name>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int FolderBinder::item(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FolderBinder::class_name(), object->item(index));
	return 1;
}

int FolderBinder::count(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FolderBinder::add(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	std::string name = lua_tostring(state, 2);
	push_object(state, FolderBinder::class_name(), object->Add(name));
	return 1;
}

int FolderBinder::clear(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int FolderBinder::name(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int FolderBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * PEFileBinder
 */

void PEFileBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"format", &SafeFunction<format>},
		{"size", &SafeFunction<size>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"flush", &SafeFunction<flush>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEFormatBinder::Register(state);
	PEArchitectureBinder::Register(state);
	NETFormatBinder::Register(state);
	NETArchitectureBinder::Register(state);
}

int PEFileBinder::item(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	IArchitecture *arch = object->item(index);
	const char *class_name = NULL;
	if (dynamic_cast<PEArchitecture *>(arch)) {
		class_name = PEArchitectureBinder::class_name();
	}
	else if (dynamic_cast<NETArchitecture *>(arch)) {
		class_name = NETArchitectureBinder::class_name();
	}
	else {
		arch = NULL;
	}

	push_object(state, class_name, object->item(index));
	return 1;
}

int PEFileBinder::count(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEFileBinder::name(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	std::string name = object->file_name(true);
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEFileBinder::format(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	std::string name = object->format_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEFileBinder::size(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int PEFileBinder::seek(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int PEFileBinder::tell(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int PEFileBinder::write(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int PEFileBinder::flush(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	object->Flush();
	return 0;
}

int PEFileBinder::read(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (!buffer.empty())
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * PEArchitectureBinder
 */

void PEArchitectureBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"file", &SafeFunction<file>},
		{"entryPoint", &SafeFunction<entry_point>},
		{"imageBase", &SafeFunction<image_base>},
		{"cpuAddressSize", &SafeFunction<cpu_address_size>},
		{"dllCharacteristics", &SafeFunction<dll_characteristics>},
		{"size", &SafeFunction<size>},
		{"segments", &SafeFunction<segments>},
		{"sections", &SafeFunction<sections>},
		{"functions", &SafeFunction<functions>},
		{"directories", &SafeFunction<directories>},
		{"imports", &SafeFunction<imports>},
		{"exports", &SafeFunction<exports>},
		{"resources", &SafeFunction<resources>},
		{"fixups", &SafeFunction<fixups>},
		{"mapFunctions", &SafeFunction<map_functions>},
		{"folders", &SafeFunction<folders>},
		{"addressSeek", &SafeFunction<address_seek>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PESegmentsBinder::Register(state);
	PESectionsBinder::Register(state);
	PEDirectoriesBinder::Register(state);
	PEImportsBinder::Register(state);
	PEExportsBinder::Register(state);
	PEResourcesBinder::Register(state);
	PEFixupsBinder::Register(state);
	MapFunctionsBinder::Register(state);
}

int PEArchitectureBinder::segments(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PESegmentsBinder::class_name(), object->segment_list());
	return 1;
}

int PEArchitectureBinder::sections(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PESectionsBinder::class_name(), object->segment_list());
	return 1;
}

int PEArchitectureBinder::name(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEArchitectureBinder::file(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEFileBinder::class_name(), object->owner());
	return 1;
}

int PEArchitectureBinder::entry_point(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->entry_point());
	return 1;
}

int PEArchitectureBinder::image_base(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->image_base());
	return 1;
}

int PEArchitectureBinder::cpu_address_size(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->cpu_address_size());
	return 1;
}

int PEArchitectureBinder::dll_characteristics(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->dll_characteristics());
	return 1;
}

int PEArchitectureBinder::size(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int PEArchitectureBinder::functions(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, IntelFunctionsBinder::class_name(), object->function_list());
	return 1;
}

int PEArchitectureBinder::directories(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEDirectoriesBinder::class_name(), object->command_list());
	return 1;
}

int PEArchitectureBinder::imports(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEImportsBinder::class_name(), object->import_list());
	return 1;
}

int PEArchitectureBinder::exports(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEExportsBinder::class_name(), object->export_list());
	return 1;
}

int PEArchitectureBinder::resources(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEResourcesBinder::class_name(), object->resource_list());
	return 1;
}

int PEArchitectureBinder::fixups(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEFixupsBinder::class_name(), object->fixup_list());
	return 1;
}

int PEArchitectureBinder::map_functions(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MapFunctionsBinder::class_name(), object->map_function_list());
	return 1;
}

int PEArchitectureBinder::folders(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, FoldersBinder::class_name(), object->owner()->folder_list());
	return 1;
}

int PEArchitectureBinder::address_seek(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	lua_pushboolean(state, object->AddressSeek(address));
	return 1;
}

int PEArchitectureBinder::seek(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int PEArchitectureBinder::tell(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int PEArchitectureBinder::write(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int PEArchitectureBinder::read(lua_State *state)
{
	PEArchitecture *object = reinterpret_cast<PEArchitecture *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (!buffer.empty())
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * PESegmentsBinder
 */

void PESegmentsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	PESegmentBinder::Register(state);
}

int PESegmentsBinder::item(lua_State *state)
{
	PESegmentList *object = reinterpret_cast<PESegmentList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PESegmentBinder::class_name(), object->item(index));
	return 1;
}

int PESegmentsBinder::count(lua_State *state)
{
	PESegmentList *object = reinterpret_cast<PESegmentList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PESegmentsBinder::GetItemByAddress(lua_State *state)
{
	PESegmentList *object = reinterpret_cast<PESegmentList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, PESegmentBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

int PESegmentsBinder::GetItemByName(lua_State *state)
{
	PESegmentList *object = reinterpret_cast<PESegmentList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, PESegmentBinder::class_name(), object->GetSectionByName(name));
	return 1;
}

/**
 * PESegmentBinder
 */

void PESegmentBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"setName", &SafeFunction<set_name>},
		{"size", &SafeFunction<size>},
		{"physicalOffset", &SafeFunction<physical_offset>},
		{"physicalSize", &SafeFunction<physical_size>},
		{"flags", &SafeFunction<flags>},
		{"excludedFromPacking", &SafeFunction<excluded_from_packing>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int PESegmentBinder::size(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int PESegmentBinder::address(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PESegmentBinder::name(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PESegmentBinder::set_name(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

int PESegmentBinder::physical_offset(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int PESegmentBinder::physical_size(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_size());
	return 1;
}

int PESegmentBinder::flags(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int PESegmentBinder::excluded_from_packing(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->excluded_from_packing());
	return 1;
}

int PESegmentBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * PESectionsBinder
 */

void PESectionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	PESectionBinder::Register(state);
}

int PESectionsBinder::item(lua_State *state)
{
	PESectionList *object = reinterpret_cast<PESectionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PESectionBinder::class_name(), object->item(index));
	return 1;
}

int PESectionsBinder::count(lua_State *state)
{
	PESectionList *object = reinterpret_cast<PESectionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PESectionsBinder::GetItemByAddress(lua_State *state)
{
	PESectionList *object = reinterpret_cast<PESectionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, PESectionBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

/**
 * PESectionBinder
 */

void PESectionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"offset", &SafeFunction<offset>},
		{"segment", &SafeFunction<segment>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int PESectionBinder::size(lua_State *state)
{
	PESection *object = reinterpret_cast<PESection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int PESectionBinder::address(lua_State *state)
{
	PESection *object = reinterpret_cast<PESection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PESectionBinder::name(lua_State *state)
{
	PESection *object = reinterpret_cast<PESection *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PESectionBinder::offset(lua_State *state)
{
	PESection *object = reinterpret_cast<PESection *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int PESectionBinder::segment(lua_State *state)
{
	PESection *object = reinterpret_cast<PESection *>(check_object(state, 1, class_name()));
	push_object(state, PESegmentBinder::class_name(), object->parent());
	return 1;
}

/**
 * PEDirectoriesBinder
 */

void PEDirectoriesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByType", &SafeFunction<GetItemByType>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEDirectoryBinder::Register(state);
}

int PEDirectoriesBinder::item(lua_State *state)
{
	PEDirectoryList *object = reinterpret_cast<PEDirectoryList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEDirectoryBinder::class_name(), object->item(index));
	return 1;
}

int PEDirectoriesBinder::count(lua_State *state)
{
	PEDirectoryList *object = reinterpret_cast<PEDirectoryList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEDirectoriesBinder::GetItemByType(lua_State *state)
{
	PEDirectoryList *object = reinterpret_cast<PEDirectoryList *>(check_object(state, 1, class_name()));
	uint32_t type = static_cast<uint32_t>(check_integer(state, 2));
	push_object(state, PEDirectoryBinder::class_name(), object->GetCommandByType(type));
	return 1;
}

/**
 * PEDirectoryBinder
 */

void PEDirectoryBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"type", &SafeFunction<type>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"setAddress", &SafeFunction<set_address>},
		{"setSize", &SafeFunction<set_size>},
		{"clear", &SafeFunction<clear>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int PEDirectoryBinder::size(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int PEDirectoryBinder::address(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PEDirectoryBinder::name(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEDirectoryBinder::type(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int PEDirectoryBinder::set_size(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	uint32_t size = static_cast<uint32_t>(check_integer(state, 2));
	object->set_size(size);
	return 0;
}

int PEDirectoryBinder::set_address(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	object->set_address(address);
	return 0;
}

int PEDirectoryBinder::clear(lua_State *state)
{
	PEDirectory *object = reinterpret_cast<PEDirectory *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

/**
 * PEFormatBinder
 */

void PEFormatBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		// Directory Entries
		{"IMAGE_DIRECTORY_ENTRY_EXPORT", IMAGE_DIRECTORY_ENTRY_EXPORT},
		{"IMAGE_DIRECTORY_ENTRY_IMPORT", IMAGE_DIRECTORY_ENTRY_IMPORT},
		{"IMAGE_DIRECTORY_ENTRY_RESOURCE", IMAGE_DIRECTORY_ENTRY_RESOURCE},
		{"IMAGE_DIRECTORY_ENTRY_EXCEPTION", IMAGE_DIRECTORY_ENTRY_EXCEPTION},
		{"IMAGE_DIRECTORY_ENTRY_SECURITY", IMAGE_DIRECTORY_ENTRY_SECURITY},
		{"IMAGE_DIRECTORY_ENTRY_BASERELOC", IMAGE_DIRECTORY_ENTRY_BASERELOC},
		{"IMAGE_DIRECTORY_ENTRY_DEBUG", IMAGE_DIRECTORY_ENTRY_DEBUG},
		{"IMAGE_DIRECTORY_ENTRY_ARCHITECTURE", IMAGE_DIRECTORY_ENTRY_ARCHITECTURE},
		{"IMAGE_DIRECTORY_ENTRY_TLS", IMAGE_DIRECTORY_ENTRY_TLS},
		{"IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG", IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG},
		{"IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT", IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT},
		{"IMAGE_DIRECTORY_ENTRY_IAT", IMAGE_DIRECTORY_ENTRY_IAT},
		{"IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT", IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT},
		{"IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR", IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR},
		// Section characteristics
		{"IMAGE_SCN_CNT_CODE", IMAGE_SCN_CNT_CODE},
		{"IMAGE_SCN_CNT_INITIALIZED_DATA", IMAGE_SCN_CNT_INITIALIZED_DATA},
		{"IMAGE_SCN_CNT_UNINITIALIZED_DATA", IMAGE_SCN_CNT_UNINITIALIZED_DATA},
		{"IMAGE_SCN_MEM_DISCARDABLE", IMAGE_SCN_MEM_DISCARDABLE},
		{"IMAGE_SCN_MEM_NOT_CACHED", IMAGE_SCN_MEM_NOT_CACHED},
		{"IMAGE_SCN_MEM_NOT_PAGED", IMAGE_SCN_MEM_NOT_PAGED},
		{"IMAGE_SCN_MEM_SHARED", IMAGE_SCN_MEM_SHARED},
		{"IMAGE_SCN_MEM_EXECUTE", IMAGE_SCN_MEM_EXECUTE},
		{"IMAGE_SCN_MEM_READ", IMAGE_SCN_MEM_READ},
		{"IMAGE_SCN_MEM_WRITE", IMAGE_SCN_MEM_WRITE},
		// DllCharacteristics
		{"IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE", IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE},
		{"IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY", IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY},
		{"IMAGE_DLLCHARACTERISTICS_NX_COMPAT", IMAGE_DLLCHARACTERISTICS_NX_COMPAT},
		{"IMAGE_DLLCHARACTERISTICS_NO_ISOLATION", IMAGE_DLLCHARACTERISTICS_NO_ISOLATION},
		{"IMAGE_DLLCHARACTERISTICS_NO_SEH", IMAGE_DLLCHARACTERISTICS_NO_SEH},
		{"IMAGE_DLLCHARACTERISTICS_NO_BIND", IMAGE_DLLCHARACTERISTICS_NO_BIND},
		{"IMAGE_DLLCHARACTERISTICS_WDM_DRIVER", IMAGE_DLLCHARACTERISTICS_WDM_DRIVER},
		{"IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE", IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE},
		// Resource types
		{"RT_CURSOR", rtCursor},
		{"RT_BITMAP", rtBitmap},
		{"RT_ICON", rtIcon},
		{"RT_MENU", rtMenu},
		{"RT_DIALOG", rtDialog},
		{"RT_STRING", rtStringTable},
		{"RT_FONTDIR", rtFontDir},
		{"RT_FONT", rtFont},
		{"RT_ACCELERATOR", rtAccelerators},
		{"RT_RCDATA", rtRCData},
		{"RT_MESSAGETABLE", rtMessageTable},
		{"RT_GROUP_CURSOR", rtGroupCursor},
		{"RT_GROUP_ICON", rtGroupIcon},
		{"RT_VERSION", rtVersionInfo},
		{"RT_DLGINCLUDE", rtDlgInclude},
		{"RT_PLUGPLAY", rtPlugPlay},
		{"RT_VXD", rtVXD},
		{"RT_ANICURSOR", rtAniCursor},
		{"RT_ANIICON", rtAniIcon},
		{"RT_HTML", rtHTML},
		{"RT_MANIFEST", rtManifest},
		{"RT_DLGINIT", rtDialogInit},
		{"RT_TOOLBAR", rtToolbar},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * PEImportsBinder
 */

void PEImportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEImportBinder::Register(state);
}

int PEImportsBinder::item(lua_State *state)
{
	PEImportList *object = reinterpret_cast<PEImportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEImportBinder::class_name(), object->item(index));
	return 1;
}

int PEImportsBinder::count(lua_State *state)
{
	PEImportList *object = reinterpret_cast<PEImportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEImportsBinder::GetItemByName(lua_State *state)
{
	PEImportList *object = reinterpret_cast<PEImportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, PEImportBinder::class_name(), object->GetImportByName(name));
	return 1;
}

/**
 * PEImportBinder
 */

void PEImportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"isSDK", &SafeFunction<is_sdk>},
		{"setName", &SafeFunction<set_name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEImportFunctionBinder::Register(state);
}

int PEImportBinder::item(lua_State *state)
{
	PEImport *object = reinterpret_cast<PEImport *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEImportFunctionBinder::class_name(), object->item(index));
	return 1;
}

int PEImportBinder::count(lua_State *state)
{
	PEImport *object = reinterpret_cast<PEImport *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEImportBinder::name(lua_State *state)
{
	PEImport *object = reinterpret_cast<PEImport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEImportBinder::is_sdk(lua_State *state)
{
	PEImport *object = reinterpret_cast<PEImport *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_sdk());
	return 1;
}

int PEImportBinder::set_name(lua_State *state)
{
	PEImport *object = reinterpret_cast<PEImport *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
 * PEImportFunctionBinder
 */

void PEImportFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	APITypeBinder::Register(state);
}

int PEImportFunctionBinder::address(lua_State *state)
{
	PEImportFunction *object = reinterpret_cast<PEImportFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PEImportFunctionBinder::name(lua_State *state)
{
	PEImportFunction *object = reinterpret_cast<PEImportFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEImportFunctionBinder::type(lua_State *state)
{
	PEImportFunction *object = reinterpret_cast<PEImportFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * APITypeBinder
 */

void APITypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", atNone},
		{"Begin", atBegin},
		{"End", atEnd},
		{"IsVirtualMachinePresent", atIsVirtualMachinePresent},
		{"IsDebuggerPresent", atIsDebuggerPresent},
		{"IsValidImageCRC", atIsValidImageCRC},
		{"DecryptStringA", atDecryptStringA},
		{"DecryptStringW", atDecryptStringW},
		{"FreeString", atFreeString},
		{"ActivateLicense", atActivateLicense},
		{"DeactivateLicense", atDeactivateLicense},
		{"GetOfflineActivationString", atGetOfflineActivationString},
		{"GetOfflineDeactivationString", atGetOfflineDeactivationString},
		{"SetSerialNumber", atSetSerialNumber},
		{"GetSerialNumberState", atGetSerialNumberState},
		{"GetSerialNumberData", atGetSerialNumberData},
		{"GetCurrentHWID", atGetCurrentHWID},
		{"LoadResource", atLoadResource},
		{"FindResourceA", atFindResourceA},
		{"FindResourceExA", atFindResourceExA},
		{"FindResourceW", atFindResourceW},
		{"FindResourceExW", atFindResourceExW},
		{"LoadStringA", atLoadStringA},
		{"LoadStringW", atLoadStringW},
		{"EnumResourceNamesA", atEnumResourceNamesA},
		{"EnumResourceNamesW", atEnumResourceNamesW},
		{"EnumResourceLanguagesA", atEnumResourceLanguagesA},
		{"EnumResourceLanguagesW", atEnumResourceLanguagesW},
		{"EnumResourceTypesA", atEnumResourceTypesA},
		{"EnumResourceTypesW", atEnumResourceTypesW},
		{"DecryptBuffer", atDecryptBuffer},
		{"RuntimeInit", atRuntimeInit},
		{"LoaderData", atLoaderData},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * PEExportsBinder
 */

void PEExportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"setName", &SafeFunction<set_name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEExportBinder::Register(state);
}

int PEExportsBinder::item(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEExportBinder::class_name(), object->item(index));
	return 1;
}

int PEExportsBinder::count(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEExportsBinder::clear(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int PEExportsBinder::Delete(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int PEExportsBinder::name(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEExportsBinder::set_name(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

int PEExportsBinder::GetItemByAddress(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, PEExportBinder::class_name(), object->GetExportByAddress(address));
	return 1;
}

int PEExportsBinder::GetItemByName(lua_State *state)
{
	PEExportList *object = reinterpret_cast<PEExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, PEExportBinder::class_name(), object->GetExportByName(name));
	return 1;
}

/**
 * PEExportBinder
 */

void PEExportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"setName", &SafeFunction<set_name>},
		{"ordinal", &SafeFunction<ordinal>},
		{"forwardedName", &SafeFunction<forwarded_name>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int PEExportBinder::address(lua_State *state)
{
	PEExport *object = reinterpret_cast<PEExport *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PEExportBinder::name(lua_State *state)
{
	PEExport *object = reinterpret_cast<PEExport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEExportBinder::set_name(lua_State *state)
{
	PEExport *object = reinterpret_cast<PEExport *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

int PEExportBinder::ordinal(lua_State *state)
{
	PEExport *object = reinterpret_cast<PEExport *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->ordinal());
	return 1;
}

int PEExportBinder::forwarded_name(lua_State *state)
{
	PEExport *object = reinterpret_cast<PEExport *>(check_object(state, 1, class_name()));
	std::string name = object->forwarded_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEExportBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * PEResourcesBinder
 */

void PEResourcesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{"itemByType", &SafeFunction<GetItemByType>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	PEResourceBinder::Register(state);
}

int PEResourcesBinder::item(lua_State *state)
{
	PEResourceList *object = reinterpret_cast<PEResourceList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEResourceBinder::class_name(), object->item(index));
	return 1;
}

int PEResourcesBinder::count(lua_State *state)
{
	PEResourceList *object = reinterpret_cast<PEResourceList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEResourcesBinder::clear(lua_State *state)
{
	PEResourceList *object = reinterpret_cast<PEResourceList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int PEResourcesBinder::GetItemByName(lua_State *state)
{
	PEResourceList *object = reinterpret_cast<PEResourceList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, PEResourceBinder::class_name(), object->GetResourceByName(name));
	return 1;
}

int PEResourcesBinder::GetItemByType(lua_State *state)
{
	PEResourceList *object = reinterpret_cast<PEResourceList *>(check_object(state, 1, class_name()));
	uint32_t type = static_cast<uint32_t>(check_integer(state, 2));
	push_object(state, PEResourceBinder::class_name(), object->GetResourceByType(type));
	return 1;
}

/**
 * PEResourceBinder
 */

void PEResourceBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"address", &SafeFunction<address>},
		{"size", &SafeFunction<size>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{"isDirectory", &SafeFunction<is_directory>},
		{"destroy", &SafeFunction<destroy>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{"excludedFromPacking", &SafeFunction<excluded_from_packing>},
		{"setExcludedFromPacking", &SafeFunction<set_excluded_from_packing>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int PEResourceBinder::item(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, class_name(), object->item(index));
	return 1;
}

int PEResourceBinder::count(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEResourceBinder::clear(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int PEResourceBinder::address(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PEResourceBinder::size(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int PEResourceBinder::name(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int PEResourceBinder::type(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int PEResourceBinder::is_directory(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_directory());
	return 1;
}

int PEResourceBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

int PEResourceBinder::GetItemByName(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, PEResourceBinder::class_name(), object->GetResourceByName(name));
	return 1;
}

int PEResourceBinder::excluded_from_packing(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->excluded_from_packing());
	return 1;
}

int PEResourceBinder::set_excluded_from_packing(lua_State *state)
{
	PEResource *object = reinterpret_cast<PEResource *>(check_object(state, 1, class_name()));
	bool excluded_from_packing = check_integer(state, 2) == 1;
	object->set_excluded_from_packing(excluded_from_packing);
	return 0;
}

/**
 * PEFixupsBinder
 */

void PEFixupsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"indexOf", &SafeFunction<IndexOf>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	PEFixupBinder::Register(state);
}

int PEFixupsBinder::item(lua_State *state)
{
	PEFixupList *object = reinterpret_cast<PEFixupList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, PEFixupBinder::class_name(), object->item(index));
	return 1;
}

int PEFixupsBinder::count(lua_State *state)
{
	PEFixupList *object = reinterpret_cast<PEFixupList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int PEFixupsBinder::IndexOf(lua_State *state)
{
	PEFixupList *object = reinterpret_cast<PEFixupList *>(check_object(state, 1, class_name()));
	PEFixup *item = reinterpret_cast<PEFixup *>(check_object(state, 2, PEFixupBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

int PEFixupsBinder::GetItemByAddress(lua_State *state)
{
	PEFixupList *object = reinterpret_cast<PEFixupList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, PEFixupBinder::class_name(), object->GetFixupByAddress(address));
	return 1;
}

/**
 * PEFixupBinder
 */

void PEFixupBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"setDeleted", &SafeFunction<set_deleted>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);
}

int PEFixupBinder::address(lua_State *state)
{
	PEFixup *object = reinterpret_cast<PEFixup *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int PEFixupBinder::set_deleted(lua_State *state)
{
	PEFixup *object = reinterpret_cast<PEFixup *>(check_object(state, 1, class_name()));
	bool value = lua_toboolean(state, 2) == 1;
	object->set_deleted(value);
	return 0;
}

/**
 * MacFileBinder
 */

void MacFileBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"format", &SafeFunction<format>},
		{"size", &SafeFunction<size>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"flush", &SafeFunction<flush>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacFormatBinder::Register(state);
	MacArchitectureBinder::Register(state);
}

int MacFileBinder::item(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacArchitectureBinder::class_name(), object->item(index));
	return 1;
}

int MacFileBinder::count(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacFileBinder::name(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	std::string name = object->file_name(true);
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacFileBinder::format(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	std::string name = object->format_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacFileBinder::size(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int MacFileBinder::seek(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int MacFileBinder::tell(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int MacFileBinder::write(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int MacFileBinder::flush(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	object->Flush();
	return 0;
}

int MacFileBinder::read(lua_State *state)
{
	MacFile *object = reinterpret_cast<MacFile *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (!buffer.empty())
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * MacArchitectureBinder
 */

void MacArchitectureBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"file", &SafeFunction<file>},
		{"entryPoint", &SafeFunction<entry_point>},
		{"imageBase", &SafeFunction<image_base>},
		{"cpuAddressSize", &SafeFunction<cpu_address_size>},
		{"size", &SafeFunction<size>},
		{"segments", &SafeFunction<segments>},
		{"sections", &SafeFunction<sections>},
		{"functions", &SafeFunction<functions>},
		{"commands", &SafeFunction<commands>},
		{"symbols", &SafeFunction<symbols>},
		{"imports", &SafeFunction<imports>},
		{"exports", &SafeFunction<exports>},
		{"fixups", &SafeFunction<fixups>},
		{"mapFunctions", &SafeFunction<map_functions>},
		{"addressSeek", &SafeFunction<address_seek>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacSegmentsBinder::Register(state);
	MacSectionsBinder::Register(state);
	MacSymbolsBinder::Register(state);
	MacCommandsBinder::Register(state);
	MacImportsBinder::Register(state);
	MacExportsBinder::Register(state);
	MacFixupsBinder::Register(state);
}

int MacArchitectureBinder::entry_point(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->entry_point());
	return 1;
}

int MacArchitectureBinder::image_base(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->image_base());
	return 1;
}

int MacArchitectureBinder::cpu_address_size(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->cpu_address_size());
	return 1;
}

int MacArchitectureBinder::size(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int MacArchitectureBinder::name(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacArchitectureBinder::file(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacFileBinder::class_name(), object->owner());
	return 1;
}

int MacArchitectureBinder::segments(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacSegmentsBinder::class_name(), object->segment_list());
	return 1;
}

int MacArchitectureBinder::sections(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacSectionsBinder::class_name(), object->section_list());
	return 1;
}

int MacArchitectureBinder::functions(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, IntelFunctionsBinder::class_name(), object->function_list());
	return 1;
}

int MacArchitectureBinder::commands(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacCommandsBinder::class_name(), object->command_list());
	return 1;
}

int MacArchitectureBinder::symbols(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacSymbolsBinder::class_name(), object->symbol_list());
	return 1;
}

int MacArchitectureBinder::imports(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacImportsBinder::class_name(), object->import_list());
	return 1;
}

int MacArchitectureBinder::exports(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacExportsBinder::class_name(), object->export_list());
	return 1;
}

int MacArchitectureBinder::fixups(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MacFixupsBinder::class_name(), object->fixup_list());
	return 1;
}

int MacArchitectureBinder::map_functions(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MapFunctionsBinder::class_name(), object->map_function_list());
	return 1;
}

int MacArchitectureBinder::folders(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, FoldersBinder::class_name(), object->owner()->folder_list());
	return 1;
}

int MacArchitectureBinder::address_seek(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	lua_pushboolean(state, object->AddressSeek(address));
	return 1;
}

int MacArchitectureBinder::seek(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int MacArchitectureBinder::tell(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int MacArchitectureBinder::write(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int MacArchitectureBinder::read(lua_State *state)
{
	MacArchitecture *object = reinterpret_cast<MacArchitecture *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (!buffer.empty())
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * MacSegmentsBinder
 */

void MacSegmentsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	MacSegmentBinder::Register(state);
}

int MacSegmentsBinder::item(lua_State *state)
{
	MacSegmentList *object = reinterpret_cast<MacSegmentList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacSegmentBinder::class_name(), object->item(index));
	return 1;
}

int MacSegmentsBinder::count(lua_State *state)
{
	MacSegmentList *object = reinterpret_cast<MacSegmentList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacSegmentsBinder::GetItemByAddress(lua_State *state)
{
	MacSegmentList *object = reinterpret_cast<MacSegmentList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, MacSegmentBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

/**
 * MacSegmentBinder
 */

void MacSegmentBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"physicalOffset", &SafeFunction<physical_offset>},
		{"physicalSize", &SafeFunction<physical_size>},
		{"flags", &SafeFunction<flags>},
		{"excludedFromPacking", &SafeFunction<excluded_from_packing>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacSegmentBinder::size(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int MacSegmentBinder::address(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacSegmentBinder::name(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacSegmentBinder::physical_offset(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int MacSegmentBinder::physical_size(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_size());
	return 1;
}

int MacSegmentBinder::flags(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int MacSegmentBinder::excluded_from_packing(lua_State *state)
{
	MacSegment *object = reinterpret_cast<MacSegment *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->excluded_from_packing());
	return 1;
}

/**
 * MacSectionsBinder
 */

void MacSectionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	MacSectionBinder::Register(state);
}

int MacSectionsBinder::item(lua_State *state)
{
	MacSectionList *object = reinterpret_cast<MacSectionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacSectionBinder::class_name(), object->item(index));
	return 1;
}

int MacSectionsBinder::count(lua_State *state)
{
	MacSectionList *object = reinterpret_cast<MacSectionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacSectionsBinder::GetItemByAddress(lua_State *state)
{
	MacSectionList *object = reinterpret_cast<MacSectionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, MacSectionBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

/**
 * MacSectionBinder
 */

void MacSectionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"offset", &SafeFunction<offset>},
		{"segment", &SafeFunction<segment>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacSectionBinder::size(lua_State *state)
{
	MacSection *object = reinterpret_cast<MacSection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int MacSectionBinder::address(lua_State *state)
{
	MacSection *object = reinterpret_cast<MacSection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacSectionBinder::name(lua_State *state)
{
	MacSection *object = reinterpret_cast<MacSection *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacSectionBinder::offset(lua_State *state)
{
	MacSection *object = reinterpret_cast<MacSection *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int MacSectionBinder::segment(lua_State *state)
{
	MacSection *object = reinterpret_cast<MacSection *>(check_object(state, 1, class_name()));
	push_object(state, MacSegmentBinder::class_name(), object->parent());
	return 1;
}

int MacSectionBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * MacCommandsBinder
 */

void MacCommandsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByType", &SafeFunction<GetItemByType>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacCommandBinder::Register(state);
}

int MacCommandsBinder::item(lua_State *state)
{
	MacLoadCommandList *object = reinterpret_cast<MacLoadCommandList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacCommandBinder::class_name(), object->item(index));
	return 1;
}

int MacCommandsBinder::count(lua_State *state)
{
	MacLoadCommandList *object = reinterpret_cast<MacLoadCommandList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacCommandsBinder::GetItemByType(lua_State *state)
{
	MacLoadCommandList *object = reinterpret_cast<MacLoadCommandList *>(check_object(state, 1, class_name()));
	uint32_t type = static_cast<uint32_t>(check_integer(state, 2));
	push_object(state, MacCommandBinder::class_name(), object->GetCommandByType(type));
	return 1;
}

/**
 * MacCommandBinder
 */

void MacCommandBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"type", &SafeFunction<type>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacCommandBinder::size(lua_State *state)
{
	MacLoadCommand *object = reinterpret_cast<MacLoadCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int MacCommandBinder::address(lua_State *state)
{
	MacLoadCommand *object = reinterpret_cast<MacLoadCommand *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacCommandBinder::name(lua_State *state)
{
	MacLoadCommand *object = reinterpret_cast<MacLoadCommand *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacCommandBinder::type(lua_State *state)
{
	MacLoadCommand *object = reinterpret_cast<MacLoadCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * MacFormatBinder
 */

void MacFormatBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		// Load Command types
		{"LC_SEGMENT", LC_SEGMENT},
		{"LC_SYMTAB", LC_SYMTAB},
		{"LC_SYMSEG", LC_SYMSEG},
		{"LC_THREAD", LC_THREAD},
		{"LC_UNIXTHREAD", LC_UNIXTHREAD},
		{"LC_LOADFVMLIB", LC_LOADFVMLIB},
		{"LC_IDFVMLIB", LC_IDFVMLIB},
		{"LC_IDENT", LC_IDENT},
		{"LC_FVMFILE", LC_FVMFILE},
		{"LC_PREPAGE", LC_PREPAGE},
		{"LC_DYSYMTAB", LC_DYSYMTAB},
		{"LC_LOAD_DYLIB", LC_LOAD_DYLIB},
		{"LC_ID_DYLIB", LC_ID_DYLIB},
		{"LC_LOAD_DYLINKER", LC_LOAD_DYLINKER},
		{"LC_PREBOUND_DYLIB", LC_PREBOUND_DYLIB},
		{"LC_ROUTINES", LC_ROUTINES},
		{"LC_SUB_FRAMEWORK", LC_SUB_FRAMEWORK},
		{"LC_SUB_UMBRELLA", LC_SUB_UMBRELLA},
		{"LC_SUB_CLIENT", LC_SUB_CLIENT},
		{"LC_SUB_LIBRARY", LC_SUB_LIBRARY},
		{"LC_TWOLEVEL_HINTS", LC_TWOLEVEL_HINTS},
		{"LC_PREBIND_CKSUM", LC_PREBIND_CKSUM},
		{"LC_LOAD_WEAK_DYLIB", LC_LOAD_WEAK_DYLIB},
		{"LC_SEGMENT_64", LC_SEGMENT_64},
		{"LC_ROUTINES_64", LC_ROUTINES_64},
		{"LC_UUID", LC_UUID},
		{"LC_RPATH", LC_RPATH},
		{"LC_CODE_SIGNATURE", LC_CODE_SIGNATURE},
		{"LC_SEGMENT_SPLIT_INFO", LC_SEGMENT_SPLIT_INFO},
		{"LC_REEXPORT_DYLIB", LC_REEXPORT_DYLIB},
		{"LC_LAZY_LOAD_DYLIB", LC_LAZY_LOAD_DYLIB},
		{"LC_ENCRYPTION_INFO", LC_ENCRYPTION_INFO},
		{"LC_DYLD_INFO", LC_DYLD_INFO},
		{"LC_DYLD_INFO_ONLY", LC_DYLD_INFO_ONLY},
		{"LC_LOAD_UPWARD_DYLIB", LC_LOAD_UPWARD_DYLIB},
		{"LC_VERSION_MIN_MACOSX", LC_VERSION_MIN_MACOSX},
		{"LC_FUNCTION_STARTS", LC_FUNCTION_STARTS},
		{"LC_DYLD_ENVIRONMENT", LC_DYLD_ENVIRONMENT},
		{"LC_MAIN", LC_MAIN},
		{"LC_DATA_IN_CODE", LC_DATA_IN_CODE},
		{"LC_SOURCE_VERSION", LC_SOURCE_VERSION},
		{"LC_DYLIB_CODE_SIGN_DRS", LC_DYLIB_CODE_SIGN_DRS},
		{"LC_ENCRYPTION_INFO_64", LC_ENCRYPTION_INFO_64},
		{"LC_LINKER_OPTION", LC_LINKER_OPTION},
		{"LC_LINKER_OPTIMIZATION_HINT", LC_LINKER_OPTIMIZATION_HINT},
		{"LC_VERSION_MIN_TVOS", LC_VERSION_MIN_TVOS},
		{"LC_VERSION_MIN_WATCHOS", LC_VERSION_MIN_WATCHOS},
		{"LC_NOTE", LC_NOTE},
		{"LC_BUILD_VERSION", LC_BUILD_VERSION},
		// Section types
		{"SECTION_TYPE", SECTION_TYPE},
		{"SECTION_ATTRIBUTES", SECTION_ATTRIBUTES},
		{"S_REGULAR", S_REGULAR},
		{"S_ZEROFILL", S_ZEROFILL},
		{"S_CSTRING_LITERALS", S_CSTRING_LITERALS},
		{"S_4BYTE_LITERALS", S_4BYTE_LITERALS},
		{"S_8BYTE_LITERALS", S_8BYTE_LITERALS},
		{"S_LITERAL_POINTERS", S_LITERAL_POINTERS},
		{"S_NON_LAZY_SYMBOL_POINTERS", S_NON_LAZY_SYMBOL_POINTERS},
		{"S_LAZY_SYMBOL_POINTERS", S_LAZY_SYMBOL_POINTERS},
		{"S_SYMBOL_STUBS", S_SYMBOL_STUBS},
		{"S_MOD_INIT_FUNC_POINTERS", S_MOD_INIT_FUNC_POINTERS},
		{"S_MOD_TERM_FUNC_POINTERS", S_MOD_TERM_FUNC_POINTERS},
		{"S_COALESCED", S_COALESCED},
		{"S_GB_ZEROFILL", S_GB_ZEROFILL},
		{"S_INTERPOSING", S_INTERPOSING},
		{"S_16BYTE_LITERALS", S_16BYTE_LITERALS},
		{"S_DTRACE_DOF", S_DTRACE_DOF},
		{"S_LAZY_DYLIB_SYMBOL_POINTERS", S_LAZY_DYLIB_SYMBOL_POINTERS},
		{"SECTION_ATTRIBUTES_USR", SECTION_ATTRIBUTES_USR},
		{"S_ATTR_PURE_INSTRUCTIONS", S_ATTR_PURE_INSTRUCTIONS},
		{"S_ATTR_NO_TOC", S_ATTR_NO_TOC},
		{"S_ATTR_STRIP_STATIC_SYMS", S_ATTR_STRIP_STATIC_SYMS},
		{"S_ATTR_NO_DEAD_STRIP", S_ATTR_NO_DEAD_STRIP},
		{"S_ATTR_LIVE_SUPPORT", S_ATTR_LIVE_SUPPORT},
		{"S_ATTR_SELF_MODIFYING_CODE", S_ATTR_SELF_MODIFYING_CODE},
		{"S_ATTR_DEBUG", S_ATTR_DEBUG},
		{"SECTION_ATTRIBUTES_SYS", SECTION_ATTRIBUTES_SYS},
		{"S_ATTR_SOME_INSTRUCTIONS", S_ATTR_SOME_INSTRUCTIONS},
		{"S_ATTR_EXT_RELOC", S_ATTR_EXT_RELOC},
		{"S_ATTR_LOC_RELOC", S_ATTR_LOC_RELOC},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * MacSymbolsBinder
 */

void MacSymbolsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacSymbolBinder::Register(state);
}

int MacSymbolsBinder::item(lua_State *state)
{
	MacSymbolList *object = reinterpret_cast<MacSymbolList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacSymbolBinder::class_name(), object->item(index));
	return 1;
}

int MacSymbolsBinder::count(lua_State *state)
{
	MacSymbolList *object = reinterpret_cast<MacSymbolList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

/**
 * MacSymbolBinder
 */

void MacSymbolBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"value", &SafeFunction<value>},
		{"name", &SafeFunction<name>},
		{"sect", &SafeFunction<sect>},
		{"desc", &SafeFunction<desc>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacSymbolBinder::value(lua_State *state)
{
	MacSymbol *object = reinterpret_cast<MacSymbol *>(check_object(state, 1, class_name()));
	push_uint64(state, object->value());
	return 1;
}

int MacSymbolBinder::name(lua_State *state)
{
	MacSymbol *object = reinterpret_cast<MacSymbol *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacSymbolBinder::sect(lua_State *state)
{
	MacSymbol *object = reinterpret_cast<MacSymbol *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->sect());
	return 1;
}

int MacSymbolBinder::desc(lua_State *state)
{
	MacSymbol *object = reinterpret_cast<MacSymbol *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->desc());
	return 1;
}

/**
 * MacImportsBinder
 */

void MacImportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacImportBinder::Register(state);
}

int MacImportsBinder::item(lua_State *state)
{
	MacImportList *object = reinterpret_cast<MacImportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacImportBinder::class_name(), object->item(index));
	return 1;
}

int MacImportsBinder::count(lua_State *state)
{
	MacImportList *object = reinterpret_cast<MacImportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacImportsBinder::GetItemByName(lua_State *state)
{
	MacImportList *object = reinterpret_cast<MacImportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, MacImportBinder::class_name(), object->GetImportByName(name));
	return 1;
}

/**
 * MacImportBinder
 */

void MacImportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"isSDK", &SafeFunction<is_sdk>},
		{"setName", &SafeFunction<set_name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacImportFunctionBinder::Register(state);
}

int MacImportBinder::item(lua_State *state)
{
	MacImport *object = reinterpret_cast<MacImport *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacImportFunctionBinder::class_name(), object->item(index));
	return 1;
}

int MacImportBinder::count(lua_State *state)
{
	MacImport *object = reinterpret_cast<MacImport *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacImportBinder::name(lua_State *state)
{
	MacImport *object = reinterpret_cast<MacImport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacImportBinder::is_sdk(lua_State *state)
{
	MacImport *object = reinterpret_cast<MacImport *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_sdk());
	return 1;
}

int MacImportBinder::set_name(lua_State *state)
{
	MacImport *object = reinterpret_cast<MacImport *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
 * MacImportFunctionBinder
 */

void MacImportFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacImportFunctionBinder::address(lua_State *state)
{
	MacImportFunction *object = reinterpret_cast<MacImportFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacImportFunctionBinder::name(lua_State *state)
{
	MacImportFunction *object = reinterpret_cast<MacImportFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacImportFunctionBinder::type(lua_State *state)
{
	MacImportFunction *object = reinterpret_cast<MacImportFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * MacExportsBinder
 */

void MacExportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"setName", &SafeFunction<set_name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MacExportBinder::Register(state);
}

int MacExportsBinder::item(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacExportBinder::class_name(), object->item(index));
	return 1;
}

int MacExportsBinder::count(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacExportsBinder::clear(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int MacExportsBinder::Delete(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int MacExportsBinder::name(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacExportsBinder::set_name(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

int MacExportsBinder::GetItemByAddress(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, MacExportBinder::class_name(), object->GetExportByAddress(address));
	return 1;
}

int MacExportsBinder::GetItemByName(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, MacExportBinder::class_name(), object->GetExportByName(name));
	return 1;
}

/**
 * MacExportBinder
 */

void MacExportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"forwardedName", &SafeFunction<forwarded_name>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int MacExportBinder::address(lua_State *state)
{
	MacExport *object = reinterpret_cast<MacExport *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacExportBinder::name(lua_State *state)
{
	MacExport *object = reinterpret_cast<MacExport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacExportBinder::forwarded_name(lua_State *state)
{
	MacExport *object = reinterpret_cast<MacExport *>(check_object(state, 1, class_name()));
	std::string name = object->forwarded_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MacExportBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * MacFixupsBinder
 */

void MacFixupsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"indexOf", &SafeFunction<IndexOf>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	MacFixupBinder::Register(state);
}

int MacFixupsBinder::item(lua_State *state)
{
	MacFixupList *object = reinterpret_cast<MacFixupList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacFixupBinder::class_name(), object->item(index));
	return 1;
}

int MacFixupsBinder::count(lua_State *state)
{
	MacFixupList *object = reinterpret_cast<MacFixupList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MacFixupsBinder::IndexOf(lua_State *state)
{
	MacFixupList *object = reinterpret_cast<MacFixupList *>(check_object(state, 1, class_name()));
	MacFixup *item = reinterpret_cast<MacFixup *>(check_object(state, 2, MacFixupBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

int MacFixupsBinder::GetItemByAddress(lua_State *state)
{
	MacFixupList *object = reinterpret_cast<MacFixupList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, MacFixupBinder::class_name(), object->GetFixupByAddress(address));
	return 1;
}

/**
 * MacFixupBinder
 */

void MacFixupBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"setDeleted", &SafeFunction<set_deleted>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);
}

int MacFixupBinder::address(lua_State *state)
{
	MacFixup *object = reinterpret_cast<MacFixup *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MacFixupBinder::set_deleted(lua_State *state)
{
	MacFixup *object = reinterpret_cast<MacFixup *>(check_object(state, 1, class_name()));
	bool value = lua_toboolean(state, 2) == 1;
	object->set_deleted(value);
	return 0;
}

/**
 * ELFFileBinder
 */

void ELFFileBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"format", &SafeFunction<format>},
		{"size", &SafeFunction<size>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"flush", &SafeFunction<flush>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFFormatBinder::Register(state);
	ELFArchitectureBinder::Register(state);
}

int ELFFileBinder::item(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFArchitectureBinder::class_name(), object->item(index));
	return 1;
}

int ELFFileBinder::count(lua_State *state)
{
	PEFile *object = reinterpret_cast<PEFile *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFFileBinder::name(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	std::string name = object->file_name(true);
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFFileBinder::format(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	std::string name = object->format_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFFileBinder::size(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int ELFFileBinder::seek(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int ELFFileBinder::tell(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int ELFFileBinder::write(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int ELFFileBinder::flush(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	object->Flush();
	return 0;
}

int ELFFileBinder::read(lua_State *state)
{
	ELFFile *object = reinterpret_cast<ELFFile *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (size)
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * ELFArchitectureBinder
 */

void ELFArchitectureBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"file", &SafeFunction<file>},
		{"entryPoint", &SafeFunction<entry_point>},
		{"imageBase", &SafeFunction<image_base>},
		{"cpuAddressSize", &SafeFunction<cpu_address_size>},
		{"size", &SafeFunction<size>},
		{"segments", &SafeFunction<segments>},
		{"sections", &SafeFunction<sections>},
		{"functions", &SafeFunction<functions>},
		{"directories", &SafeFunction<directories>},
		{"imports", &SafeFunction<imports>},
		{"exports", &SafeFunction<exports>},
		{"fixups", &SafeFunction<fixups>},
		{"mapFunctions", &SafeFunction<map_functions>},
		{"folders", &SafeFunction<folders>},
		{"addressSeek", &SafeFunction<address_seek>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFSegmentsBinder::Register(state);
	ELFSectionsBinder::Register(state);
	ELFDirectoriesBinder::Register(state);
	ELFImportsBinder::Register(state);
	ELFExportsBinder::Register(state);
	ELFFixupsBinder::Register(state);
}

int ELFArchitectureBinder::segments(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFSegmentsBinder::class_name(), object->segment_list());
	return 1;
}

int ELFArchitectureBinder::sections(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFSectionsBinder::class_name(), object->section_list());
	return 1;
}

int ELFArchitectureBinder::name(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFArchitectureBinder::file(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFFileBinder::class_name(), object->owner());
	return 1;
}

int ELFArchitectureBinder::entry_point(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->entry_point());
	return 1;
}

int ELFArchitectureBinder::image_base(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->image_base());
	return 1;
}

int ELFArchitectureBinder::cpu_address_size(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->cpu_address_size());
	return 1;
}

int ELFArchitectureBinder::size(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int ELFArchitectureBinder::functions(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, IntelFunctionsBinder::class_name(), object->function_list());
	return 1;
}

int ELFArchitectureBinder::directories(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEDirectoriesBinder::class_name(), object->command_list());
	return 1;
}

int ELFArchitectureBinder::imports(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFImportsBinder::class_name(), object->import_list());
	return 1;
}

int ELFArchitectureBinder::exports(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFExportsBinder::class_name(), object->export_list());
	return 1;
}

int ELFArchitectureBinder::fixups(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ELFFixupsBinder::class_name(), object->fixup_list());
	return 1;
}

int ELFArchitectureBinder::map_functions(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MapFunctionsBinder::class_name(), object->map_function_list());
	return 1;
}

int ELFArchitectureBinder::folders(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, FoldersBinder::class_name(), object->owner()->folder_list());
	return 1;
}

int ELFArchitectureBinder::address_seek(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	lua_pushboolean(state, object->AddressSeek(address));
	return 1;
}

int ELFArchitectureBinder::seek(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int ELFArchitectureBinder::tell(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int ELFArchitectureBinder::write(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int ELFArchitectureBinder::read(lua_State *state)
{
	ELFArchitecture *object = reinterpret_cast<ELFArchitecture *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (size) 
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
 * ELFSegmentsBinder
 */

void ELFSegmentsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	ELFSegmentBinder::Register(state);
}

int ELFSegmentsBinder::item(lua_State *state)
{
	ELFSegmentList *object = reinterpret_cast<ELFSegmentList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFSegmentBinder::class_name(), object->item(index));
	return 1;
}

int ELFSegmentsBinder::count(lua_State *state)
{
	ELFSegmentList *object = reinterpret_cast<ELFSegmentList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFSegmentsBinder::GetItemByAddress(lua_State *state)
{
	ELFSegmentList *object = reinterpret_cast<ELFSegmentList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, ELFSegmentBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

/**
 * ELFSegmentBinder
 */

void ELFSegmentBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"physicalOffset", &SafeFunction<physical_offset>},
		{"physicalSize", &SafeFunction<physical_size>},
		{"flags", &SafeFunction<flags>},
		{"excludedFromPacking", &SafeFunction<excluded_from_packing>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ELFSegmentBinder::size(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int ELFSegmentBinder::address(lua_State *state)
{
	PESegment *object = reinterpret_cast<PESegment *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFSegmentBinder::name(lua_State *state)
{
	ELFSegment *object = reinterpret_cast<ELFSegment *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFSegmentBinder::physical_offset(lua_State *state)
{
	ELFSegment *object = reinterpret_cast<ELFSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int ELFSegmentBinder::physical_size(lua_State *state)
{
	ELFSegment *object = reinterpret_cast<ELFSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_size());
	return 1;
}

int ELFSegmentBinder::flags(lua_State *state)
{
	ELFSegment *object = reinterpret_cast<ELFSegment *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int ELFSegmentBinder::excluded_from_packing(lua_State *state)
{
	ELFSegment *object = reinterpret_cast<ELFSegment *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->excluded_from_packing());
	return 1;
}

/**
 * ELFSectionsBinder
 */

void ELFSectionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	ELFSectionBinder::Register(state);
}

int ELFSectionsBinder::item(lua_State *state)
{
	ELFSectionList *object = reinterpret_cast<ELFSectionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFSectionBinder::class_name(), object->item(index));
	return 1;
}

int ELFSectionsBinder::count(lua_State *state)
{
	ELFSectionList *object = reinterpret_cast<ELFSectionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFSectionsBinder::GetItemByAddress(lua_State *state)
{
	ELFSectionList *object = reinterpret_cast<ELFSectionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, ELFSectionBinder::class_name(), object->GetSectionByAddress(address));
	return 1;
}

/**
 * ELFSectionBinder
 */

void ELFSectionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"offset", &SafeFunction<offset>},
		{"segment", &SafeFunction<segment>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ELFSectionBinder::size(lua_State *state)
{
	ELFSection *object = reinterpret_cast<ELFSection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int ELFSectionBinder::address(lua_State *state)
{
	ELFSection *object = reinterpret_cast<ELFSection *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFSectionBinder::name(lua_State *state)
{
	ELFSection *object = reinterpret_cast<ELFSection *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFSectionBinder::offset(lua_State *state)
{
	ELFSection *object = reinterpret_cast<ELFSection *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->physical_offset());
	return 1;
}

int ELFSectionBinder::segment(lua_State *state)
{
	ELFSection *object = reinterpret_cast<ELFSection *>(check_object(state, 1, class_name()));
	push_object(state, ELFSegmentBinder::class_name(), object->parent());
	return 1;
}

/**
 * ELFDirectoriesBinder
 */

void ELFDirectoriesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByType", &SafeFunction<GetItemByType>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFDirectoryBinder::Register(state);
}

int ELFDirectoriesBinder::item(lua_State *state)
{
	ELFDirectoryList *object = reinterpret_cast<ELFDirectoryList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFDirectoryBinder::class_name(), object->item(index));
	return 1;
}

int ELFDirectoriesBinder::count(lua_State *state)
{
	ELFDirectoryList *object = reinterpret_cast<ELFDirectoryList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFDirectoriesBinder::GetItemByType(lua_State *state)
{
	ELFDirectoryList *object = reinterpret_cast<ELFDirectoryList *>(check_object(state, 1, class_name()));
	uint32_t type = static_cast<uint32_t>(check_integer(state, 2));
	push_object(state, ELFDirectoryBinder::class_name(), object->GetCommandByType(type));
	return 1;
}

/**
 * ELFDirectoryBinder
 */

void ELFDirectoryBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"type", &SafeFunction<type>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ELFDirectoryBinder::size(lua_State *state)
{
	ELFDirectory *object = reinterpret_cast<ELFDirectory *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int ELFDirectoryBinder::address(lua_State *state)
{
	ELFDirectory *object = reinterpret_cast<ELFDirectory *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFDirectoryBinder::name(lua_State *state)
{
	ELFDirectory *object = reinterpret_cast<ELFDirectory *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFDirectoryBinder::type(lua_State *state)
{
	ELFDirectory *object = reinterpret_cast<ELFDirectory *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * ELFFormatBinder
 */

void ELFFormatBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		// segment types
		{"PT_NULL", PT_NULL},
		{"PT_LOAD", PT_LOAD},
		{"PT_DYNAMIC", PT_DYNAMIC},
		{"PT_INTERP", PT_INTERP},
		{"PT_NOTE", PT_NOTE},
		{"PT_SHLIB", PT_SHLIB},
		{"PT_PHDR", PT_PHDR},
		{"PT_TLS", PT_TLS},
		{"PT_GNU_EH_FRAME", PT_GNU_EH_FRAME},
		{"PT_GNU_STACK", PT_GNU_STACK},
		{"PT_GNU_RELRO", PT_GNU_RELRO},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * ELFImportsBinder
 */

void ELFImportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFImportBinder::Register(state);
}

int ELFImportsBinder::item(lua_State *state)
{
	ELFImportList *object = reinterpret_cast<ELFImportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFImportBinder::class_name(), object->item(index));
	return 1;
}

int ELFImportsBinder::count(lua_State *state)
{
	ELFImportList *object = reinterpret_cast<ELFImportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFImportsBinder::GetItemByName(lua_State *state)
{
	ELFImportList *object = reinterpret_cast<ELFImportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, ELFImportBinder::class_name(), object->GetImportByName(name));
	return 1;
}

/**
 * ELFImportBinder
 */

void ELFImportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"isSDK", &SafeFunction<is_sdk>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFImportFunctionBinder::Register(state);
}

int ELFImportBinder::item(lua_State *state)
{
	ELFImport *object = reinterpret_cast<ELFImport *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MacImportFunctionBinder::class_name(), object->item(index));
	return 1;
}

int ELFImportBinder::count(lua_State *state)
{
	ELFImport *object = reinterpret_cast<ELFImport *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFImportBinder::name(lua_State *state)
{
	ELFImport *object = reinterpret_cast<ELFImport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFImportBinder::is_sdk(lua_State *state)
{
	ELFImport *object = reinterpret_cast<ELFImport *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_sdk());
	return 1;
}

/**
 * ELFImportFunctionBinder
 */

void ELFImportFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ELFImportFunctionBinder::address(lua_State *state)
{
	ELFImportFunction *object = reinterpret_cast<ELFImportFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFImportFunctionBinder::name(lua_State *state)
{
	ELFImportFunction *object = reinterpret_cast<ELFImportFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFImportFunctionBinder::type(lua_State *state)
{
	ELFImportFunction *object = reinterpret_cast<ELFImportFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * ELFExportsBinder
 */

void ELFExportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ELFExportBinder::Register(state);
}

int ELFExportsBinder::item(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFExportBinder::class_name(), object->item(index));
	return 1;
}

int ELFExportsBinder::count(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFExportsBinder::clear(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int ELFExportsBinder::Delete(lua_State *state)
{
	MacExportList *object = reinterpret_cast<MacExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int ELFExportsBinder::name(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFExportsBinder::GetItemByAddress(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, ELFExportBinder::class_name(), object->GetExportByAddress(address));
	return 1;
}

int ELFExportsBinder::GetItemByName(lua_State *state)
{
	ELFExportList *object = reinterpret_cast<ELFExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, ELFExportBinder::class_name(), object->GetExportByName(name));
	return 1;
}

/**
 * ELFExportBinder
 */

void ELFExportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"forwardedName", &SafeFunction<forwarded_name>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ELFExportBinder::address(lua_State *state)
{
	ELFExport *object = reinterpret_cast<ELFExport *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFExportBinder::name(lua_State *state)
{
	ELFExport *object = reinterpret_cast<ELFExport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFExportBinder::forwarded_name(lua_State *state)
{
	ELFExport *object = reinterpret_cast<ELFExport *>(check_object(state, 1, class_name()));
	std::string name = object->forwarded_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ELFExportBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
 * ELFFixupsBinder
 */

void ELFFixupsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"indexOf", &SafeFunction<IndexOf>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);

	ELFFixupBinder::Register(state);
}

int ELFFixupsBinder::item(lua_State *state)
{
	ELFFixupList *object = reinterpret_cast<ELFFixupList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ELFFixupBinder::class_name(), object->item(index));
	return 1;
}

int ELFFixupsBinder::count(lua_State *state)
{
	ELFFixupList *object = reinterpret_cast<ELFFixupList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ELFFixupsBinder::IndexOf(lua_State *state)
{
	ELFFixupList *object = reinterpret_cast<ELFFixupList *>(check_object(state, 1, class_name()));
	ELFFixup *item = reinterpret_cast<ELFFixup *>(check_object(state, 2, ELFFixupBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

int ELFFixupsBinder::GetItemByAddress(lua_State *state)
{
	MacFixupList *object = reinterpret_cast<MacFixupList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, ELFFixupBinder::class_name(), object->GetFixupByAddress(address));
	return 1;
}

/**
 * ELFFixupBinder
 */

void ELFFixupBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"setDeleted", &SafeFunction<set_deleted>},
		{NULL, NULL}
	};
	register_class(state, class_name(), methods);
}

int ELFFixupBinder::address(lua_State *state)
{
	ELFFixup *object = reinterpret_cast<ELFFixup *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ELFFixupBinder::set_deleted(lua_State *state)
{
	ELFFixup *object = reinterpret_cast<ELFFixup *>(check_object(state, 1, class_name()));
	bool value = lua_toboolean(state, 2) == 1;
	object->set_deleted(value);
	return 0;
}

/**
 * NETArchitectureBinder
 */

void NETArchitectureBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"file", &SafeFunction<file>},
		{"entryPoint", &SafeFunction<entry_point>},
		{"imageBase", &SafeFunction<image_base>},
		{"cpuAddressSize", &SafeFunction<cpu_address_size>},
		{"size", &SafeFunction<size>},
		{"segments", &SafeFunction<segments>},
		{"sections", &SafeFunction<sections>},
		{"functions", &SafeFunction<functions>},
		{"streams", &SafeFunction<streams>},
		{"exports", &SafeFunction<exports>},
		{"imports", &SafeFunction<imports>},
		{"mapFunctions", &SafeFunction<map_functions>},
		{"folders", &SafeFunction<folders>},
		{"addressSeek", &SafeFunction<address_seek>},
		{"seek", &SafeFunction<seek>},
		{"tell", &SafeFunction<tell>},
		{"write", &SafeFunction<write>},
		{"read", &SafeFunction<read>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETMetaDataBinder::Register(state);
	NETImportsBinder::Register(state);
	NETExportsBinder::Register(state);
	ILFunctionsBinder::Register(state);
}

int NETArchitectureBinder::segments(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PESegmentsBinder::class_name(), object->segment_list());
	return 1;
}

int NETArchitectureBinder::sections(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PESectionsBinder::class_name(), object->section_list());
	return 1;
}

int NETArchitectureBinder::name(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETArchitectureBinder::file(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, PEFileBinder::class_name(), object->owner());
	return 1;
}

int NETArchitectureBinder::entry_point(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->entry_point());
	return 1;
}

int NETArchitectureBinder::image_base(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->image_base());
	return 1;
}

int NETArchitectureBinder::cpu_address_size(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->cpu_address_size());
	return 1;
}

int NETArchitectureBinder::size(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->size());
	return 1;
}

int NETArchitectureBinder::functions(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, ILFunctionsBinder::class_name(), object->function_list());
	return 1;
}

int NETArchitectureBinder::streams(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, NETMetaDataBinder::class_name(), object->command_list());
	return 1;
}

int NETArchitectureBinder::exports(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, NETExportsBinder::class_name(), object->export_list());
	return 1;
}

int NETArchitectureBinder::imports(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, NETImportsBinder::class_name(), object->import_list());
	return 1;
}

int NETArchitectureBinder::map_functions(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, MapFunctionsBinder::class_name(), object->map_function_list());
	return 1;
}

int NETArchitectureBinder::folders(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_object(state, FoldersBinder::class_name(), object->owner()->folder_list());
	return 1;
}

int NETArchitectureBinder::address_seek(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	lua_pushboolean(state, object->AddressSeek(address));
	return 1;
}

int NETArchitectureBinder::seek(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	size_t position = static_cast<size_t>(check_integer(state, 2));
	push_uint64(state, object->Seek(position));
	return 1;
}

int NETArchitectureBinder::tell(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	push_uint64(state, object->Tell());
	return 1;
}

int NETArchitectureBinder::write(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	int top = lua_gettop(state);
	int res = 0;
	for (int i = 2; i <= top; i++) {
		size_t l;
		const char *s = luaL_checklstring(state, i, &l);
		res += (int)object->Write(s, l);
    }
	lua_pushinteger(state, res);
	return 1;
}

int NETArchitectureBinder::read(lua_State *state)
{
	NETArchitecture *object = reinterpret_cast<NETArchitecture *>(check_object(state, 1, class_name()));
	size_t size = static_cast<size_t>(check_integer(state, 2));
	std::string buffer;
	buffer.resize(size);
	if (size)
		object->Read(&buffer[0], buffer.size());
	lua_pushlstring(state, buffer.c_str(), buffer.size());
	return 1;
}

/**
* NETFormatBinder
*/

void NETFormatBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		// CorTypeAttr
		{"tdVisibilityMask", tdVisibilityMask},
		{"tdNotPublic", tdNotPublic},
		{"tdPublic", tdPublic},
		{"tdNestedPublic", tdNestedPublic},
		{"tdNestedPrivate", tdNestedPrivate},
		{"tdNestedFamily", tdNestedFamily},
		{"tdNestedAssembly", tdNestedAssembly},
		{"tdNestedFamANDAssem", tdNestedFamANDAssem},
		{"tdNestedFamORAssem", tdNestedFamORAssem},
		{"tdLayoutMask", tdLayoutMask},
		{"tdAutoLayout", tdAutoLayout},
		{"tdSequentialLayout", tdSequentialLayout},
		{"tdExplicitLayout", tdExplicitLayout},
		{"tdClassSemanticsMask", tdClassSemanticsMask},
		{"tdClass", tdClass},
		{"tdInterface", tdInterface},
		{"tdAbstract", tdAbstract},
		{"tdSealed", tdSealed},
		{"tdSpecialName", tdSpecialName},
		{"tdImport", tdImport},
		{"tdSerializable", tdSerializable},
		{"tdStringFormatMask", tdStringFormatMask},
		{"tdAnsiClass", tdAnsiClass},
		{"tdUnicodeClass", tdUnicodeClass},
		{"tdAutoClass", tdAutoClass},
		{"tdBeforeFieldInit", tdBeforeFieldInit},
		{"mdReservedMask", mdReservedMask},
		{"mdRTSpecialName", mdRTSpecialName},
		{"mdHasSecurity", mdHasSecurity},
		{"mdRequireSecObject", mdRequireSecObject},
		// CorMethodAttr
		{"mdMemberAccessMask", mdMemberAccessMask},
		{"mdPrivateScope", mdPrivateScope},
		{"mdPrivate", mdPrivate},
		{"mdFamANDAssem", mdFamANDAssem},
		{"mdAssem", mdAssem},
		{"mdFamily", mdFamily},
		{"mdFamORAssem", mdFamORAssem},
		{"mdPublic", mdPublic},
		{"mdStatic", mdStatic},
		{"mdFinal", mdFinal},
		{"mdVirtual", mdVirtual},
		{"mdHideBySig", mdHideBySig},
		{"mdVtableLayoutMask", mdVtableLayoutMask},
		{"mdReuseSlot", mdReuseSlot},
		{"mdNewSlot", mdNewSlot},
		{"mdCheckAccessOnOverride", mdCheckAccessOnOverride},
		{"mdAbstract", mdAbstract},
		{"mdSpecialName", mdSpecialName},
		{"mdPinvokeImpl", mdPinvokeImpl},
		{"mdUnmanagedExport", mdUnmanagedExport},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * NETMetaDataBinder
 */

void NETMetaDataBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"heap", &SafeFunction<heap>},
		{"table", &SafeFunction<table>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETHeapBinder::Register(state);
	NETStreamBinder::Register(state);
}

int NETMetaDataBinder::item(lua_State *state)
{
	ILMetaData *object = reinterpret_cast<ILMetaData *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_stream(state, object->item(index));
	return 1;
}

int NETMetaDataBinder::count(lua_State *state)
{
	ILMetaData *object = reinterpret_cast<ILMetaData *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int NETMetaDataBinder::heap(lua_State *state)
{
	ILMetaData *object = reinterpret_cast<ILMetaData *>(check_object(state, 1, class_name()));
	push_stream(state, object->GetStreamByName("#~"));
	return 1;
}

void NETMetaDataBinder::push_stream(lua_State *state, ILStream *stream)
{
	std::string class_name;
	if (dynamic_cast<ILHeapStream *>(stream))
		class_name = NETHeapBinder::class_name();
	else
		class_name = NETStreamBinder::class_name();
	push_object(state, class_name.c_str(), stream);
}

int NETMetaDataBinder::table(lua_State *state)
{
	ILMetaData *object = reinterpret_cast<ILMetaData *>(check_object(state, 1, class_name()));
	ILTokenType index = static_cast<ILTokenType>(check_integer(state, 2));
	push_object(state, NETTableBinder::class_name(), object->table(index));
	return 1;
}

/**
 * NETStreamBinder
 */

void NETStreamBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETStreamBinder::size(lua_State *state)
{
	ILStream *object = reinterpret_cast<ILStream *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int NETStreamBinder::address(lua_State *state)
{
	ILStream *object = reinterpret_cast<ILStream *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int NETStreamBinder::name(lua_State *state)
{
	ILStream *object = reinterpret_cast<ILStream *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
 * NETHeapBinder
 */

void NETHeapBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"size", &SafeFunction<size>},
		{"table", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETTableBinder::Register(state);
}

int NETHeapBinder::size(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int NETHeapBinder::address(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int NETHeapBinder::name(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETHeapBinder::item(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, NETTableBinder::class_name(), object->table(index));
	return 1;
}

int NETHeapBinder::count(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int NETHeapBinder::GetItemByType(lua_State *state)
{
	ILHeapStream *object = reinterpret_cast<ILHeapStream *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, NETTableBinder::class_name(), object->table(index));
	return 1;
}

/**
 * TokenTypeBinder
 */

void TokenTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"Module", ttModule},
		{"TypeRef", ttTypeRef},
		{"TypeDef", ttTypeDef},
		{"Field", ttField},
		{"MethodDef", ttMethodDef},
		{"Param", ttParam},
		{"InterfaceImpl", ttInterfaceImpl},
		{"MemberRef", ttMemberRef},
		{"Constant", ttConstant},
		{"CustomAttribute", ttCustomAttribute},
		{"FieldMarshal", ttFieldMarshal},
		{"DeclSecurity", ttDeclSecurity},
		{"ClassLayout", ttClassLayout},
		{"FieldLayout", ttFieldLayout},
		{"StandAloneSig", ttStandAloneSig},
		{"EventMap", ttEventMap},
		{"Event", ttEvent},
		{"PropertyMap", ttPropertyMap},
		{"Property", ttProperty},
		{"MethodSemantics", ttMethodSemantics},
		{"MethodImpl", ttMethodImpl},
		{"ModuleRef", ttModuleRef},
		{"TypeSpec", ttTypeSpec},
		{"ImplMap", ttImplMap},
		{"FieldRVA", ttFieldRVA},
		{"Assembly", ttAssembly},
		{"AssemblyProcessor", ttAssemblyProcessor},
		{"AssemblyOS", ttAssemblyOS},
		{"AssemblyRef", ttAssemblyRef},
		{"AssemblyRefProcessor", ttAssemblyRefProcessor},
		{"AssemblyRefOS", ttAssemblyRefOS},
		{"File", ttFile},
		{"ExportedType", ttExportedType},
		{"ManifestResource", ttManifestResource},
		{"NestedClass", ttNestedClass},
		{"GenericParam", ttGenericParam},
		{"MethodSpec", ttMethodSpec},
		{"GenericParamConstraint", ttGenericParamConstraint},
		{"UserString", ttUserString},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * NETTableBinder
 */

void push_token(lua_State *state, ILToken *token)
{
	std::string class_name = NETTokenBinder::class_name();
	if (token) {
		switch (token->type()) {
		case ttTypeDef:
			class_name = NETTypeDefBinder::class_name();
			break;
		case ttTypeRef:
			class_name = NETTypeRefBinder::class_name();
			break;
		case ttAssemblyRef:
			class_name = NETAssemblyRefBinder::class_name();
			break;
		case ttMethodDef:
			class_name = NETMethodDefBinder::class_name();
			break;
		case ttField:
			class_name = NETFieldBinder::class_name();
			break;
		case ttParam:
			class_name = NETParamBinder::class_name();
			break;
		case ttCustomAttribute:
			class_name = NETCustomAttributeBinder::class_name();
			break;
		case ttMemberRef:
			class_name = NETMemberRefBinder::class_name();
			break;
		case ttUserString:
			class_name = NETUserStringBinder::class_name();
			break;
		}
	}
	push_object(state, class_name.c_str(), token);
}

void NETTableBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"type", &SafeFunction<type>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	TokenTypeBinder::Register(state);
	NETTokenBinder::Register(state);
	NETTypeDefBinder::Register(state);
	NETTypeRefBinder::Register(state);
	NETAssemblyRefBinder::Register(state);
	NETMethodDefBinder::Register(state);
	NETFieldBinder::Register(state);
	NETParamBinder::Register(state);
	NETCustomAttributeBinder::Register(state);
	NETMemberRefBinder::Register(state);
	NETUserStringBinder::Register(state);
}

int NETTableBinder::type(lua_State *state)
{
	ILTable *object = reinterpret_cast<ILTable *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETTableBinder::item(lua_State *state)
{
	ILTable *object = reinterpret_cast<ILTable *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_token(state, object->item(index));
	return 1;
}

int NETTableBinder::count(lua_State *state)
{
	ILTable *object = reinterpret_cast<ILTable *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

/**
* NETTokenTypeBinder
*/

void NETTokenTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"Module", ttModule},
		{"TypeRef", ttTypeRef},
		{"TypeDef", ttTypeDef},
		{"Field", ttField},
		{"MethodDef", ttMethodDef},
		{"Param", ttParam},
		{"InterfaceImpl", ttInterfaceImpl},
		{"MemberRef", ttMemberRef},
		{"Constant", ttConstant},
		{"CustomAttribute", ttCustomAttribute},
		{"FieldMarshal", ttFieldMarshal},
		{"DeclSecurity", ttDeclSecurity},
		{"ClassLayout", ttClassLayout},
		{"FieldLayout", ttFieldLayout},
		{"StandAloneSig", ttStandAloneSig},
		{"EventMap", ttEventMap},
		{"Event", ttEvent},
		{"PropertyMap", ttPropertyMap},
		{"Property", ttProperty},
		{"MethodSemantics", ttMethodSemantics},
		{"MethodImpl", ttMethodImpl},
		{"ModuleRef", ttModuleRef},
		{"TypeSpec", ttTypeSpec},
		{"ImplMap", ttImplMap},
		{"FieldRVA", ttFieldRVA},
		{"Assembly", ttAssembly},
		{"AssemblyProcessor", ttAssemblyProcessor},
		{"AssemblyOS", ttAssemblyOS},
		{"AssemblyRef", ttAssemblyRef},
		{"AssemblyRefProcessor", ttAssemblyRefProcessor},
		{"AssemblyRefOS", ttAssemblyRefOS},
		{"File", ttFile},
		{"ExportedType", ttExportedType},
		{"ManifestResource", ttManifestResource},
		{"NestedClass", ttNestedClass},
		{"GenericParam", ttGenericParam},
		{"MethodSpec", ttMethodSpec},
		{"GenericParamConstraint", ttGenericParamConstraint},
		{"UserString", ttUserString},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * NETTokenBinder
 */

void NETTokenBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"canRename", &SafeFunction<can_rename>},
		{"setCanRename", &SafeFunction<set_can_rename>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETTokenTypeBinder::Register(state);
}

int NETTokenBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETTokenBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETTokenBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETTokenBinder::can_rename(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->can_rename());
	return 1;
}

int NETTokenBinder::set_can_rename(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	bool can_rename = lua_toboolean(state, 2) == 1;
	object->set_can_rename(can_rename);
	return 0;
}

/**
* NETMethodsBinder
*/

void NETMethodsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETMethodsBinder::item(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	ILMethodDef *item = reinterpret_cast<ILMethodDef *>(table->item(self_index + index));
	if (item->declaring_type() != object->declaring_type())
		throw std::runtime_error("subscript out of range");
	push_token(state, item);
	return 1;
}

int NETMethodsBinder::count(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	size_t res = 0;
	for (size_t i = self_index; i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
		if (method->declaring_type() != object->declaring_type())
			break;
		res++;
	}
	lua_pushinteger(state, res);
	return 1;
}

/**
* NETFieldsBinder
*/

void NETFieldsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETFieldsBinder::item(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	ILField *item = reinterpret_cast<ILField *>(table->item(self_index + index));
	if (item->declaring_type() != object->declaring_type())
		throw std::runtime_error("subscript out of range");
	push_token(state, item);
	return 1;
}

int NETFieldsBinder::count(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	size_t res = 0;
	for (size_t i = self_index; i < table->count(); i++) {
		ILField *field = reinterpret_cast<ILField *>(table->item(i));
		if (field->declaring_type() != object->declaring_type())
			break;
		res++;
	}
	lua_pushinteger(state, res);
	return 1;
}

/**
* NETTypeDefBinder
*/

void NETTypeDefBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"namespace", &SafeFunction<name_space>},
		{"name", &SafeFunction<name>},
		{"fullName", &SafeFunction<full_name>},
		{"flags", &SafeFunction<flags>},
		{"declaringType", &SafeFunction<declaring_type>},
		{"baseType", &SafeFunction<base_type>},
		{"methods", &SafeFunction<method_list>},
		{"fields", &SafeFunction<field_list>},
		{"setNamespace", &SafeFunction<set_namespace>},
		{"setName", &SafeFunction<set_name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETMethodsBinder::Register(state);
	NETFieldsBinder::Register(state);
}

int NETTypeDefBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETTypeDefBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETTypeDefBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETTypeDefBinder::name_space(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	std::string name = object->name_space();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeDefBinder::name(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeDefBinder::full_name(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	std::string name = object->full_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeDefBinder::flags(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int NETTypeDefBinder::declaring_type(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	push_token(state, object->declaring_type());
	return 1;
}

int NETTypeDefBinder::base_type(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	push_token(state, object->base_type());
	return 1;
}

int NETTypeDefBinder::method_list(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	push_object(state, NETMethodsBinder::class_name(), object->method_list());
	return 1;
}

int NETTypeDefBinder::field_list(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	push_object(state, NETFieldsBinder::class_name(), object->field_list());
	return 1;
}

int NETTypeDefBinder::set_namespace(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_namespace(name);
	return 0;
}

int NETTypeDefBinder::set_name(lua_State *state)
{
	ILTypeDef *object = reinterpret_cast<ILTypeDef *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
* NETTypeRefBinder
*/

void NETTypeRefBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"namespace", &SafeFunction<name_space>},
		{"name", &SafeFunction<name> },
		{"fullName", &SafeFunction<full_name>},
		{"resolutionScope", &SafeFunction<resolution_scope>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETTypeRefBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETTypeRefBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETTypeRefBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETTypeRefBinder::name_space(lua_State *state)
{
	ILTypeRef *object = reinterpret_cast<ILTypeRef *>(check_object(state, 1, class_name()));
	std::string name = object->name_space();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeRefBinder::name(lua_State *state)
{
	ILTypeRef *object = reinterpret_cast<ILTypeRef *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeRefBinder::full_name(lua_State *state)
{
	ILTypeRef *object = reinterpret_cast<ILTypeRef *>(check_object(state, 1, class_name()));
	std::string name = object->full_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETTypeRefBinder::resolution_scope(lua_State *state)
{
	ILTypeRef *object = reinterpret_cast<ILTypeRef *>(check_object(state, 1, class_name()));
	push_token(state, object->resolution_scope());
	return 1;
}

/**
* NETAssemblyRefBinder
*/

void NETAssemblyRefBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"name", &SafeFunction<name>},
		{"fullName", &SafeFunction<full_name>},
		{ NULL, NULL }
	};

	register_class(state, class_name(), methods);
}

int NETAssemblyRefBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETAssemblyRefBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETAssemblyRefBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETAssemblyRefBinder::name(lua_State *state)
{
	ILAssemblyRef *object = reinterpret_cast<ILAssemblyRef *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETAssemblyRefBinder::full_name(lua_State *state)
{
	ILAssemblyRef *object = reinterpret_cast<ILAssemblyRef *>(check_object(state, 1, class_name()));
	std::string name = object->full_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
* NETParamsBinder
*/

void NETParamsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETParamsBinder::item(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	ILParam *item = reinterpret_cast<ILParam *>(table->item(self_index + index));
	if (item->parent() != object->parent())
		throw std::runtime_error("subscript out of range");
	push_token(state, item);
	return 1;
}

int NETParamsBinder::count(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	ILTable *table = object->owner();
	size_t self_index = table->IndexOf(object);
	if (self_index == NOT_ID)
		throw std::runtime_error("subscript out of range");
	size_t res = 0;
	for (size_t i = self_index; i < table->count(); i++) {
		ILParam *method = reinterpret_cast<ILParam *>(table->item(i));
		if (method->parent() != object->parent())
			break;
		res++;
	}
	lua_pushinteger(state, res);
	return 1;
}

/**
* NETMethodDefBinder
*/

void NETMethodDefBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"name", &SafeFunction<name>},
		{"fullName", &SafeFunction<full_name>},
		{"address", &SafeFunction<address>},
		{"flags", &SafeFunction<flags>},
		{"implFlags", &SafeFunction<impl_flags>},
		{"parent", &SafeFunction<parent>},
		{"params", &SafeFunction<param_list>},
		{"setName", &SafeFunction<set_name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETParamsBinder::Register(state);
}

int NETMethodDefBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETMethodDefBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETMethodDefBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETMethodDefBinder::name(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETMethodDefBinder::full_name(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	std::string name = object->declaring_type()->full_name() + "::" + object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETMethodDefBinder::address(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int NETMethodDefBinder::flags(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int NETMethodDefBinder::impl_flags(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->impl_flags());
	return 1;
}

int NETMethodDefBinder::parent(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	push_token(state, object->declaring_type());
	return 1;
}

int NETMethodDefBinder::param_list(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	push_object(state, NETParamsBinder::class_name(), object->param_list());
	return 1;
}

int NETMethodDefBinder::set_name(lua_State *state)
{
	ILMethodDef *object = reinterpret_cast<ILMethodDef *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
* NETFieldBinder
*/

void NETFieldBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"flags", &SafeFunction<flags>},
		{"name", &SafeFunction<name>},
		{"declaringType", &SafeFunction<declaring_type>},
		{"setName", &SafeFunction<set_name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETFieldBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETFieldBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETFieldBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETFieldBinder::name(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETFieldBinder::flags(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int NETFieldBinder::declaring_type(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	push_token(state, object->declaring_type());
	return 1;
}

int NETFieldBinder::set_name(lua_State *state)
{
	ILField *object = reinterpret_cast<ILField *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
* NETParamBinder
*/

void NETParamBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"name", &SafeFunction<name>},
		{"parent", &SafeFunction<parent>},
		{"flags", &SafeFunction<flags>},
		{"setName", &SafeFunction<set_name>},
		{ NULL, NULL }
	};

	register_class(state, class_name(), methods);
}

int NETParamBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETParamBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETParamBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETParamBinder::name(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETParamBinder::parent(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	push_token(state, object->parent());
	return 1;
}

int NETParamBinder::flags(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int NETParamBinder::set_name(lua_State *state)
{
	ILParam *object = reinterpret_cast<ILParam *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

/**
* NETCustomAttributeBinder
*/

void NETCustomAttributeBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"parent", &SafeFunction<parent>},
		{"refType", &SafeFunction<ref_type>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETCustomAttributeBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETCustomAttributeBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETCustomAttributeBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETCustomAttributeBinder::parent(lua_State *state)
{
	ILCustomAttribute *object = reinterpret_cast<ILCustomAttribute *>(check_object(state, 1, class_name()));
	push_token(state, object->parent());
	return 1;
}

int NETCustomAttributeBinder::ref_type(lua_State *state)
{
	ILCustomAttribute *object = reinterpret_cast<ILCustomAttribute *>(check_object(state, 1, class_name()));
	push_token(state, object->type());
	return 1;
}

/**
* NETMemberRefBinder
*/

void NETMemberRefBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"declaringType", &SafeFunction<declaring_type>},
		{"name", &SafeFunction<name>},
		{ NULL, NULL }
	};

	register_class(state, class_name(), methods);
}

int NETMemberRefBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETMemberRefBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETMemberRefBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETMemberRefBinder::declaring_type(lua_State *state)
{
	ILMemberRef *object = reinterpret_cast<ILMemberRef *>(check_object(state, 1, class_name()));
	push_token(state, object->declaring_type());
	return 1;
}

int NETMemberRefBinder::name(lua_State *state)
{
	ILMemberRef *object = reinterpret_cast<ILMemberRef *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
* NETUserStringBinder
*/

void NETUserStringBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"id", &SafeFunction<id>},
		{"type", &SafeFunction<type>},
		{"value", &SafeFunction<value>},
		{"name", &SafeFunction<name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETUserStringBinder::id(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->id());
	return 1;
}

int NETUserStringBinder::type(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int NETUserStringBinder::value(lua_State *state)
{
	ILToken *object = reinterpret_cast<ILToken *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value());
	return 1;
}

int NETUserStringBinder::name(lua_State *state)
{
	ILUserString *object = reinterpret_cast<ILUserString *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
 * NETImportBinder
 */

void NETImportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"isSDK", &SafeFunction<is_sdk>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETImportFunctionBinder::Register(state);
}

int NETImportBinder::item(lua_State *state)
{
	NETImport *object = reinterpret_cast<NETImport *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, NETImportFunctionBinder::class_name(), object->item(index));
	return 1;
}

int NETImportBinder::count(lua_State *state)
{
	NETImport *object = reinterpret_cast<NETImport *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int NETImportBinder::name(lua_State *state)
{
	NETImport *object = reinterpret_cast<NETImport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETImportBinder::is_sdk(lua_State *state)
{
	NETImport *object = reinterpret_cast<NETImport *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_sdk());
	return 1;
}

/**
 * NETImportFunctionBinder
 */

void NETImportFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETImportFunctionBinder::address(lua_State *state)
{
	NETImportFunction *object = reinterpret_cast<NETImportFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int NETImportFunctionBinder::name(lua_State *state)
{
	NETImportFunction *object = reinterpret_cast<NETImportFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int NETImportFunctionBinder::type(lua_State *state)
{
	NETImportFunction *object = reinterpret_cast<NETImportFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

/**
 * NETImportsBinder
 */

void NETImportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETImportBinder::Register(state);
}

int NETImportsBinder::item(lua_State *state)
{
	NETImportList *object = reinterpret_cast<NETImportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, NETImportBinder::class_name(), object->item(index));
	return 1;
}

int NETImportsBinder::count(lua_State *state)
{
	NETImportList *object = reinterpret_cast<NETImportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

/**
 * NETExportsBinder
 */

void NETExportsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	NETExportBinder::Register(state);
}

int NETExportsBinder::item(lua_State *state)
{
	NETExportList *object = reinterpret_cast<NETExportList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, NETExportBinder::class_name(), object->item(index));
	return 1;
}

int NETExportsBinder::count(lua_State *state)
{
	NETExportList *object = reinterpret_cast<NETExportList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int NETExportsBinder::clear(lua_State *state)
{
	NETExportList *object = reinterpret_cast<NETExportList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int NETExportsBinder::GetItemByAddress(lua_State *state)
{
	NETExportList *object = reinterpret_cast<NETExportList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, NETExportBinder::class_name(), object->GetExportByAddress(address));
	return 1;
}

int NETExportsBinder::GetItemByName(lua_State *state)
{
	NETExportList *object = reinterpret_cast<NETExportList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, NETExportBinder::class_name(), object->GetExportByName(name));
	return 1;
}

/**
 * NETExportBinder
 */

void NETExportBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int NETExportBinder::address(lua_State *state)
{
	NETExport *object = reinterpret_cast<NETExport *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int NETExportBinder::name(lua_State *state)
{
	NETExport *object = reinterpret_cast<NETExport *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
 * ILFunctionsBinder
 */

void ILFunctionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{"addByAddress", &SafeFunction<AddByAddress>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ILFunctionBinder::Register(state);
}

int ILFunctionsBinder::item(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ILFunctionBinder::class_name(), object->item(index));
	return 1;
}

int ILFunctionsBinder::count(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ILFunctionsBinder::clear(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int ILFunctionsBinder::Delete(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int ILFunctionsBinder::GetItemByAddress(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, ILFunctionBinder::class_name(), object->GetFunctionByAddress(address));
	return 1;
}

int ILFunctionsBinder::GetItemByName(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, ILFunctionBinder::class_name(), object->GetFunctionByName(name));
	return 1;
}

int ILFunctionsBinder::AddByAddress(lua_State *state)
{
	ILFunctionList *object = reinterpret_cast<ILFunctionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	int top = lua_gettop(state);
	CompilationType compilation_type = (top > 2) ? static_cast<CompilationType>(lua_tointeger(state, 3)) : ctVirtualization;
	bool need_compile = (top > 3) ? lua_toboolean(state, 4) == 1 : true;
	push_object(state, ILFunctionBinder::class_name(), object->AddByAddress(address, compilation_type, 0, need_compile, NULL));
	return 1;
}

/**
 * ILFunctionBinder
 */

void ILFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"type", &SafeFunction<type>},
		{"compilationType", &SafeFunction<compilation_type>},
		{"setCompilationType", &SafeFunction<set_compilation_type>},
		{"lockToKey", &SafeFunction<lock_to_key>},
		{"setLockToKey", &SafeFunction<set_lock_to_key>},
		{"needCompile", &SafeFunction<need_compile>},
		{"setNeedCompile", &SafeFunction<set_need_compile>},
		{"links", &SafeFunction<links>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"indexOf", &SafeFunction<IndexOf>},
		{"destroy", &SafeFunction<destroy>},
		{"info", &SafeFunction<info>},
		{"ranges", &SafeFunction<ranges>},
		{"folder", &SafeFunction<folder>},
		{"setFolder", &SafeFunction<set_folder>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ILCommandBinder::Register(state);
}

int ILFunctionBinder::address(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ILFunctionBinder::name(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int ILFunctionBinder::item(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ILCommandBinder::class_name(), object->item(index));
	return 1;
}

int ILFunctionBinder::count(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ILFunctionBinder::type(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int ILFunctionBinder::compilation_type(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->compilation_type());
	return 1;
}

int ILFunctionBinder::set_compilation_type(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	CompilationType compilation_type = static_cast<CompilationType>(check_integer(state, 2));
	object->set_compilation_type(compilation_type);
	return 0;
}

int ILFunctionBinder::lock_to_key(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, (object->compilation_options() & coLockToKey) != 0);
	return 1;
}

int ILFunctionBinder::set_lock_to_key(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	bool value = lua_toboolean(state, 2) == 1;
	uint32_t options = object->compilation_options();
	if (value) {
		options |= coLockToKey;
	} else {
		options &= ~coLockToKey;
	}
	object->set_compilation_options(options);
	return 0;
}

int ILFunctionBinder::need_compile(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->need_compile());
	return 1;
}

int ILFunctionBinder::set_need_compile(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	bool need_compile = lua_toboolean(state, 2) == 1;
	object->set_need_compile(need_compile);
	return 0;
}

int ILFunctionBinder::links(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	push_object(state, CommandLinksBinder::class_name(), object->link_list());
	return 1;
}

int ILFunctionBinder::GetItemByAddress(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	int top = lua_gettop(state);
	bool is_near = (top > 2) ? lua_toboolean(state, 3) == 1 : false;
	push_object(state, ILCommandBinder::class_name(), is_near ? object->GetCommandByNearAddress(address) : object->GetCommandByAddress(address));
	return 1;
}

int ILFunctionBinder::IndexOf(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	ILCommand *item = reinterpret_cast<ILCommand *>(check_object(state, 2, ILCommandBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

int ILFunctionBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

int ILFunctionBinder::info(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	push_object(state, FunctionInfoListBinder::class_name(), object->function_info_list());
	return 1;
}

int ILFunctionBinder::ranges(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	push_object(state, FunctionInfoBinder::class_name(), object->range_list());
	return 1;
}

int ILFunctionBinder::folder(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	push_object(state, FolderBinder::class_name(), object->folder());
	return 1;
}

int ILFunctionBinder::set_folder(lua_State *state)
{
	ILFunction *object = reinterpret_cast<ILFunction *>(check_object(state, 1, class_name()));
	Folder *folder = reinterpret_cast<Folder *>(check_object(state, 1, FolderBinder::class_name()));
	object->set_folder(folder);
	return 0;
}

/**
 * ILCommandBinder
 */

void ILCommandBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"type", &SafeFunction<type>},
		{"text", &SafeFunction<text>},
		{"size", &SafeFunction<size>},
		{"dump", &SafeFunction<dump>},
		{"link", &SafeFunction<link>},
		{"operandValue", &SafeFunction<operand_value>},
		{"options", &SafeFunction<options>},
		{"alignment", &SafeFunction<alignment>},
		{"tokenReference", &SafeFunction<token_reference>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ILCommandTypeBinder::Register(state);
}

int ILCommandBinder::address(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ILCommandBinder::type(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int ILCommandBinder::text(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	std::string text = object->text();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int ILCommandBinder::size(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->dump_size());
	return 1;
}

int ILCommandBinder::dump(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	lua_pushinteger(state, object->dump(index));
	return 1;
}

int ILCommandBinder::link(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	push_object(state, CommandLinkBinder::class_name(), object->link());
	return 1;
}

int ILCommandBinder::operand_value(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	push_uint64(state, object->operand_value());
	return 1;
}

int ILCommandBinder::options(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->options());
	return 1;
}

int ILCommandBinder::alignment(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->alignment());
	return 1;
}

int ILCommandBinder::token_reference(lua_State *state)
{
	ILCommand *object = reinterpret_cast<ILCommand *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->token_reference() != NULL);
	return 1;
}

/**
 * ILCommandTypeBinder
 */

void ILCommandTypeBinder::Register(lua_State *state)
{
	EnumReg values[_countof(ILOpCodes) + 1];
	memset(values, 0, sizeof(values));

	for (size_t i = 0; i < _countof(values) - 1; i++) {
		std::string str_name;
		switch (i) {
		case icUnknown:
			str_name = "unknown";
			break;
		case icByte:
			str_name = "byte";
			break;
		case icWord:
			str_name = "word";
			break;
		case icDword:
			str_name = "dword";
			break;
		case icComment:
			str_name = "comment";
			break;
		case icData:
			str_name = "data";
			break;
		case icCase:
			str_name = "case";
			break;
		default:
			str_name = ILOpCodes[i].name;
			break;
		}
		if (str_name.empty())
			continue;

		str_name[0] = toupper(str_name[0]);
		size_t size = str_name.size() + 1;
		char *name = new char[size];
		memcpy(name, str_name.c_str(), size);
		values[i].value = (int)i;
		values[i].name = name;
	}

	register_enum(state, enum_name(), values);

	for (size_t i = 0; i < _countof(values) - 1; i++) {
		delete [] values[i].name;
	}
}

/**
 * MapFunctionsBinder
 */

void MapFunctionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	MapFunctionBinder::Register(state);
}

int MapFunctionsBinder::item(lua_State *state)
{
	MapFunctionList *object = reinterpret_cast<MapFunctionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, MapFunctionBinder::class_name(), object->item(index));
	return 1;
}

int MapFunctionsBinder::count(lua_State *state)
{
	MapFunctionList *object = reinterpret_cast<MapFunctionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int MapFunctionsBinder::GetItemByAddress(lua_State *state)
{
	MapFunctionList *object = reinterpret_cast<MapFunctionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, MapFunctionBinder::class_name(), object->GetFunctionByAddress(address));
	return 1;
}

int MapFunctionsBinder::GetItemByName(lua_State *state)
{
	MapFunctionList *object = reinterpret_cast<MapFunctionList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, MapFunctionBinder::class_name(), object->GetFunctionByName(name));
	return 1;
}

/**
 * MapFunctionBinder
 */

void MapFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"size", &SafeFunction<size>},
		{"name", &SafeFunction<name>},
		{"type", &SafeFunction<type>},
		{"references", &SafeFunction<references>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ReferencesBinder::Register(state);
}

int MapFunctionBinder::address(lua_State *state)
{
	MapFunction *object = reinterpret_cast<MapFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int MapFunctionBinder::size(lua_State *state)
{
	MapFunction *object = reinterpret_cast<MapFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->end_address() ? static_cast<lua_Integer>(object->end_address() - object->address()) : 0);
	return 1;
}

int MapFunctionBinder::name(lua_State *state)
{
	MapFunction *object = reinterpret_cast<MapFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int MapFunctionBinder::type(lua_State *state)
{
	MapFunction *object = reinterpret_cast<MapFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int MapFunctionBinder::references(lua_State *state)
{
	MapFunction *object = reinterpret_cast<MapFunction *>(check_object(state, 1, class_name()));
	push_object(state, ReferencesBinder::class_name(), object->reference_list());
	return 1;
}

/**
 * ReferencesBinder
 */

void ReferencesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ReferenceBinder::Register(state);
}

int ReferencesBinder::item(lua_State *state)
{
	ReferenceList *object = reinterpret_cast<ReferenceList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, ReferenceBinder::class_name(), object->item(index));
	return 1;
}

int ReferencesBinder::count(lua_State *state)
{
	ReferenceList *object = reinterpret_cast<ReferenceList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int ReferencesBinder::clear(lua_State *state)
{
	ReferenceList *object = reinterpret_cast<ReferenceList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int ReferencesBinder::Delete(lua_State *state)
{
	ReferenceList *object = reinterpret_cast<ReferenceList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

/**
 * ReferenceBinder
 */

void ReferenceBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"operandAddress", &SafeFunction<operand_address>},
		{"tag", &SafeFunction<tag>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int ReferenceBinder::address(lua_State *state)
{
	Reference *object = reinterpret_cast<Reference *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int ReferenceBinder::operand_address(lua_State *state)
{
	Reference *object = reinterpret_cast<Reference *>(check_object(state, 1, class_name()));
	push_uint64(state, object->operand_address());
	return 1;
}

int ReferenceBinder::tag(lua_State *state)
{
	Reference *object = reinterpret_cast<Reference *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->tag());
	return 1;
}

/**
 * IntelFunctionsBinder
 */

void IntelFunctionsBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"clear", &SafeFunction<clear>},
		{"delete", &SafeFunction<Delete>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{"addByAddress", &SafeFunction<AddByAddress>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	IntelFunctionBinder::Register(state);
}

int IntelFunctionsBinder::item(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, IntelFunctionBinder::class_name(), object->item(index));
	return 1;
}

int IntelFunctionsBinder::count(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int IntelFunctionsBinder::clear(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int IntelFunctionsBinder::Delete(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int IntelFunctionsBinder::GetItemByAddress(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	push_object(state, IntelFunctionBinder::class_name(), object->GetFunctionByAddress(address));
	return 1;
}

int IntelFunctionsBinder::GetItemByName(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, IntelFunctionBinder::class_name(), object->GetFunctionByName(name));
	return 1;
}

int IntelFunctionsBinder::AddByAddress(lua_State *state)
{
	IntelFunctionList *object = reinterpret_cast<IntelFunctionList *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	int top = lua_gettop(state);
	CompilationType compilation_type = (top > 2) ? static_cast<CompilationType>(lua_tointeger(state, 3)) : ctVirtualization;
	bool need_compile = (top > 3) ? lua_toboolean(state, 4) == 1 : true;
	push_object(state, IntelFunctionBinder::class_name(), object->AddByAddress(address, compilation_type, 0, need_compile, NULL));
	return 1;
}

/**
 * IntelFunctionBinder
 */

void IntelFunctionBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"name", &SafeFunction<name>},
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"type", &SafeFunction<type>},
		{"compilationType", &SafeFunction<compilation_type>},
		{"setCompilationType", &SafeFunction<set_compilation_type>},
		{"lockToKey", &SafeFunction<lock_to_key>},
		{"setLockToKey", &SafeFunction<set_lock_to_key>},
		{"needCompile", &SafeFunction<need_compile>},
		{"setNeedCompile", &SafeFunction<set_need_compile>},
		{"links", &SafeFunction<links>},
		{"itemByAddress", &SafeFunction<GetItemByAddress>},
		{"indexOf", &SafeFunction<IndexOf>},
		{"destroy", &SafeFunction<destroy>},
		{"info", &SafeFunction<info>},
		{"ranges", &SafeFunction<ranges>},
		{"xproc", &SafeFunction<x_proc>},
		{"folder", &SafeFunction<folder>},
		{"setFolder", &SafeFunction<set_folder>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	IntelCommandBinder::Register(state);
	CompilationTypeBinder::Register(state);
	CommandLinksBinder::Register(state);
	FunctionInfoListBinder::Register(state);
}

int IntelFunctionBinder::address(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int IntelFunctionBinder::name(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int IntelFunctionBinder::item(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, IntelCommandBinder::class_name(), object->item(index));
	return 1;
}

int IntelFunctionBinder::count(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int IntelFunctionBinder::type(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int IntelFunctionBinder::compilation_type(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->compilation_type());
	return 1;
}

int IntelFunctionBinder::set_compilation_type(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	CompilationType compilation_type = static_cast<CompilationType>(check_integer(state, 2));
	object->set_compilation_type(compilation_type);
	return 0;
}

int IntelFunctionBinder::lock_to_key(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, (object->compilation_options() & coLockToKey) != 0);
	return 1;
}

int IntelFunctionBinder::set_lock_to_key(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	bool value = lua_toboolean(state, 2) == 1;
	uint32_t options = object->compilation_options();
	if (value) {
		options |= coLockToKey;
	} else {
		options &= ~coLockToKey;
	}
	object->set_compilation_options(options);
	return 0;
}

int IntelFunctionBinder::need_compile(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->need_compile());
	return 1;
}

int IntelFunctionBinder::set_need_compile(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	bool need_compile = lua_toboolean(state, 2) == 1;
	object->set_need_compile(need_compile);
	return 0;
}

int IntelFunctionBinder::links(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	push_object(state, CommandLinksBinder::class_name(), object->link_list());
	return 1;
}

int IntelFunctionBinder::GetItemByAddress(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	int top = lua_gettop(state);
	bool is_near = (top > 2) ? lua_toboolean(state, 3) == 1 : false;
	push_object(state, IntelCommandBinder::class_name(), is_near ? object->GetCommandByNearAddress(address) : object->GetCommandByAddress(address));
	return 1;
}

int IntelFunctionBinder::IndexOf(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	IntelCommand *item = reinterpret_cast<IntelCommand *>(check_object(state, 2, IntelCommandBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

int IntelFunctionBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

int IntelFunctionBinder::info(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	push_object(state, FunctionInfoListBinder::class_name(), object->function_info_list());
	return 1;
}

int IntelFunctionBinder::ranges(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	push_object(state, FunctionInfoBinder::class_name(), object->range_list());
	return 1;
}

int IntelFunctionBinder::x_proc(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	uint64_t address = check_uint64(state, 2);
	std::string value = luaL_checklstring(state, 3, NULL);
	object->AddWatermarkReference(address, value);
	return 0;
}

int IntelFunctionBinder::folder(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	push_object(state, FolderBinder::class_name(), object->folder());
	return 1;
}

int IntelFunctionBinder::set_folder(lua_State *state)
{
	IntelFunction *object = reinterpret_cast<IntelFunction *>(check_object(state, 1, class_name()));
	Folder *folder = reinterpret_cast<Folder *>(check_object(state, 1, FolderBinder::class_name()));
	object->set_folder(folder);
	return 0;
}

/**
 * CommandLinksBinder
 */

void CommandLinksBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	CommandLinkBinder::Register(state);
}

int CommandLinksBinder::item(lua_State *state)
{
	CommandLinkList *object = reinterpret_cast<CommandLinkList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, CommandLinkBinder::class_name(), object->item(index));
	return 1;
}

int CommandLinksBinder::count(lua_State *state)
{
	CommandLinkList *object = reinterpret_cast<CommandLinkList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

/**
 * AddressRangeBinder
 */

void AddressRangeBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"beginAddress", &SafeFunction<begin>},
		{"endAddress", &SafeFunction<end>},
		{"beginEntry", &SafeFunction<begin_entry>},
		{"endEntry", &SafeFunction<end_entry>},
		{"sizeEntry", &SafeFunction<size_entry>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int AddressRangeBinder::begin(lua_State *state)
{
	AddressRange *object = reinterpret_cast<AddressRange *>(check_object(state, 1, class_name()));
	push_uint64(state, object->begin());
	return 1;
}

int AddressRangeBinder::end(lua_State *state)
{
	AddressRange *object = reinterpret_cast<AddressRange *>(check_object(state, 1, class_name()));
	push_uint64(state, object->end());
	return 1;
}

int AddressRangeBinder::begin_entry(lua_State *state)
{
	AddressRange *object = reinterpret_cast<AddressRange *>(check_object(state, 1, class_name()));
	push_command(state, object->begin_entry());
	return 1;
}

int AddressRangeBinder::end_entry(lua_State *state)
{
	AddressRange *object = reinterpret_cast<AddressRange *>(check_object(state, 1, class_name()));
	push_command(state, object->end_entry());
	return 1;
}

int AddressRangeBinder::size_entry(lua_State *state)
{
	AddressRange *object = reinterpret_cast<AddressRange *>(check_object(state, 1, class_name()));
	push_command(state, object->size_entry());
	return 1;
}

/**
* UnwindOpcodesBinder
*/

void UnwindOpcodesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int UnwindOpcodesBinder::count(lua_State *state)
{
	std::vector<ICommand *> *object = reinterpret_cast<std::vector<ICommand *> *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size());
	return 1;
}

int UnwindOpcodesBinder::item(lua_State *state)
{
	std::vector<ICommand *> *object = reinterpret_cast<std::vector<ICommand *> *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_command(state, object->at(index));
	return 1;
}

/**
 * FunctionInfoBinder
 */

void FunctionInfoBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"beginAddress", &SafeFunction<begin>},
		{"endAddress", &SafeFunction<end>},
		{"baseType", &SafeFunction<base_type>},
		{"baseValue", &SafeFunction<base_value>},
		{"prologSize", &SafeFunction<prolog_size>},
		{"frameRegistr", &SafeFunction<frame_registr>},
		{"entry", &SafeFunction<entry>},
		{"dataEntry", &SafeFunction<data_entry>},
		{"unwindOpcodes", &SafeFunction<unwind_opcodes>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	AddressRangeBinder::Register(state);
	UnwindOpcodesBinder::Register(state);
}

int FunctionInfoBinder::item(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, AddressRangeBinder::class_name(), object->item(index));
	return 1;
}

int FunctionInfoBinder::count(lua_State *state)
{
	FunctionInfoList *object = reinterpret_cast<FunctionInfoList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FunctionInfoBinder::begin(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_uint64(state, object->begin());
	return 1;
}

int FunctionInfoBinder::end(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_uint64(state, object->end());
	return 1;
}

int FunctionInfoBinder::prolog_size(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->prolog_size());
	return 1;
}

int FunctionInfoBinder::frame_registr(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->frame_registr());
	return 1;
}

int FunctionInfoBinder::entry(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_command(state, object->entry());
	return 1;
}

int FunctionInfoBinder::data_entry(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_command(state, object->data_entry());
	return 1;
}

int FunctionInfoBinder::base_type(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->base_type());
	return 1;
}

int FunctionInfoBinder::base_value(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_uint64(state, object->base_value());
	return 1;
}

int FunctionInfoBinder::unwind_opcodes(lua_State *state)
{
	FunctionInfo *object = reinterpret_cast<FunctionInfo *>(check_object(state, 1, class_name()));
	push_object(state, UnwindOpcodesBinder::class_name(), object->unwind_opcodes());
	return 1;
}

/**
 * FunctionInfoListBinder
 */

void FunctionInfoListBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"indexOf", &SafeFunction<IndexOf>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	FunctionInfoBinder::Register(state);
}

int FunctionInfoListBinder::item(lua_State *state)
{
	FunctionInfoList *object = reinterpret_cast<FunctionInfoList *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FunctionInfoBinder::class_name(), object->item(index));
	return 1;
}

int FunctionInfoListBinder::count(lua_State *state)
{
	FunctionInfoList *object = reinterpret_cast<FunctionInfoList *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FunctionInfoListBinder::IndexOf(lua_State *state)
{
	FunctionInfoList *object = reinterpret_cast<FunctionInfoList *>(check_object(state, 1, class_name()));
	FunctionInfo *item = reinterpret_cast<FunctionInfo *>(check_object(state, 2, FunctionInfoBinder::class_name()));
	lua_pushinteger(state, object->IndexOf(item) + 1);
	return 1;
}

/**
 * CompilationTypeBinder
 */

void CompilationTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", ctNone},
		{"Virtualization", ctVirtualization},
		{"Mutation", ctMutation},
		{"Ultra", ctUltra},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * IntelCommandBinder
 */

void IntelCommandBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"address", &SafeFunction<address>},
		{"type", &SafeFunction<type>},
		{"text", &SafeFunction<text>},
		{"size", &SafeFunction<size>},
		{"dump", &SafeFunction<dump>},
		{"link", &SafeFunction<link>},
		{"flags", &SafeFunction<flags>},
		{"baseSegment", &SafeFunction<base_segment>},
		{"preffix", &SafeFunction<preffix>},
		{"operand", &SafeFunction<operand>},
		{"options", &SafeFunction<options>},
		{"alignment", &SafeFunction<alignment>},
		{"setDump", &SafeFunction<set_dump>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	IntelCommandTypeBinder::Register(state);
	CommandOptionBinder::Register(state);
	IntelOperandBinder::Register(state);
	IntelSegmentBinder::Register(state);
	IntelFlagBinder::Register(state);
	IntelRegistrBinder::Register(state);
}

int IntelCommandBinder::address(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	push_uint64(state, object->address());
	return 1;
}

int IntelCommandBinder::type(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int IntelCommandBinder::text(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	std::string text = object->text();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int IntelCommandBinder::size(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->dump_size());
	return 1;
}

int IntelCommandBinder::dump(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	lua_pushinteger(state, object->dump(index));
	return 1;
}

int IntelCommandBinder::link(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	push_object(state, CommandLinkBinder::class_name(), object->link());
	return 1;
}

int IntelCommandBinder::flags(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->flags());
	return 1;
}

int IntelCommandBinder::base_segment(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->base_segment());
	return 1;
}

int IntelCommandBinder::operand(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, IntelOperandBinder::class_name(), object->operand_ptr(index));
	return 1;
}

int IntelCommandBinder::preffix(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->preffix_command());
	return 1;
}

int IntelCommandBinder::options(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->options());
	return 1;
}

int IntelCommandBinder::alignment(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->alignment());
	return 1;
}

int IntelCommandBinder::set_dump(lua_State *state)
{
	IntelCommand *object = reinterpret_cast<IntelCommand *>(check_object(state, 1, class_name()));
	size_t l;
	const char *dump = luaL_checklstring(state, 2, &l);
	object->set_dump(dump, l);
	return 0;
}

/**
 * IntelOperandBinder
 */

void IntelOperandBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"type", &SafeFunction<type>},
		{"size", &SafeFunction<size>},
		{"registr", &SafeFunction<registr>},
		{"baseRegistr", &SafeFunction<base_registr>},
		{"scale", &SafeFunction<scale>},
		{"value", &SafeFunction<value>},
		{"addressSize", &SafeFunction<address_size>},
		{"valueSize", &SafeFunction<value_size>},
		{"fixup", &SafeFunction<fixup>},
		{"isLargeValue", &SafeFunction<is_large_value>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int IntelOperandBinder::type(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type);
	return 1;
}

int IntelOperandBinder::size(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->size);
	return 1;
}

int IntelOperandBinder::registr(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->registr);
	return 1;
}

int IntelOperandBinder::base_registr(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->base_registr);
	return 1;
}

int IntelOperandBinder::scale(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->scale_registr);
	return 1;
}

int IntelOperandBinder::value(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	push_uint64(state, object->value);
	return 1;
}

int IntelOperandBinder::address_size(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->address_size);
	return 1;
}

int IntelOperandBinder::value_size(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->value_size);
	return 1;
}

int IntelOperandBinder::fixup(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	push_object(state, PEFixupBinder::class_name(), object->fixup);
	return 1;
}

int IntelOperandBinder::is_large_value(lua_State *state)
{
	IntelOperand *object = reinterpret_cast<IntelOperand *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->is_large_value);
	return 1;
}

/**
 * IntelCommandTypeBinder
 */

void IntelCommandTypeBinder::Register(lua_State *state)
{
	EnumReg values[_countof(intel_command_name) + 1];
	memset(values, 0, sizeof(values));

	for (size_t i = 0; i < _countof(values) - 1; i++) {
		std::string str_name = (i == cmJmpWithFlag) ? "jxx" : intel_command_name[i];
		if (str_name.empty())
			continue;

		str_name[0] = toupper(str_name[0]);
		size_t size = str_name.size() + 1;
		char *name = new char[size];
		memcpy(name, str_name.c_str(), size);
		values[i].value = (int)i;
		values[i].name = name;
	}

	register_enum(state, enum_name(), values);

	for (size_t i = 0; i < _countof(values) - 1; i++) {
		delete [] values[i].name;
	}
}

/**
 * IntelSegmentBinder
 */

void IntelSegmentBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"es", segES},
		{"cs", segCS},
		{"ss", segSS},
		{"ds", segDS},
		{"fs", segFS},
		{"gs", segGS},
		{"None", segDefault},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * IntelFlagBinder
 */

void IntelFlagBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"C", fl_C},
		{"P", fl_P},
		{"A", fl_A},
		{"Z", fl_Z},
		{"S", fl_S},
		{"T", fl_T},
		{"I", fl_I},
		{"D", fl_D},
		{"O", fl_O},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * IntelRegistrBinder
 */

void IntelRegistrBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"eax", regEAX},
		{"ecx", regECX},
		{"edx", regEDX},
		{"ebx", regEBX},
		{"esp", regESP},
		{"ebp", regEBP},
		{"esi", regESI},
		{"edi", regEDI},
		{"r8", regR8},
		{"r9", regR9},
		{"r10", regR10},
		{"r11", regR11},
		{"r12", regR12},
		{"r13", regR13},
		{"r14", regR14},
		{"r15", regR15},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * CommandLinkBinder
 */

void CommandLinkBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"toAddress", &SafeFunction<to_address>},
		{"type", &SafeFunction<type>},
		{"from", &SafeFunction<from>},
		{"parent", &SafeFunction<parent>},
		{"operand", &SafeFunction<operand>},
		{"subValue", &SafeFunction<sub_value>},
		{"baseInfo", &SafeFunction<base_function_info>},
		{NULL, 0}
	};

	register_class(state, class_name(), methods);

	LinkTypeBinder::Register(state);
}

int CommandLinkBinder::to_address(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	push_uint64(state, object->to_address());
	return 1;
}

int CommandLinkBinder::type(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->type());
	return 1;
}

int CommandLinkBinder::from(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	push_command(state, object->from_command());
	return 1;
}

int CommandLinkBinder::parent(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	push_command(state, object->parent_command());
	return 1;
}

int CommandLinkBinder::operand(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->operand_index());
	return 1;
}

int CommandLinkBinder::sub_value(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	push_uint64(state, object->sub_value());
	return 1;
}

int CommandLinkBinder::base_function_info(lua_State *state)
{
	CommandLink *object = reinterpret_cast<CommandLink *>(check_object(state, 1, class_name()));
	push_object(state, FunctionInfoBinder::class_name(), object->base_function_info());
	return 1;
}

/**
 * LinkTypeBinder
 */

void LinkTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", ltNone},
		{"SEHBlock", ltSEHBlock},
		{"FinallyBlock", ltFinallyBlock},
		{"DualSEHBlock", ltDualSEHBlock},
		{"FilterSEHBlock", ltFilterSEHBlock},
		{"Jmp", ltJmp},
		{"JmpWithFlag", ltJmpWithFlag},
		{"JmpWithFlagNSFS", ltJmpWithFlagNSFS},
		{"JmpWithFlagNSNA", ltJmpWithFlagNSNA},
		{"JmpWithFlagNSNS", ltJmpWithFlagNSNS},
		{"Call", ltCall},
		{"Case", ltCase},
		{"Switch", ltSwitch},
		{"Native", ltNative},
		{"Offset", ltOffset},
		{"GateOffset", ltGateOffset},
		{"ExtSEHBlock", ltExtSEHBlock},
		{"MemSEHBlock", ltMemSEHBlock},
		{"ExtSEHHandler", ltExtSEHHandler},
		{"VBMemSEHBlock", ltVBMemSEHBlock},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * CoreBinder
 */

void CoreBinder::Register(lua_State *state)
{
	static const luaL_Reg lib_methods[] = {
		{"core", &SafeFunction<instance>},
		{"extractFilePath", &SafeFunction<extract_file_path>},
		{"extractFileName", &SafeFunction<extract_file_name>},
		{"extractFileExt", &SafeFunction<extract_file_ext>},
		{"expandEnvironmentVariables", &SafeFunction<expand_environment_variables>},
		{"setEnvironmentVariable", &SafeFunction<set_environment_variable>},
		{"commandLine", &SafeFunction<command_line>},
		{"openLib", &SafeFunction<open_lib>},
		{NULL, NULL}
	};

	static const luaL_Reg methods[] = {
		{"inputFile", &SafeFunction<input_file>},
		{"inputFileName", &SafeFunction<input_file_name>},
		{"outputFile", &SafeFunction<output_file>},
		{"outputFileName", &SafeFunction<output_file_name>},
		{"setOutputFileName", &SafeFunction<set_output_file_name>},
		{"projectFileName", &SafeFunction<project_file_name>},
		{"watermarks", &SafeFunction<watermarks>},
		{"watermarkName", &SafeFunction<watermark_name>},
		{"setWatermarkName", &SafeFunction<set_watermark_name>},
		{"options", &SafeFunction<options>},
		{"setOptions", &SafeFunction<set_options>},
		{"vmSectionName", &SafeFunction<vm_section_name>},
		{"setVMSectionName", &SafeFunction<set_vm_section_name>},
		{"saveProject", &SafeFunction<save_project>},
		{"inputArchitecture", &SafeFunction<input_architecture>},
		{"outputArchitecture", &SafeFunction<output_architecture>},
#ifdef ULTIMATE
		{"licenses", &SafeFunction<licenses>},
		{"files", &SafeFunction<files>},
#endif
		{NULL, NULL}
	};

	static const luaL_Reg g_methods[] = {
		{"print", &log_print},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	ProjectOptionBinder::Register(state);
#ifdef ULTIMATE
	LicensesBinder::Register(state);
	FilesBinder::Register(state);
#endif
	WatermarksBinder::Register(state);

    luaL_newlib(state, lib_methods);
    lua_setglobal(state, "vmprotect");

	lua_getglobal(state, "_G");
	luaL_setfuncs(state, g_methods, 0);
	lua_pop(state, 1);
}

int CoreBinder::instance(lua_State* state)
{
	push_object(state, class_name(), Script::core());
	return 1;
}

int CoreBinder::extract_file_path(lua_State *state)
{
	std::string text = os::ExtractFilePath(lua_tostring(state, 1));
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::extract_file_name(lua_State *state)
{
	std::string text = os::ExtractFileName(lua_tostring(state, 1));
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::extract_file_ext(lua_State *state)
{
	std::string text = os::ExtractFileExt(lua_tostring(state, 1));
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::expand_environment_variables(lua_State *state)
{
	std::string text = os::ExpandEnvironmentVariables(lua_tostring(state, 1));
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::set_environment_variable(lua_State *state)
{
	std::string name = luaL_checkstring(state, 1);
	std::string value = luaL_checkstring(state, 2);
	os::SetEnvironmentVariable(name.c_str(), value.c_str());
	return 0;
}

int CoreBinder::command_line(lua_State *state)
{
	std::vector<std::string> command_line = os::CommandLine();
	lua_newtable(state);
	for (size_t i = 0; i < command_line.size(); i++) {
		lua_pushinteger(state, i + 1);
		lua_pushlstring(state, command_line[i].c_str(), command_line[i].size());
		lua_settable(state, -3);
	}
	return 1;
}

int CoreBinder::input_file(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_file(state, object->input_file());
	return 1;
}

int CoreBinder::input_architecture(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_architecture(state, object->input_architecture());
	return 1;
}

int CoreBinder::input_file_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string text = object->input_file_name();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::output_file(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_file(state, object->output_file());
	return 1;
}

int CoreBinder::output_architecture(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_architecture(state, object->output_architecture());
	return 1;
}

int CoreBinder::output_file_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string text = object->absolute_output_file_name();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::set_output_file_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_output_file_name(name);
	return 0;
}

int CoreBinder::project_file_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string text = object->project_file_name();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

#ifdef ULTIMATE
int CoreBinder::licenses(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_object(state, LicensesBinder::class_name(), object->licensing_manager());
	return 1;
}

int CoreBinder::files(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_object(state, FilesBinder::class_name(), object->file_manager());
	return 1;
}
#endif

int CoreBinder::watermarks(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	push_object(state, WatermarksBinder::class_name(), object->watermark_manager());
	return 1;
}

int CoreBinder::watermark_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string text = object->watermark_name();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::set_watermark_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_watermark_name(name);
	return 0;
}

int CoreBinder::options(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->options());
	return 1;
}

int CoreBinder::set_options(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	uint32_t options = static_cast<uint32_t>(check_integer(state, 2));
	object->set_options(options);
	return 0;
}

int CoreBinder::vm_section_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string text = object->vm_section_name();
	lua_pushlstring(state, text.c_str(), text.size());
	return 1;
}

int CoreBinder::set_vm_section_name(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_vm_section_name(name);
	return 0;
}

int CoreBinder::save_project(lua_State *state)
{
	Core *object = reinterpret_cast<Core *>(check_object(state, 1, class_name()));
	object->Save();
	return 0;
}

/**
 * ProjectOptionBinder
 */

void ProjectOptionBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", 0},
		{"Pack", cpPack},
		{"ImportProtection", cpImportProtection},
		{"MemoryProtection", cpMemoryProtection},
		{"ResourceProtection", cpResourceProtection},
		{"CheckDebugger", cpCheckDebugger},
		{"CheckKernelDebugger", cpCheckKernelDebugger},
		{"CheckVirtualMachine", cpCheckVirtualMachine},
		{"StripFixups", cpStripFixups},
		{"StripDebugInfo", cpStripDebugInfo},
		{"DebugMode", cpDebugMode},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

#ifdef ULTIMATE

/**
 * LicensesBinder
 */

void LicensesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"delete", &SafeFunction<Delete>},
		{"clear", &SafeFunction<clear>},
		{"publicExp", &SafeFunction<public_exp>},
		{"privateExp", &SafeFunction<private_exp>},
		{"modulus", &SafeFunction<modulus>},
		{"keyLength", &SafeFunction<key_length>},
		{"hash", &SafeFunction<hash> },
		{"itemBySerialNumber", &SafeFunction<GetLicenseBySerialNumber>},
		{"importLicense", &SafeFunction<import_license>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	LicenseBinder::Register(state);
}

int LicensesBinder::item(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, LicenseBinder::class_name(), object->item(index));
	return 1;
}

int LicensesBinder::count(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int LicensesBinder::clear(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int LicensesBinder::Delete(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int LicensesBinder::public_exp(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string name = VectorToBase64(object->public_exp());
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicensesBinder::private_exp(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string name = VectorToBase64(object->private_exp());
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicensesBinder::modulus(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string name = VectorToBase64(object->modulus());
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicensesBinder::key_length(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->bits());
	return 1;
}

int LicensesBinder::hash(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string name = VectorToBase64(object->hash());
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicensesBinder::GetLicenseBySerialNumber(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string serial_number = luaL_checkstring(state, 2);
	push_object(state, LicenseBinder::class_name(), object->GetLicenseBySerialNumber(serial_number));
	return 1;
}

int LicensesBinder::import_license(lua_State *state)
{
	LicensingManager *object = reinterpret_cast<LicensingManager *>(check_object(state, 1, class_name()));
	std::string serial_number = luaL_checkstring(state, 2);
	LicenseInfo info;
	License *license = (object->DecryptSerialNumber(serial_number, info)) ? object->Add(0, info.CustomerName, info.CustomerEmail, "", "", serial_number, false) : NULL;
	push_object(state, LicenseBinder::class_name(), license);
	return 1;
}

/**
 * LicenseInfoBinder
 */

void LicenseInfoBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"flags", &SafeFunction<flags>},
		{"customerName", &SafeFunction<customer_name>},
		{"customerEmail", &SafeFunction<customer_email>},
		{"expireDate", &SafeFunction<expire_date>},
		{"hwid", &SafeFunction<hwid>},
		{"runningTimeLimit", &SafeFunction<running_time_limit>},
		{"maxBuildDate", &SafeFunction<max_build_date>},
		{"userData", &SafeFunction<user_data>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int LicenseInfoBinder::flags(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->Flags);
	return 1;
}

int LicenseInfoBinder::customer_name(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	std::string name = object->CustomerName;
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseInfoBinder::customer_email(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	std::string name = object->CustomerEmail;
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseInfoBinder::expire_date(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	push_date(state, object->ExpireDate, luaL_optlstring(state, 2, "%c", NULL));
	return 1;
}

int LicenseInfoBinder::hwid(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	std::string name = object->HWID;
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseInfoBinder::running_time_limit(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->RunningTimeLimit);
	return 1;
}

int LicenseInfoBinder::max_build_date(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	push_date(state, object->MaxBuildDate, luaL_optlstring(state, 2, "%c", NULL));
	return 1;
}

int LicenseInfoBinder::user_data(lua_State *state)
{
	LicenseInfo *object = reinterpret_cast<LicenseInfo *>(check_object(state, 1, class_name()));
	std::string name = object->UserData;
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

/**
 * LicenseBinder
 */

void LicenseBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"date", &SafeFunction<date>},
		{"customerName", &SafeFunction<customer_name>},
		{"customerEmail", &SafeFunction<customer_email>},
		{"orderRef", &SafeFunction<order_ref>},
		{"comments", &SafeFunction<comments>},
		{"serialNumber", &SafeFunction<serial_number>},
		{"blocked", &SafeFunction<blocked>},
		{"setBlocked", &SafeFunction<set_blocked>},
		{"info", &SafeFunction<info>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	LicenseInfoBinder::Register(state);
}

int LicenseBinder::date(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	push_date(state, object->date(), luaL_optlstring(state, 2, "%c", NULL));
	return 1;
}

int LicenseBinder::customer_name(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	std::string name = object->customer_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseBinder::customer_email(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	std::string name = object->customer_email();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseBinder::order_ref(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	std::string name = object->order_ref();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseBinder::comments(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	std::string name = object->comments();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseBinder::serial_number(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	std::string name = object->serial_number();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int LicenseBinder::blocked(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, object->blocked());
	return 1;
}

int LicenseBinder::set_blocked(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	object->set_blocked(lua_tointeger(state, 2) != 0);
	return 0;
}

int LicenseBinder::info(lua_State *state)
{
	License *object = reinterpret_cast<License *>(check_object(state, 1, class_name()));
	push_object(state, LicenseInfoBinder::class_name(), object->info());
	return 1;
}

/**
 * FileFoldersBinder
 */

void FileFoldersBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"add", &SafeFunction<add>},
		{"clear", &SafeFunction<clear>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	FileFolderBinder::Register(state);
}

int FileFoldersBinder::item(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FileFolderBinder::class_name(), object->item(index));
	return 1;
}

int FileFoldersBinder::count(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FileFoldersBinder::add(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	std::string name = lua_tostring(state, 2);
	push_object(state, FileFolderBinder::class_name(), object->Add(name));
	return 1;
}

int FileFoldersBinder::clear(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}
/**
 * FileFolderBinder
 */

void FileFolderBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"count", &SafeFunction<count>},
		{"item", &SafeFunction<item>},
		{"add", &SafeFunction<add>},
		{"clear", &SafeFunction<clear>},
		{"name", &SafeFunction<name>},
		{"destroy", &SafeFunction<destroy>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int FileFolderBinder::item(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FileFolderBinder::class_name(), object->item(index));
	return 1;
}

int FileFolderBinder::count(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FileFolderBinder::add(lua_State *state)
{
	Folder *object = reinterpret_cast<Folder *>(check_object(state, 1, class_name()));
	std::string name = lua_tostring(state, 2);
	push_object(state, FileFolderBinder::class_name(), object->Add(name));
	return 1;
}

int FileFolderBinder::clear(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int FileFolderBinder::name(lua_State *state)
{
	FileFolder *object = reinterpret_cast<FileFolder *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int FileFolderBinder::destroy(lua_State *state)
{
	delete_object(state, 1, class_name());
	return 0;
}

/**
* FileActionTypeBinder
*/

void FileActionTypeBinder::Register(lua_State *state)
{
	static const EnumReg values[] = {
		{"None", faNone},
		{"Load", faLoad},
		{"Register", faRegister},
		{"Install", faInstall},
		{NULL, 0}
	};

	register_enum(state, enum_name(), values);
}

/**
 * FilesBinder
 */

void FilesBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"delete", &SafeFunction<Delete>},
		{"clear", &SafeFunction<clear>},
		{"folders", &SafeFunction<folders>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	FileActionTypeBinder::Register(state);
	FileFoldersBinder::Register(state);
	FileBinder::Register(state);
}

int FilesBinder::item(lua_State *state)
{
	FileManager *object = reinterpret_cast<FileManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, FileBinder::class_name(), object->item(index));
	return 1;
}

int FilesBinder::count(lua_State *state)
{
	FileManager *object = reinterpret_cast<FileManager *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int FilesBinder::clear(lua_State *state)
{
	FileManager *object = reinterpret_cast<FileManager *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int FilesBinder::Delete(lua_State *state)
{
	FileManager *object = reinterpret_cast<FileManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int FilesBinder::folders(lua_State *state)
{
	FileManager *object = reinterpret_cast<FileManager *>(check_object(state, 1, class_name()));
	push_object(state, FileFoldersBinder::class_name(), object->folder_list());
	return 1;
}

/**
 * FileBinder
 */

void FileBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"fileName", &SafeFunction<file_name>},
		{"action", &SafeFunction<action>},
		{"folder", &SafeFunction<folder>},
		{"setName", &SafeFunction<set_name>},
		{"setFileName", &SafeFunction<set_file_name>},
		{"setAction", &SafeFunction<set_action>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int FileBinder::name(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int FileBinder::file_name(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	std::string name = object->file_name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int FileBinder::action(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	//std::string name = object->file_name();
	lua_pushinteger(state, object->action());
	return 1;
}

int FileBinder::folder(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	push_object(state, FileFolderBinder::class_name(), object->folder());
	return 1;
}

int FileBinder::set_name(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_name(name);
	return 0;
}

int FileBinder::set_file_name(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	std::string name = luaL_checkstring(state, 2);
	object->set_file_name(name);
	return 0;
}

int FileBinder::set_action(lua_State *state)
{
	InternalFile *object = reinterpret_cast<InternalFile *>(check_object(state, 1, class_name()));
	InternalFileAction action = static_cast<InternalFileAction>(check_integer(state, 2));
	object->set_action(action);
	return 0;
}

#endif

/**
 * WatermarksBinder
 */

void WatermarksBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"item", &SafeFunction<item>},
		{"count", &SafeFunction<count>},
		{"delete", &SafeFunction<Delete>},
		{"clear", &SafeFunction<clear>},
		{"itemByName", &SafeFunction<GetItemByName>},
		{"add", &SafeFunction<add>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);

	WatermarkBinder::Register(state);
}

int WatermarksBinder::item(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	push_object(state, WatermarkBinder::class_name(), object->item(index));
	return 1;
}

int WatermarksBinder::count(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	lua_pushinteger(state, object->count());
	return 1;
}

int WatermarksBinder::clear(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	object->clear();
	return 0;
}

int WatermarksBinder::Delete(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	size_t index = static_cast<size_t>(check_integer(state, 2) - 1);
	object->Delete(index);
	return 0;
}

int WatermarksBinder::GetItemByName(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	push_object(state, WatermarkBinder::class_name(), object->GetWatermarkByName(name));
	return 1;
}

int WatermarksBinder::add(lua_State *state)
{
	WatermarkManager *object = reinterpret_cast<WatermarkManager *>(check_object(state, 1, class_name()));
	std::string name = luaL_checklstring(state, 2, NULL);
	std::string value = (lua_gettop(state) > 2) ? luaL_checklstring(state, 3, NULL) : object->CreateValue();
	push_object(state, WatermarkBinder::class_name(), object->Add(name, value));
	return 1;
}

/**
 * WatermarkBinder
 */

void WatermarkBinder::Register(lua_State *state)
{
	static const luaL_Reg methods[] = {
		{"name", &SafeFunction<name>},
		{"value", &SafeFunction<value>},
		{"blocked", &SafeFunction<blocked>},
		{"setBlocked", &SafeFunction<set_blocked>},
		{NULL, NULL}
	};

	register_class(state, class_name(), methods);
}

int WatermarkBinder::name(lua_State *state)
{
	Watermark *object = reinterpret_cast<Watermark *>(check_object(state, 1, class_name()));
	std::string name = object->name();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int WatermarkBinder::value(lua_State *state)
{
	Watermark *object = reinterpret_cast<Watermark *>(check_object(state, 1, class_name()));
	std::string name = object->value();
	lua_pushlstring(state, name.c_str(), name.size());
	return 1;
}

int WatermarkBinder::blocked(lua_State *state)
{
	Watermark *object = reinterpret_cast<Watermark *>(check_object(state, 1, class_name()));
	lua_pushboolean(state, !object->enabled());
	return 1;
}

int WatermarkBinder::set_blocked(lua_State *state)
{
	Watermark *object = reinterpret_cast<Watermark *>(check_object(state, 1, class_name()));
	object->set_enabled(lua_tointeger(state, 2) == 0);
	return 0;
}

/**
 * Script
 */

Core *Script::core_ = NULL;

Script::Script(Core *owner)
	: state_(NULL), need_compile_(true) 
{
	core_ = owner;
};

Script::~Script()
{
	close();
}

void Script::close()
{
	if (state_) {
		lua_close(state_);
		state_ = NULL;
	}
}

bool Script::LoadFromFile(const std::string &file_name)
{
	FileStream file;
	if (!file.Open(file_name.c_str(), fmOpenRead | fmShareDenyNone))
		return false;

	need_compile_ = true;
	text_ = file.ReadAll();
	return true;
}

bool Script::Compile()
{
	close();

	if (!need_compile_)
		return true;

	state_ = luaL_newstate();
	luaL_openlibs(state_);

	Uint64Binder::Register(state_);
	OperandTypeBinder::Register(state_);
	OperandSizeBinder::Register(state_);
	ObjectTypeBinder::Register(state_);
	IntelFunctionsBinder::Register(state_);
	PEFileBinder::Register(state_);
	MacFileBinder::Register(state_);
	ELFFileBinder::Register(state_);
	CoreBinder::Register(state_);
	FFILibraryBinder::Register(state_);

	if (luaL_dostring(state_, text_.c_str()) != LUA_OK) {
		std::string txt = lua_tostring(state_, -1);
		for (size_t i = 0; i < txt.size(); i++) {
			if (txt[i] == '\r')
				txt[i] = ' ';
		}
		core_->Notify(mtError, this, txt);
		return false;
	}
	return true;
}

void Script::set_need_compile(bool need_compile)
{ 
	if (need_compile_ != need_compile) {
		need_compile_ = need_compile;
		core_->Notify(mtChanged, this);
	}
}

void Script::DoBeforeCompilation()
{
	ExecuteFunction("OnBeforeCompilation");
}

void Script::DoAfterCompilation()
{
	ExecuteFunction("OnAfterCompilation");
}

void Script::DoBeforeSaveFile()
{
	ExecuteFunction("OnBeforeSaveFile");
}

void Script::DoAfterSaveFile()
{
	ExecuteFunction("OnAfterSaveFile");
}

void Script::DoBeforePackFile()
{
	ExecuteFunction("OnBeforePackFile");
}

void Script::ExecuteFunction(const std::string &func_name)
{
	if (!need_compile_)
		return;

	int top = lua_gettop(state_);
	lua_getglobal(state_, func_name.c_str());
	if (lua_isfunction(state_, -1)) {
		if (lua_pcall(state_, 0, 0, 0) != LUA_OK) {
			std::string txt = lua_tostring(state_, -1);
			for (size_t i = 0; i < txt.size(); i++) {
				if (txt[i] == '\r')
					txt[i] = ' ';
			}
			throw std::runtime_error("Script error: " + txt);
		}
	}
	lua_settop(state_, top);
}