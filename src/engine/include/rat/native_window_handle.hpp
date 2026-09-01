#pragma once

namespace rat {

// Opaque OS window for the renderer. nwh is HWND on Win32 and X11 Window on Linux;
// ndt is the X11 Display* (unused on Win32). rat_core never includes Win32/GLFW headers.
struct NativeWindowHandle {
  void* nwh = nullptr;
  void* ndt = nullptr;
};

}  // namespace rat
