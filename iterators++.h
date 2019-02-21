#ifndef DRAGAZO_ITERATORS_PLUS_PLUS_H
#define DRAGAZO_ITERATORS_PLUS_PLUS_H

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <utility>
#include <memory>
#include <type_traits>
#include <iterator>
#include <numeric>

// an iterator traits helper specifically for the requirements of value_iterator.
// this helper decides on all the compile-time iterator traits to use and value_iterator aliases them and performs sfinae logic to provide the correct interface.
template<typename T>
struct value_iterator_traits
{
public: // -- simple types -- //

	typedef T value_type; // the type held by a value iterator

	typedef T *pointer;   // the pointer type to use for aliasing the stored object
	typedef T &reference; // the reference type to use for aliasing the stored object

private: // -- helpers -- //
	
	// given the type _T (should be the same as T), creates an overload set.
	// the return value of the function selected by passing nullptr is the difference type - should only be used in decltype contexts.
	template<typename _T = T>
	static void __diff_type(void*);
	template<typename _T = T, std::enable_if_t<std::is_integral<_T>::value, int> = 0>
	static _T __diff_type(std::nullptr_t);
	template<typename _T = T, std::enable_if_t<std::is_pointer<_T>::value, int> = 0>
	static std::ptrdiff_t __diff_type(std::nullptr_t);

	// given the type _T (should be the same as T), creates an overload set.
	// the return value of the function selected by passing nullptr is a type denoting if _T has an operator -- (prefix).
	template<typename _T = T>
	static std::false_type __has_prefix_dec(void*);
	template<typename _T = T, std::enable_if_t<!std::is_same<decltype(--*(_T*)nullptr), void>::value, int> = 0>
	static std::true_type __has_prefix_dec(std::nullptr_t);

public: // -- fancy types -- //

	// the difference type to use for random access operators.
	// this is void if the iterator is not random access.
	typedef typename decltype(__diff_type(nullptr)) difference_type;

	// the iterator category type denoting the capabilities of a value iterator for type T.
	// this is random access if the difference type is non-void, otherwise bidirectional if a -- (prefix) operator exists, otherwise forward only.
	typedef std::conditional_t<!std::is_same<difference_type, void>::value, std::random_access_iterator_tag,
		std::conditional_t<decltype(__has_prefix_dec(nullptr))::value, std::bidirectional_iterator_tag,
		std::forward_iterator_tag>> iterator_category;
};

// holds (by-value) an object of type T - iterator operations modify/fetch the value as if it pointed into an imaginary container of T values.
// T must define operators ++ (prefix) and ==.
// this is guaranteed to be at least a forward iterator.
// if operator -- (prefix) is also defined for this type, it is at least a bidirectional iterator.
// if T is an integral type or pointer type, this is a random access iterator.
// T is not assumed to be a trivial type, but a large, non-trivial type would likely be a bad idea, as many libraries assume iterators are easy to copy.
template<typename T>
class value_iterator
{
public: // -- types -- //

	typedef typename value_iterator_traits<T>::iterator_category iterator_category;
	typedef typename value_iterator_traits<T>::difference_type difference_type;

	typedef typename value_iterator_traits<T>::value_type value_type;

	typedef typename value_iterator_traits<T>::pointer pointer;
	typedef typename value_iterator_traits<T>::reference reference;

private: // -- data -- //

	T data; // the stored value

	// true if we're supposed to be at least a random access iterator
	static constexpr bool rand_access = std::is_same<iterator_category, std::random_access_iterator_tag>::value;
	// true if we're supposed to be at least a bidirectional iterator
	static constexpr bool bidirectional = rand_access || std::is_same<iterator_category, std::bidirectional_iterator_tag>::value;

public: // -- ctor / dtor / asgn -- //

	// constructs the T object with the specified value.
	constexpr explicit value_iterator(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) : data(v) {}
	constexpr explicit value_iterator(T &&v) noexcept(std::is_nothrow_move_constructible<T>::value) : data(std::move(v)) {}

	// constructs a new value iterator by copying the current value from other.
	constexpr value_iterator(const value_iterator &other) noexcept(std::is_nothrow_copy_constructible<T>::value) : data(other.data) {}
	// constructs a new value iterator by moving the current value from other.
	// the other iterator is left in an undefined but valid state.
	constexpr value_iterator(value_iterator &&other) noexcept(std::is_nothrow_move_constructible<T>::value) : data(std::move(other.data)) {}

