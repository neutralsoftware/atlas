from pathlib import Path
import subprocess
import argparse
import shutil
import tempfile
parser = argparse.ArgumentParser()
parser.add_argument('artifacts', type=Path)
parser.add_argument('--spirv-cross-prefix', type=Path, default=Path(shutil.which('spirv-cross')).parent.parent)
arguments = parser.parse_args()
root = Path(__file__).resolve().parent.parent
workspace = tempfile.TemporaryDirectory(prefix='atlas-shader-contracts-')
tmp = Path(workspace.name)
cpp=(root/'opal/src/metal/metal_state.cpp').read_text();header=(root/'opal/src/metal/metal_state.h').read_text();opal=(root/'opal/include/opal/opal.h').read_text();shader=(root/'opal/src/shaders.cpp').read_text()
includes='''#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <functional>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <spirv_cross/spirv_cross.hpp>
using VkDescriptorType = int;
constexpr int VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1;
constexpr int VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2;
constexpr int VK_DESCRIPTOR_TYPE_SAMPLER = 3;
constexpr int VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 4;
'''
metal=header[header.index('enum class MetalProgramStage'):header.index('struct ContextState')]
metal+='''enum class TextureType { Texture2D, TextureCubeMap, Texture3D, Texture2DArray };
struct ProgramState {
void *vertexFunction = reinterpret_cast<void *>(1);
void *fragmentFunction = reinterpret_cast<void *>(1);
void *computeFunction = reinterpret_cast<void *>(1);
bool computeProgram = false;
std::unordered_map<std::string, StructLayout> layouts;
std::vector<BufferBinding> bindings;
std::unordered_map<uint32_t, size_t> bindingSize;
std::unordered_map<std::string, int> textureBindings;
std::unordered_map<int, TextureType> textureTypesByBinding;
std::unordered_map<std::string, std::vector<UniformLocation>> uniformResolutionCache;
};
'''
metal+=cpp[cpp.index('template <typename T> inline T alignUp'):cpp.index('template <typename T> inline T enumOr')]
metal+=cpp[cpp.index('uint32_t stageBindingKey('):cpp.index('MTL::PixelFormat textureFormatToPixelFormat')]
metal+=cpp[cpp.index('bool parseProgramLayouts('):cpp.index('std::string makePipelineKey(')]
ubo=opal[opal.index('struct UniformBindingInfo {'):opal.index('};',opal.index('struct UniformBindingInfo {'))+2]
vulkan=ubo+'''\nstruct Shader {
std::vector<uint32_t> spirvBytecode;
std::unordered_map<std::string, UniformBindingInfo> uniformBindings;
void performReflection();
};\n'''+shader[shader.index('void Shader::performReflection()'):shader.index('const UniformBindingInfo *\nShaderProgram::findUniform')]
main=r'''
std::string read(const std::string &p) { std::ifstream f(p); return {std::istreambuf_iterator<char>(f), {}}; }
int main(int argc, char **argv) {
 const std::string root = argv[1];
 const std::vector<std::pair<std::string, std::vector<std::string>>> checks = {
  {"DEFERRED_VERT", {"model", "view", "projection", "isInstanced"}},
  {"DEFERRED_FRAG", {"textureTypes[0]", "textureTypes[15]", "textureCount", "cameraPosition", "normalMapStrength", "useNormalMap", "textureScale", "textureOffset", "material.albedo", "material.metallic"}},
  {"LIGHT_FRAG", {"cameraPosition", "useIBL", "directionalLightCount", "pointLightCount", "environment.rimLightColor", "ambientLight.color", "ps.origin"}},
  {"GAUSSIAN_FRAG", {"weight[0]", "weight[4]", "horizontal", "radius"}},
  {"SSAO_FRAG", {"samples[0]", "samples[63]", "projection"}},
  {"POINT_DEPTH_NOGEOM_VERT", {"model", "isInstanced", "shadowMatrix"}},
  {"POINT_DEPTH_NOGEOM_FRAG", {"lightPos", "far_plane"}},
  {"FULLSCREEN_FRAG", {"environment.fogColor", "environment.fogIntensity", "hasBrightTexture", "projectionMatrix", "invProjectionMatrix", "viewMatrix", "deltaTime"}},
  {"PATH_DENOISE", {"stepWidth", "bloomThreshold"}}
 };
 int count=0;
 for (const auto &[symbol, names] : checks) {
  auto bytes=read(root+"/"+symbol+".spv");
  Shader shader; shader.spirvBytecode.resize(bytes.size()/4); std::memcpy(shader.spirvBytecode.data(),bytes.data(),bytes.size()); shader.performReflection();
  ProgramState state;
  if(symbol=="PATH_DENOISE") parseComputeProgramLayouts(read(root+"/"+symbol+".metal"),state);
  else parseProgramLayouts(symbol.ends_with("VERT")?read(root+"/"+symbol+".metal"):"",symbol.ends_with("FRAG")?read(root+"/"+symbol+".metal"):"",state);
  for(const auto &name:names) {
   auto found=shader.uniformBindings.find(name);
   auto locations=resolveUniformLocations(state,name);
   if(found==shader.uniformBindings.end() || locations.empty()) {std::cerr<<symbol<<" missing "<<name<<" Vulkan="<<(found!=shader.uniformBindings.end())<<" Metal="<<locations.size()<<"\n"; return 1;}
   if(found->second.offset!=locations.front().offset){std::cerr<<symbol<<" offset "<<name<<" Vulkan="<<found->second.offset<<" Metal="<<locations.front().offset<<"\n";return 1;}
   ++count;
  }
  if(symbol=="LIGHT_FRAG") for(auto name:{"DirectionalLights","PointLights","SpotLights","AreaLights","ShadowParams"}) {
   if(!shader.uniformBindings.contains(name)||resolveBufferBindings(state,name).empty()) {std::cerr<<"missing storage buffer "<<name<<"\n";return 1;}
   ++count;
  }
 }
 std::cout<<count<<" uniform and buffer contracts pass in the actual Metal and Vulkan reflection implementations.\n";
}
'''
source=includes+'\n#include <cstring>\n'+metal+'\n'+vulkan+main;(tmp/'reflection.cpp').write_text(source)
subprocess.run(['clang++','-std=c++20','-I'+str(arguments.spirv_cross_prefix/'include'),str(tmp/'reflection.cpp'),str(arguments.spirv_cross_prefix/'lib/libspirv-cross-core.a'),'-o',str(tmp/'reflection')],check=True)
subprocess.run([str(tmp/'reflection'),str(arguments.artifacts)],check=True)
