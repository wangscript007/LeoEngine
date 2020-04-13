#pragma once

#include "ldef.h"
#include <string>
#include <type_traits>

namespace leo
{
	template<typename _tString>
	//�ַ�������
	struct string_traits
	{
		using string_type = std::decay_t < _tString >;
		using value_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<string_type>()[0]) >>;
		using traits_type = typename std::char_traits < value_type >;
		//! \since build 1.4
		//@{
		using allocator_type = typename string_type::allocator_type;
		using size_type = typename std::allocator_traits<allocator_type>::size_type;
		using difference_type
			= typename std::allocator_traits<allocator_type>::difference_type;
		using reference = value_type&;
		using const_reference = const value_type&;
		//@}
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using initializer = std::initializer_list < value_type >;
	};

	/*!
	\note ʹ�� ADL �����ַ�����Χ��
	\note ͬ std::begin �� std::end �����ַ�������⡣
	\note �˴� string_end ����� boost::end ��ͬ�������������Ͳ�ͬ�� std::end ��
	\bug decltype ָ���ķ������Ͳ���ʹ�� ADL ��
	\see WG21 N3936 20.4.7[iterator.range] ��
	\since build 1.4
	*/
	//@{
	template<class _tRange>
	lconstfn auto
		string_begin(_tRange& c) -> decltype(c.begin())
	{
		return begin(c);
	}
	template<class _tRange>
	lconstfn auto
		string_begin(const _tRange& c) -> decltype(c.begin())
	{
		return begin(c);
	}
	//! \since build 1.4
	//@{
	template<typename _tChar>
	lconstfn _tChar*
		string_begin(_tChar* str) lnothrow
	{
		return lconstraint(str), str;
	}
#if __cplusplus <= 201402L
	//! \see http://wg21.cmeerw.net/cwg/issue1591 ��
	template<typename _tElem>
	lconstfn auto
		string_begin(std::initializer_list<_tElem> il) -> decltype(il.begin())
	{
		return il.begin();
	}
#endif
	//@}

	template<class _tRange>
	lconstfn auto
		string_end(_tRange& c) -> decltype(c.end())
	{
		return end(c);
	}
	template<class _tRange>
	lconstfn auto
		string_end(const _tRange& c) -> decltype(c.end())
	{
		return end(c);
	}
	//! \since build 1.4
	//@{
	template<typename _tChar>
	lconstfn _tChar*
		string_end(_tChar* str) lnothrow
	{
		return str + ntctslen(str);
	}
#if __cplusplus <= 201402L
	//! \see http://wg21.cmeerw.net/cwg/issue1591 ��
	template<typename _tElem>
	lconstfn auto
		string_end(std::initializer_list<_tElem> il) -> decltype(il.end())
	{
		return il.end();
	}
#endif
	//@}
	//@}

	template<typename _tString, typename _tChar = typename string_traits<_tString>::value_type>
	void to_lower(_tString&& str) {
		std::transform(string_begin(str), string_end(str), string_begin(str), [](_tChar c)
			{
				return static_cast<_tChar>(std::tolower(c));
			}
		);
	}
}