	// copies other's current value to this object's current value.
	constexpr value_iterator &operator=(const value_iterator &other) noexcept(std::is_nothrow_copy_assignable<T>::value) { data = other.data; return *this; }
	// moves other's current value to this object's current value.
	// the other iterator is left in an undefined but valid state.
	constexpr value_iterator &operator=(value_iterator &&other) noexcept(std::is_nothrow_move_assignable<T>::value) { data = std::move(other.data); return *this; }

public: // -- data access -- //

	// returns the current stored T value.
	constexpr T &operator*() & noexcept { return data; }
	constexpr T operator*() && noexcept(std::is_nothrow_move_constructible<T>::value) { return std::move(data); }
	constexpr const T &operator*() const& noexcept { return data; }

	// retruns the address of the current stored T value.
	constexpr T *operator->() & noexcept { return std::addressof(data); }
	constexpr T *operator->() && = delete;
	constexpr const T *operator->() const& noexcept { return std::addressof(data); }

public: // -- forward iterator functions -- //

	// increments the stored T value.
	constexpr value_iterator &operator++() noexcept(noexcept(++data)) { ++data; return *this; }
	constexpr value_iterator operator++(int) { value_iterator cpy(*this); ++data; return cpy; }

public: // -- bidirectional iterator functions -- //

	// bidirectional - decrements the stored T value.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && bidirectional, int> = 0>
	constexpr value_iterator &operator--() noexcept(noexcept(--data)) { --data; return *this; }
	// bidirectional - decrements the stored T value.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && bidirectional, int> = 0>
	constexpr value_iterator operator--(int) { value_iterator cpy(*this); --data; return cpy; }

public: // -- random access iterator functions -- //

	// random access - copies the current stored value, adds d, and returns the result.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr T operator[](typename value_iterator_traits<_T>::difference_type d) { T cpy(data); cpy += d; return cpy; }

	// random access - adds d to the current value and returns a reference to this object.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr value_iterator &operator+=(typename value_iterator_traits<_T>::difference_type d) { data += d; return *this; }
	// random access - subtracts d from the current value and returns a reference to this object.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr value_iterator &operator-=(typename value_iterator_traits<_T>::difference_type d) { data -= d; return *this; }

	// random access - copies the current iterator state, adds d to it, and returns the result.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend value_iterator operator+(const value_iterator &v, typename value_iterator_traits<_T>::difference_type d) { value_iterator cpy(v); cpy += d; return cpy; }
	// random access - copies the current iterator state, adds d to it, and returns the result.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend value_iterator operator+(typename value_iterator_traits<_T>::difference_type d, const value_iterator &v) { value_iterator cpy(v); cpy += d; return cpy; }

	// random access - copies the current iterator state, subtracts d from it, and returns the result.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend value_iterator operator-(const value_iterator &v, typename value_iterator_traits<_T>::difference_type d) { value_iterator cpy(v); cpy -= d; return cpy; }

	// random access - returns the difference of the stored T values a-b as defined by type T.
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend difference_type operator-(const value_iterator &a, const value_iterator &b) { return a.data - b.data; }
	
	// random access - returns the result of comparing the stored values of iterators a and b
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend bool operator<(const value_iterator &a, const value_iterator &b) { return a.data < b.data; }
	// random access - returns the result of comparing the stored values of iterators a and b
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend bool operator<=(const value_iterator &a, const value_iterator &b) { return a.data <= b.data; }
	// random access - returns the result of comparing the stored values of iterators a and b
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend bool operator>(const value_iterator &a, const value_iterator &b) { return a.data > b.data; }
	// random access - returns the result of comparing the stored values of iterators a and b
	template<typename _T = T, std::enable_if_t<std::is_same<_T, T>::value && rand_access, int> = 0>
	constexpr friend bool operator>=(const value_iterator &a, const value_iterator &b) { return a.data >= b.data; }

public: // -- comparison -- //

	// compares the stored values in the iterators
	constexpr friend bool operator==(const value_iterator &a, const value_iterator &b) noexcept(noexcept(a.data == b.data)) { return a.data == b.data; }
	constexpr friend bool operator!=(const value_iterator &a, const value_iterator &b) noexcept(noexcept(a.data == b.data)) { return !(a.data == b.data); }
};

