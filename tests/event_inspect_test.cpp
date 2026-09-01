#include <rat/event_inspect.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("trigger_kind_name covers MVP triggers", "[unit][event_inspect]") {
  REQUIRE(std::string(rat::trigger_kind_name(rat::TriggerKind::Action)) == "action");
  REQUIRE(std::string(rat::trigger_kind_name(rat::TriggerKind::Autorun)) == "autorun");
}

TEST_CASE("summarize_page_conditions lists switches", "[unit][event_inspect]") {
  rat::EventPage page;
  REQUIRE(rat::summarize_page_conditions(page) == "(none)");

  rat::Condition sw;
  sw.type = rat::ConditionType::Switch;
  sw.id = 3;
  sw.bool_value = true;
  page.conditions.push_back(sw);
  REQUIRE(rat::summarize_page_conditions(page) == "SW3=ON");
}

TEST_CASE("ensure_page_enable_switch adds once", "[unit][event_inspect]") {
  rat::EventPage page;
  const int first = rat::ensure_page_enable_switch(page, 7, false);
  REQUIRE(first == 0);
  REQUIRE(page.conditions.size() == 1);
  REQUIRE(page.conditions[0].id == 7);
  REQUIRE(page.conditions[0].bool_value == false);

  const int again = rat::ensure_page_enable_switch(page, 99, true);
  REQUIRE(again == 0);
  REQUIRE(page.conditions.size() == 1);
  REQUIRE(page.conditions[0].id == 7);
}

TEST_CASE("find_first_show_text locates ShowText command", "[unit][event_inspect]") {
  rat::EventPage page;
  REQUIRE(rat::find_first_show_text(page) == -1);
  rat::Command wait;
  wait.op = rat::CommandOp::Wait;
  page.commands.push_back(wait);
  rat::Command text;
  text.op = rat::CommandOp::ShowText;
  text.text = "hi";
  page.commands.push_back(text);
  REQUIRE(rat::find_first_show_text(page) == 1);
}
