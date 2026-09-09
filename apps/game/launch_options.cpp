#include "launch_options.hpp"
#include <charconv>
#include <stdexcept>
namespace rat::expedition {
LaunchOptions parse_launch_options(const std::vector<std::string>& args, const std::filesystem::path& executable) {
  LaunchOptions out;
  out.data_dir = executable.parent_path() / "data/expedition";
  for (std::size_t i=0; i<args.size(); ++i) {
    const auto& key=args[i];
    if (key=="--help") { out.help=true; continue; }
    if (key=="--hidden") { out.hidden=true; continue; }
    if (i+1==args.size()) throw std::runtime_error("Missing value for " + key);
    const auto& value=args[++i];
    if (key=="--data-dir") out.data_dir=std::filesystem::path(std::u8string(value.begin(), value.end()));
    else if(key=="--screenshot") out.screenshot=std::filesystem::path(std::u8string(value.begin(), value.end()));
    else if(key=="--report") out.report=std::filesystem::path(std::u8string(value.begin(), value.end()));
    else if(key=="--renderer") {
      if(value!="auto" && value!="software-d3d11" && value!="software-opengl") throw std::runtime_error("Unknown renderer: " + value);
      out.renderer=value;
    } else if(key=="--frames" || key=="--width" || key=="--height") {
      unsigned number=0;
      const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
      if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size()) throw std::runtime_error("Invalid number for " + key);
      if(key=="--frames") { if(number<1 || number>1000000) throw std::runtime_error("frames must be 1..1000000"); out.frames=number; }
      else if(key=="--width") { if(number<640 || number>7680) throw std::runtime_error("width must be 640..7680"); out.width=static_cast<int>(number); }
      else { if(number<360 || number>4320) throw std::runtime_error("height must be 360..4320"); out.height=static_cast<int>(number); }
    } else throw std::runtime_error("Unknown option: " + key);
  }
  if (!out.screenshot.empty() && out.frames==0) throw std::runtime_error("--screenshot requires --frames");
  return out;
}
}
