#ifndef IndePlatform_iterator_op_hpp
#define IndePlatform_iterator_op_hpp


#include "type_op.hpp" // for std::pair, std::declval, enable_if_t, is_same;
#include "Lassert.h" // for yconstraint;
#include "deref_op.hpp" // for is_undereferenceable;
#include <iterator> // for std::next, std::prev, std::reverse_iterator;
#include "operators.hpp" // for input_iteratable, output_iteratable,
//	forward_iteratable, bidirectional_iteratable, random_access_iteratable;

namespace leo
{
	/*!
	\ingroup metafunctions
	\brief 选择迭代器类型的特定重载避免和其它类型冲突。
	\sa enable_if_t
	*/
	template<typename _tParam, typename = limpl(std::pair<
		decltype(*std::declval<_tParam&>()), decltype(++std::declval<_tParam&>())>)>
		using enable_for_iterator_t = enable_if_t<
		is_same<decltype(++std::declval<_tParam&>()), _tParam&>::value, int>;


	/*
	迭代器指向的值满足条件取邻接迭代器,否则取原值
	*/
	template<typename _tIn, typename _fPred>
	_tIn
		next_if(_tIn i, _fPred f,
			typename std::iterator_traits<_tIn>::difference_type n = 1)
	{
		lconstraint(!is_undereferenceable(i));
		return f(*i) ? std::next(i, n) : i;
	}
	template<typename _tIn, typename _type>
	_tIn
		next_if_eq(_tIn i, const _type& val,
			typename std::iterator_traits<_tIn>::difference_type n = 1)
	{
		lconstraint(!is_undereferenceable(i));
		return *i == val ? std::next(i, n) : i;
	}

	/*
	迭代器指向的值满足条件时取反向邻接迭代器,否则取原值
	*/
	template<typename _tBi, typename _fPred>
	_tBi
		prev_if(_tBi i, _fPred f,
			typename std::iterator_traits<_tBi>::difference_type n = 1)
	{
		return f(*i) ? std::prev(i, n) : i;
	}
	template<typename _tBi, typename _type>
	_tBi
		prev_if_eq(_tBi i, const _type& val,
			typename std::iterator_traits<_tBi>::difference_type n = 1)
	{
		return *i == val ? std::prev(i, n) : i;
	}

	template<typename _tIterator1, typename _tIterator2>
	inline auto
		make_move_iterator_pair(_tIterator1 it1, _tIterator2 it2) -> decltype(
			std::make_pair(std::make_move_iterator(it1), std::make_move_iterator(it2)))
	{
		return std::make_pair(std::make_move_iterator(it1),
			std::make_move_iterator(it2));
	}

	template<typename _tRange>
	inline auto
		make_move_iterator_pair(_tRange& c)
		-> decltype(leo::make_move_iterator_pair(begin(c), end(c)))
	{
		return leo::make_move_iterator_pair(begin(c), end(c));
	}
}

#endif
