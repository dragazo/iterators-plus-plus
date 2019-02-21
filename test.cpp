#include <iostream>
#include <vector>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iterator>

#include "iterators++.h"

struct zero_int // zeroed on dtor for post-lifetime access tests
{
	int v;

	~zero_int() { v = 0; }

	zero_int &operator++() noexcept { ++v; return *this; }
	friend bool operator==(zero_int a, zero_int b) noexcept { return a.v == b.v; }
};

struct up_counter
{
	int v;

	up_counter &operator++() { ++v; return *this; }
	up_counter operator++(int) { up_counter cpy(*this); ++v; return cpy; }
};
struct up_down_counter
{
	int v;

	up_down_counter &operator++() { ++v; return *this; }
	up_down_counter operator++(int) { up_down_counter cpy(*this); ++v; return cpy; }

	up_down_counter &operator--() { --v; return *this; }
	up_down_counter operator--(int) { up_down_counter cpy(*this); --v; return cpy; }
};

struct no_default_ctor_zero_int // not default constructible - used for type compatibility test cases
{
	int v;

	no_default_ctor_zero_int() = delete;

	~no_default_ctor_zero_int() { v = 0; }

	no_default_ctor_zero_int &operator++() { ++v; return *this; }
	no_default_ctor_zero_int operator++(int) { no_default_ctor_zero_int cpy(*this); ++v; return cpy; }
};

