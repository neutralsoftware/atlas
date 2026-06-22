#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include <Metal/Metal.hpp>

#define IMGUI_IMPL_METAL_CPP
#include <imgui.h>
#include <imgui_impl_metal.h>
#include <imgui_impl_sdl3.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Atlas Editor",
        1600,
        900,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_MetalView metalView = SDL_Metal_CreateView(window);
    if (!metalView) {
        SDL_Log("Failed to create Metal view: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL::CommandQueue* commandQueue = device->newCommandQueue();

    CA::MetalLayer* metalLayer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(metalView));

    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    metalLayer->setFramebufferOnly(true);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.TabRounding = 4.0f;

    ImGui_ImplSDL3_InitForMetal(window);

    ImGui_ImplMetal_Init(
        device
    );

    bool running = true;

    while (running) {
        NS::AutoreleasePool* framePool = NS::AutoreleasePool::alloc()->init();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        CA::MetalDrawable* drawable = metalLayer->nextDrawable();
        if (!drawable) {
            framePool->release();
        }

        MTL::RenderPassDescriptor* descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();

        auto* colorAttachment =
            descriptor->colorAttachments()->object(0);

        colorAttachment->setTexture(drawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setStoreAction(MTL::StoreActionStore);
        colorAttachment->setClearColor(
            MTL::ClearColor(0.08, 0.08, 0.09, 1.0)
        );

        MTL::CommandBuffer* commandBuffer =
            commandQueue->commandBuffer();

        ImGui_ImplMetal_NewFrame(
            descriptor
        );

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        ImGui::Begin("Atlas Editor");
        ImGui::Text("Hello from pure C++ + Metal-cpp + ImGui!");
        ImGui::End();

        ImGui::Render();

        MTL::RenderCommandEncoder* encoder =
            commandBuffer->renderCommandEncoder(descriptor);

        ImGui_ImplMetal_RenderDrawData(
            ImGui::GetDrawData(),
            commandBuffer,
            encoder
        );

        encoder->endEncoding();

        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();

        framePool->release();
    }

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    commandQueue->release();
    device->release();

    SDL_Metal_DestroyView(metalView);
    SDL_DestroyWindow(window);
    SDL_Quit();

    pool->release();

    return 0;
}
