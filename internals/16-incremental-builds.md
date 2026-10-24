# Incremental editor builds

`just editor` checks cached configuration options, then builds the AtlasEditor target with Ninja. Matching options must not force a configure step. CMake still regenerates when build inputs change.

Keep revision-dependent compiler definitions on the source files that consume them. A directory-wide `ATLAS_BUILD_REVISION` changed dependency compile commands after commits and CMake regeneration. The inspected Windows compilation database contained 2,311 affected commands, including 1,247 Slang, 363 Assimp, and 43 FreeType entries. Only `editor/main.cpp` and `runtime/lib/c_api.cpp` consume this definition.

Removing the global definition changes existing compile commands once, so the first build after this fix can still rebuild dependencies. Subsequent revision changes affect only the two consumers and their downstream links. Compiler, build-type, backend, dependency source, or shared header changes can still legitimately rebuild dependencies.

The editor source glob includes C++ sources and headers; assets have explicit Qt resource dependencies. The theme packer preserves the generated header timestamp when its content is unchanged.

Run `python -m unittest discover -s scripts -p 'test_*.py'` for configuration-cache, CMake revision-scope, and generated-theme checks. The CMake fixture uses no compiled languages. These checks do not replace a real incremental build measurement; Atlas was not built under the no-build instruction.
