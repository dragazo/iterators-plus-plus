#include <iostream>
#include <vector>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iterator>

#include "iterators++.h"

struct zero_int
{
	int v;

	~zero_int() { v = 0; }

	zero_int &operator++() noexcept { ++v; return *this; }
	friend bool operator==(zero_int a, zero_int b) noexcept { return a.v == b.v; }
};

struct noinc_zero_int
{
	int v;

	~noinc_zero_int() { v = 0; }

	noinc_zero_int &operator++() noexcept { assert(false); return *this; }
	friend noinc_zero_int &operator+=(noinc_zero_int &a, int b) noexcept { a.v += b; return a; }

	friend bool operator==(noinc_zero_int a, noinc_zero_int b) noexcept { return a.v == b.v; }
};

struct zero_int_no_move
{
	int v;

	zero_int_no_move(int _v) : v(_v) {}

	zero_int_no_move(const zero_int_no_move&) = default;
	zero_int_no_move(zero_int_no_move&&) = delete;

	zero_int_no_move &operator=(const zero_int_no_move&) = default;
	zero_int_no_move &operator=(zero_int_no_move&&) = delete;

	~zero_int_no_move() { v = 0; }

	zero_int_no_move &operator++() noexcept { ++v; return *this; }
	friend bool operator==(zero_int_no_move a, zero_int_no_move b) noexcept { return a.v == b.v; }
};
struct zero_int_no_copy
{
	int v;

	zero_int_no_copy(int _v) : v(_v) {}

	zero_int_no_copy(const zero_int_no_copy&) = delete;
	zero_int_no_copy(zero_int_no_copy&&) = default;

	zero_int_no_copy &operator=(const zero_int_no_copy&) = delete;
	zero_int_no_copy &operator=(zero_int_no_copy&&) = default;

	~zero_int_no_copy() { v = 0; }

	zero_int_no_copy &operator++() noexcept { ++v; return *this; }
	friend bool operator==(zero_int_no_copy a, zero_int_no_copy b) noexcept { return a.v == b.v; }
};

struct up_down_counter
{
	int v;

	up_down_counter &operator++() { ++v; return *this; }
	up_down_counter operator++(int) { up_down_counter cpy(*this); ++v; return cpy; }

	up_down_counter &operator--() { --v; return *this; }
	up_down_counter operator--(int) { up_down_counter cpy(*this); --v; return cpy; }
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

	// these cases aren't covered yet - might add this behavior in the future
	//auto nm_1 = make_value_range<zero_int_no_move>(12, 14).begin();
	//auto nm_2 = make_value_range<zero_int_no_copy>(12, 14).begin();

	auto m_1 = make_mapping_iterator(value_iterator<int>(1), [](int v) { return 15; });
	auto m_2 = make_mapping_iterator(make_count_iterator(value_iterator<int>(1)), [](int v) { return 15; });
	auto m_3 = make_mapping_iterator(make_value_range(1, 11).begin(), [](int v) {return 15; });
	auto m_4 = map_range(make_value_range(1, 11), [](int v) { return 15; });
	assert(*m_1 == 15);
	assert(*m_2 == 15);
	assert(*m_3 == 15);
	assert(*m_4.begin() == 15);

	const auto &L_1 = value_iterator<zero_int>(zero_int{ 12345 });
	const auto &L_2 = *value_iterator<zero_int>(zero_int{ 54321 });
	assert(L_1->v == 12345);
	assert(L_2.v == 54321);

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

	std::cin.get();
	return 0;
}