// represents an iterator that aliases a function with compatibility signature V() for getting the next value.
// the stored function may keep an internal state (e.g. stateful lambdas), enabling highly-modular iterator designs.
template<typename F, typename V = decltype(std::declval<F>()())>
class func_iterator
{
private: // -- data -- //

	// buffer for the cached value.
	// V might not be default constructible and we might not have an initial value at ctor time.
	alignas(V) char _value[sizeof(V)];

	F func; // the stored function

private: // -- helpers -- //

	// aliases the buffer object with the proper cv-qualifiers for 'this'
	V &value() noexcept { return reinterpret_cast<V&>(_value); }
	const V &value() const noexcept { return reinterpret_cast<const V&>(_value); }

public: // -- ctor / dtor / asgn -- //

	// constructs a new function iterator from the given function.
	// the function is called once to get the initial cached value.
	constexpr explicit func_iterator(const F &f) : func(f) { new (_value) V(func()); }
	constexpr explicit func_iterator(F &&f) : func(std::move(f)) { new (_value) V(func()); }

	~func_iterator() { value().~V(); }

	// constructs a new function iterator with a copy of other's stored function and cached value.
	constexpr func_iterator(const func_iterator &other) : func(other.func) { new (_value) V(other.value()); }
	// constructs a new function iterator by moving from other's stored function and cached value.
	// the other iterator is left in an undefined but valid state.
	constexpr func_iterator(func_iterator &&other) : func(std::move(other.func)) { new (_value) V(std::move(other.value())); }

	// copies other's current stored function and cached value to this iterator.
	constexpr func_iterator &operator=(const func_iterator &other) { value() = other.value(); func = other.func; return *this; }
	// moves other's current stored function and cached value to this iterator.
	// the other iterator is left in an undefined but valid state.
	constexpr func_iterator &operator=(func_iterator &&other) { value() = std::move(other.value()); func = std::move(other.func); return *this; }

public: // -- value access -- //

	// returns the cached value
	constexpr V &operator*() & noexcept { return value(); }
	constexpr V operator*() && noexcept(std::is_nothrow_move_constructible<V>::value) { return std::move(value()); }
	constexpr const V &operator*() const& noexcept { return value(); }

	// retruns the address of the cached value.
	constexpr V *operator->() & noexcept { return std::addressof(value()); }
	constexpr V *operator->() && = delete;
	constexpr const V *operator->() const& noexcept { return std::addressof(value()); }

public: // -- inc -- //

	// calls the stored function to get the next value and stores it to the cached
	constexpr func_iterator &operator++() { value() = func(); return *this; }
	constexpr func_iterator operator++(int) { func_iterator cpy(*this); value() = func(); return cpy; }

public: // -- comparison -- //

	// compares the cached values
	constexpr friend bool operator==(const func_iterator &a, const func_iterator &b) noexcept(noexcept(a.value() == b.value())) { return a.value() == b.value(); }
	constexpr friend bool operator!=(const func_iterator &a, const func_iterator &b) noexcept(noexcept(a.value() != b.value())) { return a.value() != b.value(); }
};

// represents an iterator that aliases a function with compatibility signature void(V&) for getting the next value.
// the stored function may keep an internal state (e.g. stateful lambdas), enabling highly-modular iterator designs.
template<typename F, typename V>
class unary_func_iterator
{
private: // -- data -- //

	V value; // the cached value
	F func;  // the stored function

public: // -- ctor / dtor / asgn -- //

	// constructs a new function iterator from the given function - the initial value is constructed from the args.
	template<typename ...Args>
	constexpr explicit unary_func_iterator(const F &f, Args &&...args) : value(std::forward<Args>(args)...), func(f) {}
	template<typename ...Args>
	constexpr explicit unary_func_iterator(F &&f, Args &&...args) : value(std::forward<Args>(args)...), func(std::move(f)) {}

	// constructs a new function iterator with a copy of other's stored function and cached value.
	constexpr unary_func_iterator(const unary_func_iterator &other) : value(other.value), func(other.func) {}
	// constructs a new function iterator by moving from other's stored function and cached value.
	// the other iterator is left in an undefined but valid state.
	constexpr unary_func_iterator(unary_func_iterator &&other) : value(std::move(other.value)), func(std::move(other.func)) {}

