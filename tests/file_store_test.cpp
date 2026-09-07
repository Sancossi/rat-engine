#include <rat/file_store.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>
#include <rat/native_window_handle.hpp>
#include <rat/save_game.hpp>
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

namespace {
class FaultOps final : public rat::AtomicFileOps {
 public:
  int fail_at = 0;
  int step = 0;
  bool temp_exists = false;
  bool closed = false;
  std::string main = "previous";
  std::string backup = "older";
  std::string temp;
  rat::FileWriteResult next() { return {++step != fail_at, "injected I/O failure"}; }
  rat::FileWriteResult create_temp(std::string_view) override {
    auto result = next();
    temp_exists = result.ok;
    return result;
  }
  rat::FileWriteResult write_temp(std::string_view bytes) override {
    REQUIRE(temp_exists);
    auto result = next();
    temp = result.ok ? std::string(bytes) : std::string(bytes.substr(0, 2));
    return result;
  }
  rat::FileWriteResult finish_temp() override { closed = true; return next(); }
  rat::FileWriteResult backup_existing(std::string_view) override {
    REQUIRE(closed);
    auto result = next();
    if (result.ok) backup = main;
    return result;
  }
  rat::FileWriteResult replace_target(std::string_view) override {
    REQUIRE(closed);
    auto result = next();
    if (result.ok) { main = temp; temp_exists = false; }
    return result;
  }
  void cleanup_temp() noexcept override { temp_exists = false; temp.clear(); }
};
}

TEST_CASE("Atomic transaction preserves main at every failed low-level step", "[unit][file][atomic]") {
  for (int step = 1; step <= 5; ++step) {
    INFO("Failure at transaction step " << step);
    FaultOps ops;
    ops.fail_at = step;
    const auto result = rat::atomic_write(ops, "map.json", "replacement");
    CHECK_FALSE(result.ok);
    CHECK_FALSE(result.error.empty());
    CHECK(ops.step == step); // No operation after the failing one.
    CHECK(ops.main == "previous");
    CHECK(ops.backup == (step == 5 ? "previous" : "older"));
    CHECK_FALSE(ops.temp_exists);
  }
  FaultOps ops;
  REQUIRE(rat::atomic_write(ops, "map.json", "replacement").ok);
  CHECK(ops.main == "replacement");
  CHECK(ops.backup == "previous");
  CHECK_FALSE(ops.temp_exists);
}

TEST_CASE("Atomic memory saves retain exactly the previous bytes", "[unit][file][atomic]") {
  rat::MemoryFileStore files;
  REQUIRE(files.write_atomic("slot", "first").ok);
  CHECK_FALSE(files.read("slot.bak").ok);
  REQUIRE(files.write_atomic("slot", "second").ok);
  CHECK(files.read("slot.bak").bytes.as_text() == "first");
  REQUIRE(files.write_atomic("slot", "third").ok);
  CHECK(files.read("slot.bak").bytes.as_text() == "second");
  CHECK_FALSE(files.read("slot.bak.bak").ok);
}

TEST_CASE("OS atomic save replaces file and keeps a checked backup without temp leftovers", "[unit][file][atomic]") {
  const auto dir = std::filesystem::temp_directory_path() / "rat-atomic-storage-test";
  std::filesystem::create_directories(dir);
  const auto path = dir / "slot.json";
  const auto bak = dir / "slot.json.bak";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::filesystem::remove(bak, ec);
  rat::OsFileStore files;
  REQUIRE(files.write_atomic(path.string(), "first").ok);
  REQUIRE(files.write_atomic(path.string(), "second").ok);
  CHECK(files.read(path.string()).bytes.as_text() == "second");
  CHECK(files.read(bak.string()).bytes.as_text() == "first");
  REQUIRE(files.write_atomic(path.string(), "third").ok);
  CHECK(files.read(bak.string()).bytes.as_text() == "second");
  std::filesystem::remove(bak);
  std::filesystem::create_directory(bak); // Backup replacement must fail, main survives.
  CHECK_FALSE(files.write_atomic(path.string(), "must not publish").ok);
  CHECK(files.read(path.string()).bytes.as_text() == "third");
  std::size_t entries = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    CHECK(entry.path().filename().string().find(".tmp.") == std::string::npos);
    ++entries;
  }
  CHECK(entries == 2);
  std::filesystem::remove(bak);
  std::filesystem::remove(path);
  std::filesystem::remove(dir);
}