int main()
{
	value_iterator<int> val_1(5);
	for (int i = 5; i < 100; ++i, ++val_1) { assert(*val_1 == i); }

	value_iterator<int> val_2{ 3 };
	value_iterator<int> val_3 = val_1;
	val_2 = std::move(val_3);

	value_iterator<int> val_4([&]() -> auto& { return val_2; } ());

	for (auto i : make_iterator_range(value_iterator<int>(4), value_iterator<int>(12))) std::cout << i << ' ';
	std::cout << '\n';
	for (auto i : make_value_range(4, 12)) std::cout << i << ' ';
	std::cout << "\n\n";

	auto f_1 = make_func_iterator([n = 0]()mutable{ return ++n; });
	for (int i = 0; i < 10; ++i, ++f_1) { assert(*f_1 == i + 1); std::cout << *f_1 << ' '; assert(*f_1 == i + 1); }
	std::cout << '\n';
	auto f_2 = make_unary_func_iterator<int>([](int &v) { ++v; }, 1);
	for (int i = 0; i < 10; ++i, ++f_2) { assert(*f_2 == i + 1); std::cout << *f_2 << ' '; assert(*f_2 == i + 1); }
	std::cout << '\n';
	for (auto i : make_count_range(make_unary_func_iterator<int>([](int &v) { ++v; }, 1), 10)) { std::cout << i << ' '; }
	std::cout << "\n\n";

	auto squares_iter = make_func_iterator([n = 0]()mutable { ++n; return n * n; });
	auto sqrt_iter = make_func_iterator([n = 0.0]()mutable { return std::sqrt(n += 1); });

	for (auto i : make_count_range(squares_iter, 10)) std::cout << i << ' ';
	std::cout << "\nsum square 1-10 = " << std::accumulate(make_count_iterator(squares_iter), make_count_iterator(squares_iter, 10), 0) << '\n';
	std::cout << "sum square 1-10 = " << std::accumulate(make_count_pair(squares_iter, 10), 0) << '\n';
	std::cout << "sum square 1-10 = " << make_value_range(1, 11).map([](int v) {return v * v; }).accumulate(0) << '\n';
	std::cout << "sum square 1-10 = " << make_value_range(1, 11).accumulate(0, [](int a, int b) { return a + b * b; }) << '\n';
	for (auto i : make_count_range(sqrt_iter, 10)) std::cout << i << ' ';
	std::cout << "\n\n";

	auto m_1 = make_mapping_iterator(value_iterator<int>(1), [](int v) { return 15; });
	auto m_2 = make_mapping_iterator(make_count_iterator(value_iterator<int>(1)), [](int v) { return 16; });
	auto m_3 = make_mapping_iterator(make_value_range(1, 11).begin(), [](int v) {return 17; });
	auto m_4 = map_range(make_value_range(1, 11), [](int v) { return 18; });
	assert(*m_1 == 15);
	assert(*m_2 == 16);
	assert(*m_3 == 17);
	assert(*m_4.begin() == 18);

	std::vector<int> map_test = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
	auto map_raw = map_test.begin();
	auto map_iter_1 = make_mapping_iterator(map_test.begin(), [](int v) { return v * v; });

	for (std::size_t i = 0; i < map_test.size(); ++i)
	{
		assert(*map_iter_1 == *map_raw * *map_raw);
		*map_raw += 7;
		assert(*map_iter_1 == *map_raw * *map_raw);

		++map_raw;
		++map_iter_1;
	}
	
	const auto &L_1 = value_iterator<zero_int>(zero_int{ 12345 });
	const auto &L_2 = *value_iterator<zero_int>(zero_int{ 54321 });
	const auto &L_3 = *make_func_iterator([] {return zero_int{ 13579 }; });
	const auto &L_4 = *make_unary_func_iterator<zero_int>([](zero_int &v) {}, zero_int{ 65329 });
	const auto &L_5 = *make_count_iterator(value_iterator<zero_int>(zero_int{ 273747 }));
	const auto &L_6 = *make_mapping_iterator(value_iterator<zero_int>(zero_int{ 14231 }), [](const zero_int &v) { return v; });
	assert(L_1->v == 12345);
	assert(L_2.v == 54321);
	assert(L_3.v == 13579);
	assert(L_4.v == 65329);
	assert(L_5.v == 273747);
	assert(L_6.v == 14231);

	// these should be compile-time errors
	//value_iterator<zero_int>(zero_int{ 123 })->v;
	//make_func_iterator([] { return zero_int{ 345 }; })->v;
	//make_unary_func_iterator<zero_int>([](zero_int &v) {})->v;
	//make_count_iterator(value_iterator<zero_int>(zero_int{ 234 }))->v;
	//make_mapping_iterator(value_iterator<zero_int>(zero_int{ 14231 }), [](const zero_int &v) { return v; })->v;

	// currently can't use arrow operator on mapping iterators - ideally i'd like to fix that, but i'm not sure it's possible
	//assert(make_mapping_iterator(value_iterator<zero_int>(zero_int{ 14231 }), [](const zero_int &v) { return v; })->v == 14231);

	static_assert(std::is_same<value_iterator<char>::difference_type, char>::value, "traits error");
	static_assert(std::is_same<value_iterator<unsigned short>::difference_type, unsigned short>::value, "traits error");
	static_assert(std::is_same<value_iterator<int>::difference_type, int>::value, "traits error");
	static_assert(std::is_same<value_iterator<long>::difference_type, long>::value, "traits error");
	static_assert(std::is_same<value_iterator<std::size_t>::difference_type, std::size_t>::value, "traits error");

	static_assert(std::is_same<value_iterator<unsigned long>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");
	static_assert(std::is_same<value_iterator<int>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");
	static_assert(std::is_same<value_iterator<unsigned int>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");

	static_assert(std::is_same<value_iterator<int*>::difference_type, std::ptrdiff_t>::value, "traits error");
	static_assert(std::is_same<value_iterator<const zero_int*>::difference_type, std::ptrdiff_t>::value, "traits error");
	static_assert(std::is_same<value_iterator<volatile std::ostream*>::difference_type, std::ptrdiff_t>::value, "traits error");
	static_assert(std::is_same<value_iterator<const volatile char*>::difference_type, std::ptrdiff_t>::value, "traits error");

	static_assert(std::is_same<value_iterator<int*>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");
	static_assert(std::is_same<value_iterator<const zero_int*>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");
	static_assert(std::is_same<value_iterator<volatile char*>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");
	static_assert(std::is_same<value_iterator<const volatile std::ostream*>::iterator_category, std::random_access_iterator_tag>::value, "rand access type failed");

	static_assert(std::is_same<value_iterator<up_down_counter>::difference_type, void>::value, "traits error");
	static_assert(std::is_same<value_iterator<up_down_counter>::iterator_category, std::bidirectional_iterator_tag>::value, "traits error");

	static_assert(std::is_same<value_iterator<up_counter>::difference_type, void>::value, "traits error");
	static_assert(std::is_same<value_iterator<up_counter>::iterator_category, std::forward_iterator_tag>::value, "traits error");

	static_assert(std::is_same<value_iterator<std::vector<int>>::difference_type, void>::value, "traits error");
	static_assert(std::is_same<value_iterator<std::vector<int>>::iterator_category, std::forward_iterator_tag>::value, "traits error");

	value_iterator<up_down_counter> bidir_test({ 0 });

	assert(bidir_test->v == 0);
	for (int i = 0; i < 1024; ++i) { assert(bidir_test->v == i); ++bidir_test; }
	for (int i = 1024; i > -1024; --i) { assert(bidir_test->v == i); --bidir_test; }
	for (int i = -1024; i < 0; ++i) { assert(bidir_test->v == i); ++bidir_test; }
	assert(bidir_test->v == 0);

	std::advance(bidir_test, 37);
	assert(bidir_test->v == 37);
	std::advance(bidir_test, -45);
	assert(bidir_test->v == -8);
	std::advance(bidir_test, 8);
	assert(bidir_test->v == 0);

	value_iterator<int> rand_test(0);

	for (int i = 0; i < 65565; ++i)
	{
		assert(*(rand_test + i) == i);
		assert(*(i + rand_test) == i);
		assert(*(rand_test - i) == -i);

		assert(rand_test[i] == i);
		assert(rand_test[i] == i);
		assert(rand_test[-i] == -i);

		assert((rand_test + i) - (rand_test + 32) == i - 32);

		assert((rand_test + i) < (rand_test + (i + 1)));
		assert((rand_test + i) <= (rand_test + (i + 1)));

		assert((rand_test + (i + 1)) > (rand_test + i));
		assert((rand_test + (i + 1)) >= (rand_test + i));

		assert((rand_test + i) <= (rand_test + (i + 1)) && (rand_test + (i + 1)) >= (rand_test + i));
	}
	for (int i = 0; i < 1024; ++i)
	{
		++rand_test;
		rand_test++;
		assert(*rand_test == 2);
		--rand_test;
		rand_test--;
		assert(*rand_test == 0);

		std::advance(rand_test, 253);
		assert(*rand_test == 253);
		std::advance(rand_test, -256);
		assert(*rand_test == -3);
		std::advance(rand_test, 3);
		assert(*rand_test == 0);
	}

	auto adv_1 = value_iterator<unsigned int>(0);
	auto adv_2 = value_iterator<int>(0);
	std::advance(adv_1, 1024);
	std::advance(adv_2, 1024);
	assert(*adv_1 == 1024);
	assert(*adv_2 == 1024);

	for (auto i : make_value_range(1, 11)) { std::cout << i << ' '; }
	std::cout << '\n';
	for (auto i : map_range(make_value_range(1, 11), [](int v) { return v; })) { std::cout << i << ' '; }
	std::cout << '\n';
	for (auto i : map_range(make_value_range(1, 11), [](int v) { return v * v; })) { std::cout << i << ' '; }
	std::cout << '\n';
	for (auto i : make_value_range(1, 11).map([](int v) {return v * v; })) { std::cout << i << ' '; }
	std::cout << '\n';
	for (auto i : make_value_range(1, 11).map([](int v) {return v * v; }).map([](int v) { return std::sqrt((double)v); })) { std::cout << i << ' '; }
	std::cout << '\n';

	auto ctor_1 = make_func_iterator([n = 0]()mutable{ return no_default_ctor_zero_int{ n++ }; });
	auto ctor_2 = make_unary_func_iterator<no_default_ctor_zero_int>([](no_default_ctor_zero_int &v) { ++v.v; }, no_default_ctor_zero_int{ 0 });
	for (int i = 0; i < 32; ++i)
	{
		assert(ctor_1->v == i);
		assert(ctor_1->v == i);

		++ctor_1;
		++ctor_2;
	}

	std::cout << "\n\nall tests completed" << std::endl;
	std::cin.get();
	return 0;
}