	// copies other's current stored function and cached value to this iterator.
	constexpr unary_func_iterator &operator=(const unary_func_iterator &other) { value = other.value; func = other.func; return *this; }
	// moves other's current stored function and cached value to this iterator.
	// the other iterator is left in an undefined but valid state.
	constexpr unary_func_iterator &operator=(unary_func_iterator &&other) { value = std::move(other.value); func = std::move(other.func); return *this; }

public: // -- value access -- //

	// returns the cached value
	constexpr V &operator*() & noexcept { return value; }
	constexpr V operator*() && noexcept(std::is_nothrow_move_constructible<V>::value) { return std::move(value); }
	constexpr const V &operator*() const& noexcept { return value; }

	// retruns the address of the cached value.
	constexpr V *operator->() & noexcept { return std::addressof(value); }
	constexpr V *operator->() && = delete;
	constexpr const V *operator->() const& noexcept { return std::addressof(value); }

public: // -- inc -- //

	// calls the stored function to get the next value and stores it to the cached
	constexpr unary_func_iterator &operator++() { func(value); return *this; }
	constexpr unary_func_iterator operator++(int) { unary_func_iterator cpy(*this); func(value); return cpy; }

public: // -- comparison -- //

	// compares the cached values
	constexpr friend bool operator==(const unary_func_iterator &a, const unary_func_iterator &b) noexcept(noexcept(a.value == b.value)) { return a.value == b.value; }
	constexpr friend bool operator!=(const unary_func_iterator &a, const unary_func_iterator &b) noexcept(noexcept(a.value != b.value)) { return a.value != b.value; }
};

// takes a function object and returns a function iterator for it.
template<typename F, typename V = decltype(std::declval<F>()())>
auto make_func_iterator(F &&func) { return func_iterator<std::decay_t<F>, V>(std::forward<F>(func)); }

// takes a function object and ctor args for the initial value and constructs a unary function iterator for it.
template<typename V, typename F, typename ...Args>
auto make_unary_func_iterator(F &&func, Args &&...args) { return unary_func_iterator<std::decay_t<F>, V>(std::forward<F>(func), std::forward<Args>(args)...); }

// given an iterator type, creates another iterator type that uses a counter for comparison.
// this is typically used for generating finite sequences without needed to know the effective end iterator's value.
template<typename Iter>
class count_iterator
{
private: // -- data -- //

	Iter        iter;  // the stored iterator
	std::size_t count; // the stored counter

public: // -- ctor / dtor / asgn -- //

	// constructs a new counting iterator from the underlying iterator - the count starts at the specified value.
	constexpr explicit count_iterator(const Iter &_iter, std::size_t _count = 0) : iter(_iter), count(_count) {}
	constexpr explicit count_iterator(Iter &&_iter, std::size_t _count = 0) : iter(std::move(_iter)), count(_count) {}

	// constructs a new counting iterator with a copy of other's stored count and iterator.
	constexpr count_iterator(const count_iterator &other) : iter(other.iter), count(other.count) {}
	// constructs a new counting iterator by moving from other's stored count and iterator.
	// the other iterator is left in an undefined but valid state.
	constexpr count_iterator(count_iterator &&other) : iter(std::move(other.iter)), count(other.count) {}

	// copies other's current count and iterator to this iterator.
	constexpr count_iterator &operator=(const count_iterator &other) { iter = other.iter; count = other.count; return *this; }
	// moves other's current count and iterator to this iterator.
	// the other iterator is left in an undefined but valid state.
	constexpr count_iterator &operator=(count_iterator &&other) { iter = std::move(other.iter); count = other.count; return *this; }

public: // -- iter access -- //

	// returns the value of dereferencing the stored iterator.
	constexpr decltype(auto) operator*() & { return *iter; }
	constexpr decltype(auto) operator*() && { return *std::move(iter); }
	constexpr decltype(auto) operator*() const& { return *iter; }

	// returns the value of using the arrow operator on the iterator.
	constexpr decltype(auto) operator->() & { return iter.operator->(); }
	constexpr decltype(auto) operator->() && = delete;
	constexpr decltype(auto) operator->() const& { return iter.operator->(); }

public: // -- inc -- //

	// increments the stored iterator and count
	constexpr count_iterator &operator++() { ++count; ++iter; return *this; }
	constexpr count_iterator operator++(int) { count_iterator cpy(*this); ++*this; return cpy; }

public: // -- comparison -- //

