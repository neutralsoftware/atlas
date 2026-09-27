#include "atlas/runtime/scripting.h"
#include <iostream>

ScriptInstance::~ScriptInstance() {
    if (ctx && !JS_IsUndefined(instance)) {
        JS_FreeValue(ctx, instance);
    }
}

ScriptInstance *runtime::scripting::createScriptInstance(
    JSContext *ctx, const std::string &entryModuleName,
    const std::string &scriptPath, const std::string &className) {
    JSValue ns = importModuleNamespace(ctx, entryModuleName);
    if (JS_IsException(ns)) {
        dumpExecution(ctx);
        return nullptr;
    }

    JSValue scriptExports = JS_UNDEFINED;
    if (scriptPath.empty()) {
        scriptExports = ns;
    } else {
        JSValue atlasScripts = JS_GetPropertyStr(ctx, ns, "default");
        JS_FreeValue(ctx, ns);
        if (JS_IsException(atlasScripts)) {
            dumpExecution(ctx);
            return nullptr;
        }

        scriptExports =
            JS_GetPropertyStr(ctx, atlasScripts, scriptPath.c_str());
        JS_FreeValue(ctx, atlasScripts);
        if (JS_IsException(scriptExports)) {
            dumpExecution(ctx);
            return nullptr;
        }

        if (JS_IsUndefined(scriptExports)) {
            std::cerr << "Script exports not found for path: " << scriptPath
                      << "\n";
            JS_FreeValue(ctx, scriptExports);
            return nullptr;
        }
    }

    JSValue constructor =
        JS_GetPropertyStr(ctx, scriptExports, className.c_str());
    JS_FreeValue(ctx, scriptExports);
    if (JS_IsException(constructor)) {
        dumpExecution(ctx);
        return nullptr;
    }

    if (!JS_IsFunction(ctx, constructor)) {
        std::cerr << "Export '" << className
                  << "' is not a constructor/function\n";
        JS_FreeValue(ctx, constructor);
        return nullptr;
    }

    JSValue object = JS_CallConstructor(ctx, constructor, 0, nullptr);
    JS_FreeValue(ctx, constructor);
    if (JS_IsException(object)) {
        dumpExecution(ctx);
        return nullptr;
    }

    auto *instance = new ScriptInstance{};
    instance->ctx = ctx;
    instance->instance = object;
    return instance;
}
