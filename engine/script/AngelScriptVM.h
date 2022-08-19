#pragma once

#include <string>
#include <vector>
#include <memory>
#include <angelscript.h>
#include "AngelScriptHelpers.h"

namespace chira {

class AngelScriptVM {
public:
    AngelScriptVM() = delete;
    static void init();

    [[nodiscard]] static asIScriptEngine* getScriptEngine() {
        return AngelScriptVM::asEngine;
    }

    static int registerGlobalFunction(auto f, const std::string& name) {
        return AngelScriptVM::registerGlobalFunction(f, name, asTypeString<decltype(f)>(name)());
    }
    static int registerGlobalFunction(auto f, const std::string& /*name*/, const std::string& decl) {
        return AngelScriptVM::asEngine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(f), asCALL_CDECL);
    }
private:
    static inline asIScriptEngine* asEngine = nullptr;
};

} // namespace chira