	// compares the counts - does not compare the stored iterators
	constexpr friend bool operator==(const count_iterator &a, const count_iterator &b) { return a.count == b.count; }
	constexpr friend bool operator!=(const count_iterator &a, const count_iterator &b) { return a.count != b.count; }
};

// given an iterator, creates a count iterator with the specified initial count value
template<typename Iter>
auto make_count_iterator(Iter &&iter, std::size_t count = 0) { return count_iterator<std::decay_t<Iter>>(std::forward<Iter>(iter), count); }

// contains a stored iterator and a stored function.
// dereferencing this iterator is equivalent to dereferencing the stored iterator, passing it to the function, and using the return value.
template<typename Iter, typename F>
class mapping_iterator
{
public: // -- types -- //

	// the type of value the function will operate on
	typedef std::decay_t<decltype(std::declval<F>()(std::declval<decltype(*std::declval<Iter>())>()))> value_t;

private: // -- data -- //

	Iter    iter;  // the stored iterator
	F       func;  // the stored function

	// optional storage location for the mapped value.
	// this is required because operator -> needs an lvalue to take the address of, so we need to put the potentially-prvalue result somewhere.
	mutable value_t temp_mapped_value;

public: // -- ctor / dtor / asgn -- //

	// creates a new mapping iterator from the given starting iterator and function object.
	mapping_iterator(Iter _iter, const F &_func) : iter(std::move(_iter)), func(std::move(_func)) {}
	mapping_iterator(Iter _iter, F &&_func) : iter(std::move(_iter)), func(std::move(_func)) {}

	// constructs a new mapping iterator with a copy of other's stored iterator and function.
	constexpr mapping_iterator(const mapping_iterator &other) : iter(other.iter), func(other.func) {}
	// constructs a new mapping iterator by moving from other's stored iterator and function.
	// the other iterator is left in an undefined but valid state.
	constexpr mapping_iterator(mapping_iterator &&other) : iter(std::move(other.iter)), func(std::move(other.func)) {}

	// copies other's current iterator and function to this iterator.
	constexpr mapping_iterator &operator=(const mapping_iterator &other) { iter = other.iter; func = other.func; return *this; }
	// moves other's current iterator and function to this iterator.
	// the other iterator is left in an undefined but valid state.
	constexpr mapping_iterator &operator=(mapping_iterator &&other) { iter = std::move(other.iter); func = std::move(other.func); return *this; }

public: // -- access -- //

	// dereferences the stored iterator, passes it through the mapping function, and returns the result
	constexpr decltype(auto) operator*() & { return func(*iter); }
	constexpr decltype(auto) operator*() && { return func(*std::move(iter)); }
	constexpr decltype(auto) operator*() const& { return func(*iter); }

	// dereferences the stored iterator, and passes it through the mapping function.
	// the result is stored to a temporary location inside this iterator and the address is returned.
	// care should be taken, though, as the object's lifetime potentially ends on the next -> access or the destruction of this iterator.
	constexpr value_t *operator->() & { temp_mapped_value = *iter; return std::addressof(temp_mapped_value); }
	constexpr value_t *operator->() && = delete;
	constexpr value_t *operator->() const& { temp_mapped_value = *iter; return std::addressof(temp_mapped_value); }

public: // -- inc -- //

	// increments the stored iterator.
	constexpr mapping_iterator &operator++() { ++iter; return *this; }
	constexpr mapping_iterator operator++(int) { mapping_iterator cpy(*this); ++iter; return cpy; }

public: // -- comparison -- //

	// compares the stored iterators - does not compare the mapped values
	constexpr friend bool operator==(const mapping_iterator &a, const mapping_iterator &b) noexcept(noexcept(a.iter == b.iter)) { return a.iter == b.iter; }
	constexpr friend bool operator!=(const mapping_iterator &a, const mapping_iterator &b) noexcept(noexcept(a.iter != b.iter)) { return a.iter != b.iter; }
};

template<typename Iter, typename F>
auto make_mapping_iterator(Iter &&iter, F &&func) { return mapping_iterator<std::decay_t<Iter>, std::decay_t<F>>(std::forward<Iter>(iter), std::forward<F>(func)); }

// represents an iterator range - stores a begin() and an end() and can by used in range-based for loops
template<typename IterBegin, typename IterEnd>
class iterator_range
{
private: // -- data -- //

