#include "atlas/runtime/scripting.h"
#include <string>

char *runtime::scripting::normalizeModuleName(JSContext *ctx,
                                              const char *baseName,
                                              const char *name, void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);
    const std::string base = baseName == nullptr ? "" : baseName;
    const std::string module = name == nullptr ? "" : name;

    if (host->modules.contains(module)) {
        return js_strdup(ctx, module.c_str());
    }

    if (!module.empty() && module[0] == '.') {
        auto slash = base.rfind('/');
        std::string dir =
            (slash == std::string::npos) ? "" : base.substr(0, slash + 1);
        std::string resolved = dir + module;

        while (true) {
            auto pos = resolved.find("/./");
            if (pos == std::string::npos) {
                break;
            }
            resolved.replace(pos, 3, "/");
        }

        while (true) {
            auto pos = resolved.find("../");
            if (pos == std::string::npos) {
                break;
            }
            auto prev = resolved.rfind('/', pos > 1 ? pos - 2 : 0);
            if (prev == std::string::npos) {
                break;
            }
            auto next = resolved.find('/', pos + 2);
            resolved.erase(
                prev + 1,
                (next == std::string::npos ? resolved.size() : next + 1) -
                    (prev + 1));
        }

        if (host->modules.contains(resolved)) {
            return js_strdup(ctx, resolved.c_str());
        }
    }

    JS_ThrowReferenceError(ctx, "Could not resolve module '%s' from '%s'",
                           module.c_str(),
                           base.empty() ? "<root>" : base.c_str());
    return nullptr;
}

JSModuleDef *runtime::scripting::loadModule(JSContext *ctx,
                                            const char *module_name,
                                            void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);

    auto it = host->modules.find(module_name);
    if (it == host->modules.end()) {
        JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
        return nullptr;
    }

    const std::string &source = it->second;

    JSValue func_val = JS_Eval(ctx, source.c_str(), source.size(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func_val)) {
        return nullptr;
    }

    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val));
    JS_FreeValue(ctx, func_val);
    return m;
}

bool runtime::scripting::evalModule(JSContext *ctx, const std::string &name,
                                    const std::string &src) {
    JSValue compiled = JS_Eval(ctx, src.c_str(), src.length(), name.c_str(),
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!checkNotException(ctx, compiled, "compile module")) {
        return false;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (!checkNotException(ctx, result, "execute module")) {
        return false;
    }

    JS_FreeValue(ctx, result);
    return true;
}

JSValue
runtime::scripting::importModuleNamespace(JSContext *ctx,
                                          const std::string &module_name) {
    std::string src = "import * as ns from '" + module_name +
                      "';\n"
                      "globalThis.__atlas_tmp_ns = ns;\n";

    JSValue compiled = JS_Eval(ctx, src.c_str(), src.size(), "<import_ns>",
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) {
        return JS_EXCEPTION;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) {
        return JS_EXCEPTION;
    }
    JS_FreeValue(ctx, result);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ns = JS_GetPropertyStr(ctx, global, "__atlas_tmp_ns");
    JSAtom atom = JS_NewAtom(ctx, "__atlas_tmp_ns");
    JS_DeleteProperty(ctx, global, atom, 0);
    JS_FreeAtom(ctx, atom);
    JS_FreeValue(ctx, global);
    return ns;
}
