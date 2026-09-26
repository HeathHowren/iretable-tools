#include "iretable/Xml.h"

#include <catch2/catch_test_macros.hpp>

using namespace iretable::xml;

TEST_CASE("the parser handles declarations, attributes, nesting and self-closing tags") {
    const auto doc = parse(R"(<?xml version="1.0"?>
<root count="2">
  <item id="a">first</item>
  <item id="b"/>
</root>)");
    REQUIRE(doc.ok);
    REQUIRE(doc.root.name == "root");
    REQUIRE(doc.root.attribute("count").value() == "2");
    const auto items = doc.root.childrenNamed("item");
    REQUIRE(items.size() == 2);
    REQUIRE(items[0]->attribute("id").value() == "a");
    REQUIRE(items[0]->text == "first");
    REQUIRE(items[1]->attribute("id").value() == "b");
    REQUIRE(items[1]->text.empty());
}

TEST_CASE("entities and numeric references decode") {
    const auto doc = parse("<t>a &amp; b &lt;c&gt; &quot;d&quot; &#65; &#x42;</t>");
    REQUIRE(doc.ok);
    REQUIRE(doc.root.text == "a & b <c> \"d\" A B");
}

TEST_CASE("CDATA is taken literally and comments are skipped") {
    const auto doc = parse("<t><!-- ignore me --><![CDATA[raw <not a tag> & text]]></t>");
    REQUIRE(doc.ok);
    REQUIRE(doc.root.text == "raw <not a tag> & text");
}

TEST_CASE("a mismatched close tag is an error") {
    const auto doc = parse("<a><b></a>");
    REQUIRE_FALSE(doc.ok);
}

TEST_CASE("a UTF-8 BOM before the root is tolerated") {
    const auto doc = parse("\xEF\xBB\xBF<root/>");
    REQUIRE(doc.ok);
    REQUIRE(doc.root.name == "root");
}