	IterBegin _begin; // the begin iterator
	IterEnd   _end;   // the end iterator

public: // -- ctor / dtor / asgn -- //

	// constructs a new iterator range with the specified iterator range.
	constexpr iterator_range(IterBegin b, IterEnd e) : _begin(b), _end(e) {}
	
	// creates a new iterator range that is a copy of other's iterators.
	constexpr iterator_range(const iterator_range &other) : _begin(other._begin), _end(other._end) {}
	// creates a new iterator range by move constructing from other's iterators.
	constexpr iterator_range(iterator_range &&other) : _begin(std::move(other._begin)), _end(std::move(other._end)) {}

	// copy assigns other's iterators to this object.
	constexpr iterator_range &operator=(const iterator_range &other) { _begin = other._begin; _end = other._end; return *this; }
	// move assignes other's iterators to this object
	constexpr iterator_range &operator=(iterator_range &&other) { _begin = std::move(other._begin); _end = std::move(other._end); return *this; }

public: // -- range access -- //

	// accesses the stored begin iterator.
	constexpr IterBegin begin() const& noexcept(std::is_nothrow_copy_constructible<IterBegin>::value) { return _begin; }
	constexpr IterBegin begin() && noexcept(std::is_nothrow_move_constructible<IterBegin>::value) { return std::move(_begin); }

	// accesses the stored end iterator.
	constexpr IterEnd end() const& noexcept(std::is_nothrow_copy_constructible<IterEnd>::value) { return _end; }
	constexpr IterEnd end() && noexcept(std::is_nothrow_move_constructible<IterEnd>::value) { return std::move(_end); }

public: // -- mapping -- //

	// given a mapping function, returns a new iterator range that maps this range through the function.
	template<typename F>
	constexpr auto map(const F &func) const& -> iterator_range<mapping_iterator<IterBegin, std::decay_t<F>>, mapping_iterator<IterEnd, std::decay_t<F>>>
	{
		return { { _begin, func }, { _end, func } };
	}
	template<typename F>
	constexpr auto map(const F &func) && -> iterator_range<mapping_iterator<IterBegin, std::decay_t<F>>, mapping_iterator<IterEnd, std::decay_t<F>>>
	{
		return { { std::move(_begin), func }, { std::move(_end), func } };
	}

public: // -- stdlib utilities -- //

	// equivalent to std::accumulate(begin(), end(), init)
	template<typename T>
	T accumulate(T init) { return std::accumulate(_begin, _end, init); }
	// equivalent to std::accumulate(begin(), end(), init, op)
	template<typename T, typename BinaryOperation>
	T accumulate(T init, BinaryOperation op) { return std::accumulate(_begin, _end, init, op); }
};

// given a begin and end iterator, constructs the iterator range [begin, end)
template<typename IterBegin, typename IterEnd>
iterator_range<IterBegin, IterEnd> make_iterator_range(IterBegin begin, IterEnd end) { return {begin, end}; }

// given a begin and end value, constructs the value iterator range [begin, end)
template<typename T>
iterator_range<value_iterator<T>, value_iterator<T>> make_value_range(const T &begin, const T &end) { return { value_iterator<T>(begin), value_iterator<T>(end) }; }

// given a begin iterator and a count, creates the iterator range [begin, begin + count) using counting iterators
template<typename Iter>
iterator_range<count_iterator<Iter>, count_iterator<Iter>> make_count_range(const Iter &begin, std::size_t count) { return { count_iterator<Iter>(begin, 0), count_iterator<Iter>(begin, count) }; }

// expands to two comma-separated counting iterators representing the range [begin, begin + count)
#define make_count_pair(begin, count) count_iterator<decltype(begin)>(begin, 0), count_iterator<decltype(begin)>(begin, count)

// given an iterator range and a mapping function, creates a new iterator range for the mapped values.
template<typename Iter, typename F>
auto map_range(const iterator_range<Iter, Iter> &range, const F &func) -> iterator_range<mapping_iterator<Iter, std::decay_t<F>>, mapping_iterator<Iter, std::decay_t<F>>>
{
	return { mapping_iterator<Iter, std::decay_t<F>>(range.begin(), func), mapping_iterator<Iter, std::decay_t<F>>(range.end(), func) };
}

#endif
