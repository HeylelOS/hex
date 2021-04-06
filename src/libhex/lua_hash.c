#include "hex/lua.h"

#include <stdint.h>
#include <stdlib.h>

#define HEX_HASH_METATABLE_NAME "Hex.hash"

/* Hash is implemented with an FNV-1a hash */

#if UINTPTR_MAX == UINT64_MAX
#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME        0x00000100000001B3
#elif UINTPTR_MAX == UINT32_MAX
#define FNV_OFFSET_BASIS 0x811c9dc5
#define FNV_PRIME        0x01000193
#else
#error "Unsupported hash size"
#endif

#define hash_new() FNV_OFFSET_BASIS

typedef uintptr_t hash_t;

static hash_t
hash_combine(hash_t hash, const void *data, size_t count) {
	const uint8_t *current = data;
	const uint8_t * const end = data + count;

	while (current != end) {

		hash = (hash ^ *current) * FNV_PRIME;

		current++;
	}

	return hash;
}

static int
lua_hash_new(lua_State *L) {
	const int top = lua_gettop(L);
	hash_t hash = hash_new();

	for (int i = 1; i <= top; i++) {
		size_t length;
		const char * const string = luaL_checklstring(L, i, &length);

		hash = hash_combine(hash, string, length);
	}

	lua_pushlightuserdata(L, (void *)hash);
	luaL_getmetatable(L, HEX_HASH_METATABLE_NAME);
	lua_setmetatable(L, -2);

	return 1;
}

static int
lua_hash_combine(lua_State *L) {
	const int top = lua_gettop(L);
	hash_t hash = (hash_t)luaL_checkudata(L, 1, HEX_HASH_METATABLE_NAME);

	for (int i = 2; i <= top; i++) {
		size_t length;
		const char * const string = luaL_checklstring(L, i, &length);

		hash = hash_combine(hash, string, length);
	}

	lua_pushlightuserdata(L, (void *)hash);

	return 1;
}

static int
lua_hash_tostring(lua_State *L) {
	hash_t hash = (hash_t)luaL_checkudata(L, 1, HEX_HASH_METATABLE_NAME);
	char string[sizeof (hash) * 2 + 1]; /* Hexadecimal with nul terminator */

	string[sizeof (string) - 1] = '\0';

	for (int nibble = sizeof (hash) * 2 - 1; nibble >= 0; nibble--) {
		const int value = hash & 0xF;

		if (value < 0xA) {
			string[nibble] = '0' + value;
		} else {
			string[nibble] = 'A' + value - 10;
		}

		hash >>= 4;
	}

	lua_pushstring(L, string);

	return 1;
}

static const luaL_Reg hash_metatable_funcs[] = {
	{ "__tostring", lua_hash_tostring },
	{ "__concat",   lua_hash_combine },
	{ NULL, NULL }
};

static const luaL_Reg hash_funcs[] = {
	{ "new",      lua_hash_new },
	{ "combine",  lua_hash_combine },
	{ "tostring", lua_hash_tostring },
	{ NULL, NULL }
};

int
luaopen_hash(lua_State *L) {

	luaL_newmetatable(L, HEX_HASH_METATABLE_NAME);
	luaL_setfuncs(L, hash_metatable_funcs, 0);

	luaL_newlib(L, hash_funcs);

	return 1;
}
