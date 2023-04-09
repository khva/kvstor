#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "kvstor.h"
#include "doctest.h"

#include <future>
#include <string>
#include <thread>


TEST_CASE("kvstor zero size")
{
    using stor_t = kvstor::storage_t<long, std::string>;
    stor_t stor{ 0 };
    REQUIRE(stor.max_size() == 0);
    REQUIRE(stor.size() == 0);

    stor.push(1, "10");
    stor.push(2, "20");
    REQUIRE(stor.size() == 0);

    bool no_items = true;
    auto enum_items = [&no_items](long, std::string)
    {
        no_items = false;
    };

    stor.map(enum_items);
    REQUIRE(no_items);
}


TEST_CASE("kvstor::push()")
{
    kvstor::storage_t<int, std::string> stor{ 4 };
    REQUIRE(stor.size() == 0);

    stor.push(1, "10");
    REQUIRE(stor.size() == 1);
    REQUIRE(stor.find(1).value() == "10");

    stor.push(2, "20");
    REQUIRE(stor.size() == 2);
    REQUIRE(stor.find(2).value() == "20");

    stor.push(3, "30");
    REQUIRE(stor.size() == 3);
    REQUIRE(stor.find(3).value() == "30");

    stor.push(2, "22");
    REQUIRE(stor.size() == 3);
    REQUIRE(stor.find(2).value() == "22");

    stor.push(4, "40");
    REQUIRE(stor.size() == 4);
    REQUIRE(stor.find(4).value() == "40");

    stor.push(5, "50");
    REQUIRE(stor.size() == 4);
    REQUIRE(stor.find(5).value() == "50");
    REQUIRE(!stor.find(1).has_value());

    stor.push(6, "60");
    REQUIRE(stor.size() == 4);
    REQUIRE(stor.find(6).value() == "60");
    REQUIRE(!stor.find(3).has_value());
}


TEST_CASE("kvstor::compare_exchange()")
{
    kvstor::storage_t<int, std::string> stor{ 4 };

    auto expected = stor.find(1);
    REQUIRE(!expected);

    // success: expected and actual objects have no values
    bool flag = stor.compare_exchange(1, "10", expected);
    REQUIRE(flag);
    REQUIRE(!expected);
    REQUIRE(stor.find(1).value() == "10");

    // fail: expected object have no value, actual have "10" value
    flag = stor.compare_exchange(1, "100", expected);
    REQUIRE(!flag);
    REQUIRE(expected.value() == "10");
    REQUIRE(stor.find(1).value() == "10");

    stor.push(2, "20");
    stor.push(1, "11");
    stor.push(3, "30");

    // fail: expected object have "10" value, actual have "11" value
    flag = stor.compare_exchange(1, "100", expected);
    REQUIRE(!flag);
    REQUIRE(expected.value() == "11");
    REQUIRE(stor.find(1).value() == "11");

    // success: expected and actual objects have "11" values
    flag = stor.compare_exchange(1, "100", expected);
    REQUIRE(flag);
    REQUIRE(expected.value() == "11");
    REQUIRE(stor.find(1).value() == "100");

    stor.push(4, "40");
    stor.push(2, "22");
    stor.push(5, "50");
    stor.push(6, "60");

    // fail: expected object have "100" value, actual have no value
    expected = std::string("100");
    flag = stor.compare_exchange(1, "111", expected);
    REQUIRE(!flag);
    REQUIRE(!expected);
    REQUIRE(!stor.find(1));
}


TEST_CASE("kvstor::erase()")
{
    kvstor::storage_t<size_t, size_t> stor{ 10 };

    stor.push(1, 10);
    stor.push(2, 20);
    stor.push(3, 30);
    stor.push(4, 40);
    REQUIRE(stor.size() == 4);
    REQUIRE(stor.find(3).value() == 30);

    stor.erase(3);
    REQUIRE(stor.size() == 3);
    REQUIRE(!stor.find(3).has_value());
}


TEST_CASE("kvstor::first()")
{
    kvstor::storage_t<size_t, std::string> stor{ 10 };
    REQUIRE(!stor.first().has_value());

    stor.push(1, std::string("A"));
    REQUIRE(stor.first().value() == "A");

    stor.push(2, std::string("B"));
    REQUIRE(stor.first().value() == "B");

    stor.push(3, std::string("C"));
    REQUIRE(stor.first().value() == "C");

    stor.erase(100);
    REQUIRE(stor.first().value() == "C");

    stor.erase(3);
    REQUIRE(stor.first().value() == "B");

    stor.erase(3);
    REQUIRE(stor.first().value() == "B");

    stor.erase(1);
    REQUIRE(stor.first().value() == "B");

    stor.erase(2);
    REQUIRE(!stor.first().has_value());
}


TEST_CASE("kvstor::last()")
{
    kvstor::storage_t<size_t, std::string> stor{ 10 };
    REQUIRE(!stor.last().has_value());

    stor.push(1, std::string("A"));
    REQUIRE(stor.last().value() == "A");

    stor.push(2, std::string("B"));
    REQUIRE(stor.last().value() == "A");

    stor.push(3, std::string("C"));
    REQUIRE(stor.last().value() == "A");

    stor.erase(100);
    REQUIRE(stor.last().value() == "A");

    stor.erase(1);
    REQUIRE(stor.last().value() == "B");

    stor.erase(1);
    REQUIRE(stor.last().value() == "B");

    stor.erase(3);
    REQUIRE(stor.last().value() == "B");

    stor.erase(2);
    REQUIRE(!stor.last().has_value());
}


