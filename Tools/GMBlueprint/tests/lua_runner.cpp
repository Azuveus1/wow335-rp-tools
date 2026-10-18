#include <cstdio>
#include <cstdint>
#include <chrono>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
// Standalone equivalents of the OS timing/validation hooks used by this Lua.
int64_t OsGetAsyncClocksPerSecond() { return 1000000000; }
uint64_t OsGetAsyncTimeClocks() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
void OsValidateFunctionPointer(const void*) {}
static int TestDoFile(lua_State* state) {
    const char* path = luaL_checkstring(state, 1);
    if (luaL_dofile(state, path)) return lua_error(state);
    return 0;
}
static int TestPrint(lua_State* state) {
    std::puts(luaL_checkstring(state, 1));
    return 0;
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    lua_State* state = luaL_newstate();
    luaL_openlibs(state);
    // The game removes file I/O from base Lua; enable it only in this test runner.
    lua_register(state, "dofile", TestDoFile);
    lua_register(state, "print", TestPrint);
    const int result = luaL_dofile(state, argv[1]);
    if (result) std::fprintf(stderr, "%s\n", lua_tostring(state, -1));
    lua_close(state);
    return result ? 1 : 0;
}
