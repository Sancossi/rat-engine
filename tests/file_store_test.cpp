#include <rat/file_store.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>
#include <rat/native_window_handle.hpp>
#include <rat/simulation_session.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <system_error>

#if defined(HWND) || defined(_WINDOWS_) || defined(__glfw3_h__) || defined(GLFW_TRUE)
#error rat_core Clock/FileStore/SimulationSession must not see HWND, Windows.h, or GLFW
#endif

namespace {

constexpr const char* kMiniMap = R"({
  "schema_version": 1,
  "id": "mini",
  "width": 2,
  "height": 2,
  "events": []
})";

}  // namespace

TEST_CASE("MemoryFileStore round-trips FileBytes by path", "[unit][file]") {
  rat::MemoryFileStore files;
  const rat::FileWriteResult written = files.write("maps/mini.json", kMiniMap);
  REQUIRE(written.ok);

  const rat::FileReadResult read = files.read("maps/mini.json");
  REQUIRE(read.ok);
  CHECK(read.bytes.as_text() == kMiniMap);
}

TEST_CASE("MemoryFileStore read of a missing path fails", "[unit][file]") {
  rat::MemoryFileStore files;
  const rat::FileReadResult read = files.read("missing.json");
  CHECK_FALSE(read.ok);
  CHECK_FALSE(read.error.empty());
}

TEST_CASE("load_map_from_file reads through FileStore without touching the OS path", "[unit][file]") {
  rat::MemoryFileStore files;
  REQUIRE(files.write("virtual/mini.json", kMiniMap).ok);

  const rat::MapLoadResult loaded = rat::load_map_from_file("virtual/mini.json", files);
  REQUIRE(loaded.ok);
  CHECK(loaded.map.id == "mini");
  CHECK(loaded.map.width == 2);
}

TEST_CASE("save_map_to_file writes through FileStore", "[unit][file]") {
  rat::MemoryFileStore files;
  REQUIRE(files.write("virtual/mini.json", kMiniMap).ok);
  const rat::MapLoadResult loaded = rat::load_map_from_file("virtual/mini.json", files);
  REQUIRE(loaded.ok);

  const rat::MapFileResult saved = rat::save_map_to_file(loaded.map, "virtual/out.json", files);
  REQUIRE(saved.ok);

  const rat::FileReadResult roundtrip = files.read("virtual/out.json");
  REQUIRE(roundtrip.ok);
  const rat::MapLoadResult again = rat::load_map_from_string(roundtrip.bytes.as_text());
  REQUIRE(again.ok);
  CHECK(again.map.id == "mini");
}

TEST_CASE("load_map_document_from_file uses FileStore", "[unit][file]") {
  rat::MemoryFileStore files;
  REQUIRE(files.write("doc.json", kMiniMap).ok);
  const rat::MapDocumentLoadResult loaded = rat::load_map_document_from_file("doc.json", files);
  REQUIRE(loaded.ok);
  CHECK(loaded.document.data().id == "mini");
}

TEST_CASE("OsFileStore writes and reads a real temp file", "[unit][file]") {
  const auto path = std::filesystem::temp_directory_path() / "rat-os-file-store.txt";
  std::error_code ec;
  std::filesystem::remove(path, ec);

  rat::OsFileStore files;
  REQUIRE(files.write(path.string(), "hello").ok);
  const rat::FileReadResult read = files.read(path.string());
  REQUIRE(read.ok);
  CHECK(read.bytes.as_text() == "hello");

  std::filesystem::remove(path, ec);
}

TEST_CASE("SimulationSession tick consumes InputFrame not a window type", "[unit][platform]") {
  rat::SimulationSession session;
  rat::InputFrame frame{};
  frame.jump_held = true;
  const rat::SimulationTickResult result = session.tick(frame);
  CHECK(result.tick_id == 1);
}

TEST_CASE("NativeWindowHandle is opaque void pointers", "[unit][platform]") {
  rat::NativeWindowHandle handle;
  CHECK(handle.nwh == nullptr);
  CHECK(handle.ndt == nullptr);
}