TEST_CASE("kvstor::empty() / clear()")
{
    kvstor::storage_t<std::string, std::string> stor{ 10 };
    REQUIRE(stor.empty());

    const std::string s1 = "AAA";
    const std::string s2 = "BBB";

    stor.push(s1, s1);
    REQUIRE(!stor.empty());

    stor.push(s2, s2);
    REQUIRE(!stor.empty());

    stor.clear();
    REQUIRE(stor.empty());

    stor.push(s1, s2);
    REQUIRE(!stor.empty());
}


TEST_CASE("kvstor::map()")
{
    using list_t = std::list<std::pair<int, int>>;
    using stor_t = kvstor::storage_t<int, int>;

    list_t data;

    auto dump_data = [&data](int key, const int & value)
    {
        data.emplace_back(key, value);
    };

    const list_t expected_1 = { {4, 40}, {3, 30}, {2, 20}, {1, 10}, };

    stor_t stor{ 10 };
    stor.push(1, 10);
    stor.push(2, 20);
    stor.push(3, 30);
    stor.push(4, 40);

    stor.map(dump_data);
    REQUIRE(data == expected_1);

    auto double_it = [](int, int& value)
    {
        value = 2 * value;
    };

    const list_t expected_2 = { {4, 80}, {3, 60}, {2, 40}, {1, 20}, };

    stor.map(double_it);
    data.clear();
    stor.map(dump_data);
    REQUIRE(data == expected_2);
}


TEST_CASE("kvstor::dump()")
{
    using arr_t = std::vector<std::pair<int, int>>;
    using stor_t = kvstor::storage_t<int, int>;

    const arr_t expected = { {4, 40}, {3, 30}, {2, 20}, {1, 10}, };
    stor_t stor{ 10 };

    const arr_t empty_dump = stor.dump();
    REQUIRE(empty_dump.empty());

    stor.push(1, 10);
    stor.push(2, 20);
    stor.push(3, 30);
    stor.push(4, 40);

    const arr_t dump_data = stor.dump();
    REQUIRE(dump_data == expected);
}


TEST_CASE("kvstor::dump & build a zero storage")
{
    using arr_t = std::vector<std::pair<int, int>>;
    using stor_t = kvstor::storage_t<int, int>;

    stor_t stor_0{ 0 };

    const arr_t empty_dump = stor_0.dump();
    REQUIRE(empty_dump.empty());

    stor_0.push(1, 10);
    stor_0.push(2, 20);
    stor_0.push(3, 30);

    const arr_t dump_data = stor_0.dump();
    REQUIRE(dump_data.empty());

    const arr_t dum_data = { {4, 40}, {3, 30}, {2, 20}, {1, 10}, };
    const stor_t stor_0_new(dump_data, 0);
    REQUIRE(stor_0_new.empty());
}


TEST_CASE("kvstor::build_from_dump()")
{
    using arr_t = std::vector<std::pair<int, int>>;
    using stor_t = kvstor::storage_t<int, int>;

    const arr_t dump_data = { {4, 40}, {3, 30}, {2, 20}, {1, 10}, };

    const stor_t stor_10{ dump_data, 10 };
    REQUIRE(stor_10.size() == dump_data.size());

    bool is_equal = true;
    size_t index = 0;
    auto compare = [&dump_data, &index, &is_equal](int key, const int& value)
    {
        const auto& item = dump_data.at(index);

        if (item.first != key || item.second != value)
            is_equal = false;

        ++index;
    };

    stor_10.map(compare);
    REQUIRE(is_equal);

    const stor_t stor_4{ dump_data, 4 };
    REQUIRE(stor_4.size() == dump_data.size());

    is_equal = true;
    index = 0;

    stor_4.map(compare);
    REQUIRE(is_equal);

    const stor_t stor_2{ dump_data, 2 };
    REQUIRE(stor_2.size() == 2);

    is_equal = true;
    index = 0;

    stor_2.map(compare);
    REQUIRE(is_equal);

    const stor_t stor_10_empty{ arr_t{}, 10 };
    REQUIRE(stor_10_empty.empty());
}


TEST_CASE("kvstor thread-safe")
{
    constexpr size_t max_count = 20000;
    constexpr size_t count_25_percent = max_count / 4;
    constexpr size_t count_50_percent = max_count / 2;
    constexpr size_t count_75_percent = 3 * max_count / 4;
    constexpr size_t value_factor = 100;

    using stor_t = kvstor::storage_t<size_t, size_t>;
    stor_t stor{ max_count };

    auto fill_storage = [](stor_t & stor, size_t first, size_t last, size_t value_factor)
    {
        for (size_t key = first; key < last; ++key)
        {
            const size_t value = value_factor * key;
            stor.push(key, value);
        }
    };

    auto f1 = std::async(std::launch::async, fill_storage, std::ref(stor), 0, count_25_percent, value_factor);
    auto f2 = std::async(std::launch::async, fill_storage, std::ref(stor), count_25_percent, count_50_percent, value_factor);
    auto f3 = std::async(std::launch::async, fill_storage, std::ref(stor), count_50_percent, count_75_percent, value_factor);
    auto f4 = std::async(std::launch::async, fill_storage, std::ref(stor), count_75_percent, max_count, value_factor);

    f1.wait();
    f2.wait();
    f3.wait();
    f4.wait();

    REQUIRE(stor.size() == max_count);

    for (size_t key = 0; key < max_count; ++key)
    {
        const size_t expected = value_factor * key;
        const auto found = stor.find(key);

        if (!found.has_value())
            FAIL("key is missed: ", key);
        else if (found.value() != expected)
            FAIL("invalid value: ", found.value(), " for key: ", key);
    }

    const auto not_found = stor.find(max_count);
    REQUIRE(!stor.find(max_count).has_value());
}
