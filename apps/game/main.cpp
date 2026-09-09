#include "launch_options.hpp"
#include "../platform/native_window.hpp"
#include "../platform/process.hpp"
#include "../platform/imgui_bgfx.hpp"
#include <expedition/session.hpp>
#include <rat/greybox.hpp>
#include <rat/renderer.hpp>
#include <rat/sprite_renderer.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
struct Ui {
  bool glfw=false, bgfx=false;
  std::vector<char> font;
  Ui() { ImGui::CreateContext(); ImGui::GetIO().IniFilename=nullptr; }
  ~Ui() { if(bgfx)rat::imgui_bgfx::shutdown(); if(glfw)ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); }
  void load_font(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file)throw std::runtime_error("Cannot load UI font: "+path.generic_string());
    font.assign(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    if(font.empty()||font.size()>16*1024*1024)throw std::runtime_error("Invalid UI font size");
    ImFontConfig config; config.FontDataOwnedByAtlas=false;
    auto& io=ImGui::GetIO();
    if(!io.Fonts->AddFontFromMemoryTTF(font.data(),static_cast<int>(font.size()),18,&config,io.Fonts->GetGlyphRangesCyrillic()))
      throw std::runtime_error("Cannot decode UI font");
  }
};
struct SceneTarget {
  bgfx::FrameBufferHandle handle=BGFX_INVALID_HANDLE;
  ~SceneTarget() { if(bgfx::isValid(handle))bgfx::destroy(handle); }
};
std::string utf8(const std::filesystem::path& path) { const auto u=path.u8string(); return {u.begin(),u.end()}; }
void ensure_parent(const std::filesystem::path& path) { if(path.has_parent_path())std::filesystem::create_directories(path.parent_path()); }
}
int main(int argc,char** argv) {
  try {
    const auto options=rat::expedition::parse_launch_options(rat::process_arguments(argc,argv),rat::executable_path());
    if(options.help) {
      std::cout<<"Rat Expedition prototype\nWASD: walk; Escape: pause/resume\n--data-dir PATH --width N --height N --frames N --hidden --screenshot PNG --report JSON\n--renderer auto|software-d3d11|software-opengl\n";
      return 0;
    }
    rat::expedition::ExpeditionSession game(rat::expedition::load_project(options.data_dir));
    rat::NativeWindow window;
    if(!window.create(options.width,options.height,"Rat Expedition - traversal prototype",options.hidden))throw std::runtime_error("Cannot create game window");
    glfwSetWindowSizeLimits(window.glfw_window(),640,360,GLFW_DONT_CARE,GLFW_DONT_CARE);
    rat::Renderer renderer;
    rat::RendererConfig config;
    config.window=window.handle(); config.width=options.width; config.height=options.height; config.vsync=options.frames==0;
    if(options.renderer=="software-d3d11")config.mode=rat::RendererMode::SoftwareD3D11;
    if(options.renderer=="software-opengl")config.mode=rat::RendererMode::SoftwareOpenGL;
    if(!renderer.init(config))throw std::runtime_error("Cannot initialize game renderer");
    rat::GreyboxScene scene;
    if(!scene.init())throw std::runtime_error("Cannot initialize scene renderer");
    scene.resize(640,360); scene.set_camera_mode(rat::CameraMode::ThreeQuarter);
    scene.set_focus(game.scene().camera_focus.x,game.scene().camera_focus.y,game.scene().camera_focus.z);
    scene.tick(rat::kCameraTurnSeconds);
    scene.set_player_visible(false); scene.set_terrain_map(game.scene().map); scene.set_blockers(game.scene().map.blockers);
    rat::SpriteRenderer sprites;
    std::string error;
    if(!sprites.init(game.project().sprite,error))throw std::runtime_error(error);
    SceneTarget target;
    const auto color=bgfx::createTexture2D(640,360,false,1,bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_RT|BGFX_SAMPLER_MIN_POINT|BGFX_SAMPLER_MAG_POINT|BGFX_SAMPLER_MIP_POINT|BGFX_SAMPLER_U_CLAMP|BGFX_SAMPLER_V_CLAMP);
    const auto depth=bgfx::createTexture2D(640,360,false,1,bgfx::TextureFormat::D24S8,BGFX_TEXTURE_RT_WRITE_ONLY);
    if(!bgfx::isValid(color)||!bgfx::isValid(depth)) {
      if(bgfx::isValid(color))bgfx::destroy(color); if(bgfx::isValid(depth))bgfx::destroy(depth);
      throw std::runtime_error("Cannot create 640x360 scene textures");
    }
    const bgfx::TextureHandle attachments[]{color,depth};
    target.handle=bgfx::createFrameBuffer(2,attachments,true);
    if(!bgfx::isValid(target.handle))throw std::runtime_error("Cannot create scene framebuffer");
    Ui ui;
    ui.load_font(rat::executable_path().parent_path()/"data/fonts/NotoSans-Regular.ttf");
    ui.glfw=ImGui_ImplGlfw_InitForOther(window.glfw_window(),true);
    if(!ui.glfw)throw std::runtime_error("Cannot initialize UI input");
    ui.bgfx=rat::imgui_bgfx::init();
    if(!ui.bgfx)throw std::runtime_error("Cannot initialize UI renderer");
    const auto* caps=bgfx::getCaps();
    std::cout<<"Renderer: "<<renderer.backend_name()<<" vendor="<<caps->vendorId<<" device="<<caps->deviceId<<"\n";
    unsigned frames=0; bool paused=false, previous_escape=false, capture_requested=false;
    float accumulator=0;
    double previous_time=window.time();
    std::vector<double> frame_ms;
    int old_width=options.width,old_height=options.height;
    while(!window.should_close()) {
      window.poll();
      const auto now=window.time();
      const float dt=static_cast<float>(std::clamp(now-previous_time,0.0,0.2)); previous_time=now;
      const auto input=window.sample_frame_input();
      if(input.close_requested)break;
      if(input.framebuffer_width<1||input.framebuffer_height<1) { accumulator=0; std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
      if(input.framebuffer_width!=old_width||input.framebuffer_height!=old_height) {
        old_width=input.framebuffer_width; old_height=input.framebuffer_height; renderer.resize(old_width,old_height);
      }
      const bool escape=input.keys[GLFW_KEY_ESCAPE];
      if(escape&&!previous_escape)paused=!paused;
      previous_escape=escape;
      if(!input.focused&&!options.hidden)paused=true;
      if(paused)accumulator=0;
      else {
        accumulator+=options.frames?1.0f/60.0f:dt;
        rat::expedition::TraversalInput move;
        move.screen_x=float(input.keys[GLFW_KEY_D])-float(input.keys[GLFW_KEY_A]);
        move.screen_y=float(input.keys[GLFW_KEY_S])-float(input.keys[GLFW_KEY_W]);
        while(accumulator>=rat::kSimulationFixedDt) { game.tick(move); accumulator-=rat::kSimulationFixedDt; }
      }
      renderer.begin_frame();
      bgfx::setViewFrameBuffer(0,target.handle);
      scene.draw(0);
      const auto snapshot=game.snapshot();
      sprites.draw(scene.camera(),snapshot.leader,snapshot.direction,snapshot.frame);
      ImGui_ImplGlfw_NewFrame(); rat::imgui_bgfx::begin_frame();
      const auto display=ImGui::GetIO().DisplaySize;
      const float scale=float(std::max(1,std::min(input.framebuffer_width/640,input.framebuffer_height/360)));
      const float ratio_x=display.x/float(input.framebuffer_width),ratio_y=display.y/float(input.framebuffer_height);
      const ImVec2 size{640*scale*ratio_x,360*scale*ratio_y};
      const ImVec2 offset{(display.x-size.x)/2,(display.y-size.y)/2};
      auto* background=ImGui::GetBackgroundDrawList();
      background->AddRectFilled({0,0},display,IM_COL32(10,11,16,255));
      const bool flip=caps->originBottomLeft;
      background->AddImage(static_cast<ImTextureID>(color.idx),offset,{offset.x+size.x,offset.y+size.y}, {0,flip?1.0f:0.0f},{1,flip?0.0f:1.0f});
      ImGui::SetNextWindowPos({16,16}); ImGui::SetNextWindowBgAlpha(.82f);
      ImGui::Begin("Expedition",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove);
      ImGui::TextUnformatted("Экспедиция · Серый двор");
      ImGui::TextUnformatted("WASD — движение    Esc — пауза");
      if(paused) { ImGui::Separator(); ImGui::TextUnformatted("Пауза · Esc — продолжить"); }
      ImGui::End();
      rat::imgui_bgfx::end_frame();
      ++frames;
      if(options.frames&&frames>=options.frames&&!capture_requested&&!options.screenshot.empty()) {
        ensure_parent(options.screenshot); renderer.request_capture(utf8(options.screenshot)); capture_requested=true;
      }
      renderer.end_frame();
      frame_ms.push_back((window.time()-now)*1000);
      if(options.frames&&frames>=options.frames) {
        if(options.screenshot.empty())break;
        const auto capture=renderer.capture_result();
        if(capture.complete) { if(!capture.ok)throw std::runtime_error(capture.error); break; }
        if(frames>options.frames+240)throw std::runtime_error("Screenshot capture timed out");
      }
    }
    if(capture_requested&&!renderer.capture_result().ok)throw std::runtime_error("Screenshot did not complete");
    if(!options.report.empty()) {
      ensure_parent(options.report);
      const auto snapshot=game.snapshot();
      std::sort(frame_ms.begin(),frame_ms.end());
      nlohmann::json report={{"schema_version",1},{"slice","P1.1"},{"backend",renderer.backend_name()},
        {"vendor_id",caps->vendorId},{"device_id",caps->deviceId},{"frames",frames},{"simulation_ticks",snapshot.tick},
        {"scene",snapshot.scene_id},{"position",{{"x",snapshot.leader.x},{"y",snapshot.leader.y},{"z",snapshot.leader.z}}},
        {"internal_size",{640,360}},{"window_size",{old_width,old_height}},{"screenshot",utf8(options.screenshot)},
        {"hidden",options.hidden},{"frame_cpu_ms_median",frame_ms.empty()?0:frame_ms[frame_ms.size()/2]},
        {"frame_cpu_ms_p95",frame_ms.empty()?0:frame_ms[std::min(frame_ms.size()-1,frame_ms.size()*95/100)]}};
      std::ofstream output(options.report,std::ios::binary);
      if(!output||!(output<<report.dump(2)<<'\n'))throw std::runtime_error("Cannot write report");
    }
    return 0;
  } catch(const std::exception& ex) { std::cerr<<"Rat Expedition: "<<ex.what()<<'\n'; return 1; }
}
