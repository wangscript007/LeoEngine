/*! \file LObject.h
\ingroup LFrameWork/Core
\brief 基础对象。

*/
#ifndef LFrameWork_Core_LObject_H
#define LFrameWork_Core_LObject_H 1

#include <LBase/any.h>
#include <LBase/examiner.hpp>
#include <LBase/operators.hpp>
#include <LBase/observer_ptr.hpp>
#include <LFramework/Core/LCoreUtilities.h>

#ifdef LB_IMPL_MSCPP
#include <fstream>
#include <sstream>
#endif

namespace leo
{
	/*!
	\brief 指定对参数指定类型的成员具有所有权的标签。
	*/
	template<typename = void>
	struct OwnershipTag
	{};

	struct MoveTag
	{};

	struct PointerTag
	{};

	template<class _tOwner, typename _type>
	struct HasOwnershipOf : std::is_base_of<OwnershipTag<_type>, _tOwner>
	{};

	/*!
	\brief 第一个参数指定的选项创建擦除类型的持有者或抛出异常。
	*/
	//@{
	template<class _tHolder, typename... _tParams>
	leo::any
		CreateHolderInPlace(leo::true_, _tParams&&... args)
	{
		return leo::any(leo::any_ops::use_holder,
			leo::in_place<_tHolder>, lforward(args)...);
	}
	//! \exception leo::invalid_construction 参数类型无法用于初始化持有者。
	template<class _tHolder, typename... _tParams>
	LB_NORETURN leo::any
		CreateHolderInPlace(leo::false_, _tParams&&...)
	{
		leo::throw_invalid_construction();
	}
	//@}

	DeclDerivedI(LF_API, IValueHolder, leo::any_ops::holder)

		/*!
		\brief 创建选项。
		\sa Create
		*/
		enum Creation
		{
		/*!
		\brief 创建引用的间接持有者。

		派生实现应保证持有对应的 lref<T> 类型的值引用当前持有的 T 类型的值。
		*/
		Indirect,
		/*!
		\brief 创建引用的值的副本。

		派生实现应保证持有对应的值从当前持有的值复制，
		当且仅当当前持有者不是引用；否则，从引用的值复制。
		*/
		Copy,
		/*!
		\brief 创建引用的值转移的副本。

		派生实现应保证持有对应的值从当前持有的值转移，
		当且仅当当前持有者不是引用；否则，从引用的值转移。
		*/
		Move
		};

		/*!
		\brief 判断相等。
		\pre 参数为空指针值或持有的对象去除 const 后具有和参数相同动态类型的对象。
		\return 持有的对象相等，或持有空对象且参数为空指针值。

		比较参数和持有的对象。
		派生实现应保证返回的值满足 EqualityComparable 中对 == 操作的要求。
		*/
		DeclIEntry(bool Equals(const void*) const)
		/*!
		\brief 取被持有对象的共享所有者总数。
		*/
		DeclIEntry(size_t OwnsCount() const lnothrow)
		/*!
		\sa Creation
		*/
		//@{
		/*!
		\brief 创建新的持有者。
		\return 包含新创建的持有者的动态泛型对象。
		\exception leo::invalid_construction 参数类型无法用于初始化持有者。
		\sa Creation
		\sa CreateHolder

		按参数指定的选项创建按指定选项持有的对象。
		派生实现应保证返回的值满足选项指定的条件，且变换不改变当前逻辑状态，
		除 mutable 的数据成员可被转移；否则，应抛出异常。
		*/
		DeclIEntry(leo::any Create(Creation) const)

		/*!
		\brief 提供创建持有者的默认实现。
		\sa Create
		*/
		template<typename _type>
		static leo::any
		CreateHolder(Creation, _type&);
		//@}
	EndDecl


	template<typename _type1, typename _type2>
	struct HeldEqual : private leo::examiners::equal_examiner

	{
		using examiners::equal_examiner::are_equal;
	};

#if  defined(LB_IMPL_MSCPP) && 0
	template<>
	struct HeldEqual<std::ifstream, std::ifstream>
	{
		static bool are_equal(const std::ifstream&, const std::ifstream&)
		{
			return true;
		}
	};

	template<>
	struct HeldEqual<std::istringstream, std::istringstream>
	{
		static bool are_equal(const std::istringstream&, const std::istringstream&)
		{
			return true;
		}
	};
#endif

	template<typename _type1, typename _type2>
	struct HeldEqual<weak_ptr<_type1>, weak_ptr<_type2>>
	{
		static bool
			are_equal(const weak_ptr<_type1>& x, const weak_ptr<_type2>& y)
		{
			return x == y;
		}
	};


	template<typename _type1, typename _type2, typename _type3, typename _type4>
	struct HeldEqual<pair<_type1, _type2>, pair<_type3, _type4>>

	{
		static lconstfn bool
			are_equal(const pair<_type1, _type2>& x, const pair<_type3, _type4>& y)
		{
			return x.first == y.first && x.second == y.second;
		}
	};

	template<typename _type1, typename _type2>
	lconstfn bool
		AreEqualHeld(const _type1& x, const _type2& y)
	{
		return HeldEqual<_type1, _type2>::are_equal(x, y);
	}

	template<typename _type>
	class ValueHolder
		: protected boxed_value<_type>, implements IValueHolder
	{
		static_assert(std::is_object<_type>(), "Non-object type found.");
		static_assert(!is_cv<_type>(), "Cv-qualified type found.");

	public:
		using value_type = _type;

		//@{
		DefDeCtor(ValueHolder)
			template<typename _tParam,
			limpl(typename = exclude_self_t<ValueHolder, _tParam>)>
			ValueHolder(_tParam&& arg)
			lnoexcept(std::is_nothrow_constructible<_type, _tParam&&>::value)
			: boxed_value<_type>(lforward(arg))
		{}
		using boxed_value<_type>::boxed_value;
		//@}
		DefDeCopyMoveCtorAssignment(ValueHolder)

		PDefH(leo::any, Create, Creation c) const ImplI(IValueHolder)
		ImplRet(CreateHolder(c, this->value))

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
		ImplRet(bool(p) && AreEqualHeld(this->value,
			Deref(static_cast<const value_type*>(p))))

		PDefH(size_t, OwnsCount, ) const lnothrow ImplI(IValueHolder)
		ImplRet(1)

		PDefH(void*, get, ) const ImplI(IValueHolder)
		ImplRet(leo::addressof(this->value))

		PDefH(const type_info&, type, ) const lnothrow ImplI(IValueHolder)
		ImplRet(type_id<_type>())
	};

	/*!
	\ingroup type_traits_operations
	\brief 指针持有者特征。
	*/
	template<typename _tPointer>
	struct PointerHolderTraits : std::pointer_traits<_tPointer>
	{
		using holder_pointer = _tPointer;
		using shared = leo::is_sharing<holder_pointer>;

		//@{
		static PDefH(size_t, count_owner, const holder_pointer& p_held) lnothrow
			ImplRet(count_owner(shared(), p_held))

	private:
		static PDefH(size_t, count_owner, leo::false_,
			const holder_pointer& p_held) lnothrow
			ImplRet(is_owner(p_held) ? 1 : 0)
			static PDefH(size_t, count_owner, leo::true_,
				const holder_pointer& p_held) lnothrow
			ImplRet(size_t(p_held.use_count()))
			//@}

	public:
		//! \note 使用 ADL get_raw 。
		static PDefH(auto, get, const holder_pointer& p_held)
			lnoexcept_spec(get_raw(p_held)) -> decltype(get_raw(p_held))
			ImplRet(get_raw(p_held))

			//! \note 使用 ADL owns_unique 。
			static PDefH(bool, is_unique_owner, const holder_pointer& p_held) lnothrow
			ImplRet(owns_unique(p_held))

			//! \note 对非内建指针使用 ADL owns_nonnull 。
			static PDefH(bool, is_owner, const holder_pointer& p_held) lnothrow
			ImplRet(is_owner(std::is_pointer<holder_pointer>(), p_held))

	private:
		static PDefH(bool, is_owner, leo::false_, const holder_pointer& p_held)
			lnothrow
			ImplRet(owns_nonnull(p_held))
			static PDefH(bool, is_owner, leo::true_, const holder_pointer& p_held)
			lnothrow
			ImplRet(bool(p_held))
	};

	template<typename _type, class _tTraits = PointerHolderTraits<unique_ptr<_type>>>
	class PointerHolder : implements IValueHolder
	{
		static_assert(leo::is_decayed<_type>(), "Invalid type found.");

	public:
		using value_type = _type;
		using traits = _tTraits;
		using holder_pointer = typename traits::holder_pointer;
		using pointer
			= decltype(traits::get(std::declval<const holder_pointer&>()));
		using shared = typename traits::shared;

	protected:
		holder_pointer p_held;

	public:
		//! \brief 取得所有权。
		explicit
		PointerHolder(pointer value)
			: p_held(value)
		{}
		//@{
		PointerHolder(const holder_pointer& p)
			: p_held(p)
		{}
		PointerHolder(holder_pointer&& p)
			: p_held(std::move(p))
		{}

		PointerHolder(const PointerHolder& h)
			: PointerHolder(h,shared())
		{}
	private:
		PointerHolder(const PointerHolder& h, leo::false_)
			: PointerHolder(leo::clone_monomorphic_ptr(h.p_held))
		{}
		PointerHolder(const PointerHolder& h, leo::true_)
			: p_held(h.p_held)
		{}
	public:
		DefDeMoveCtor(PointerHolder)
			//@}

			DefDeCopyAssignment(PointerHolder)
			DefDeMoveAssignment(PointerHolder)
			
			DefGetter(lnothrow, const holder_pointer&, Held, p_held)

			leo::any
			Create(Creation c) const ImplI(IValueHolder)
		{
			if (shared() && c == IValueHolder::Copy)
				return CreateHolderInPlace<PointerHolder>(shared(), p_held);
			if (const auto& p = traits::get(p_held))
				return CreateHolder(c, *p);
			leo::throw_invalid_construction();
		}

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
			ImplRet(traits::is_owner(p_held) && p
				? AreEqualHeld(Deref(traits::get(p_held)),
					Deref(static_cast<const value_type*>(p))) : !get())

			PDefH(size_t, OwnsCount, ) const lnothrow ImplI(IValueHolder)
			ImplRet(traits::count_owner(p_held))


			PDefH(void*, get, ) const ImplI(IValueHolder)
			ImplRet(traits::get(p_held))

			PDefH(const type_info&, type, ) const lnothrow ImplI(IValueHolder)
			ImplRet(traits::is_owner(p_held)
				? leo::type_id<_type>() : leo::type_id<void>())
	};

	/*!
	\ingroup metafunctions
	\relates PointerHolder
	*/
	template<typename _tPointer>
	using HolderFromPointer = PointerHolder<typename PointerHolderTraits<
		_tPointer>::element_type, PointerHolderTraits<_tPointer>>;

	/*!
	\brief 带等于接口的引用动态泛型持有者。
	\tparam _type 持有的被引用的值类型。
	\note 不对持有值具有所有权。
	\sa ValueHolder
	*/
	template<typename _type>
	class RefHolder : implements IValueHolder
	{
		static_assert(std::is_object<_type>(), "Invalid type found.");

	public:
		using value_type
			= leo::remove_reference_t<leo::unwrap_reference_t<_type>>;

	private:
		ValueHolder<lref<value_type>> base;

	public:
		//! \brief 不取得所有权。
		RefHolder(_type& r)
			: base(r)
		{}
		DefDeCopyMoveCtorAssignment(RefHolder)

		PDefH(leo::any, Create, Creation c) const ImplI(IValueHolder)
		ImplRet(CreateHolder(c, Ref()))

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
		ImplRet(bool(p) && AreEqualHeld(Deref(static_cast<const value_type*>(
			get())), Deref(static_cast<const value_type*>(p))))

		PDefH(size_t, OwnsCount, ) const lnothrow ImplI(IValueHolder)
		ImplRet(0)

	private:
		PDefH(value_type&, Ref, ) const
			ImplRet(Deref(static_cast<lref<value_type>*>(base.get())).get())
	public:
			PDefH(void*, get, ) const ImplI(IValueHolder)
			ImplRet(leo::pvoid(std::addressof(Ref())))

			PDefH(const type_info&, type, ) const lnothrow ImplI(IValueHolder)
			ImplRet(leo::type_id<value_type>())
	};


	template<typename _type>
	leo::any
		IValueHolder::CreateHolder(Creation c, _type& obj)
	{
		switch (c)
		{
		case Indirect:
			return CreateHolderInPlace<RefHolder<_type>>(leo::true_(),
				leo::ref(obj));
		case Copy:
			return CreateHolderInPlace<ValueHolder<_type>>(
				std::is_copy_constructible<_type>(), obj);
		case Move:
			return CreateHolderInPlace<ValueHolder<_type>>(
				std::is_move_constructible<_type>(), std::move(obj));
		default:
			leo::throw_invalid_construction();
		}
	}

	class LF_API ValueObject : private equality_comparable<ValueObject>
	{
	public:
		/*!
		\brief 储存的内容。
		*/
		using Content = leo::any;

	private:

		Content content;

	public:
		/*!
		\brief 无参数构造。
		\note 得到空实例。
		*/
		DefDeCtor(ValueObject)
			/*!
			\brief 构造：使用对象引用。
			\pre obj 可作为转移构造参数。
			*/
			template<typename _type,
			limpl(typename = leo::exclude_self_t<ValueObject, _type>)>
			ValueObject(_type&& obj)
			: content(leo::any_ops::use_holder,
				leo::in_place<ValueHolder<leo::decay_t<_type>>>, lforward(obj))
		{}
		/*!
		\brief 构造：使用对象初始化参数。
		\tparam _type 目标类型。
		\tparam _tParams 目标类型初始化参数类型。
		\pre _type 可被 _tParams 参数初始化。
		*/
		template<typename _type, typename... _tParams>
		ValueObject(leo::in_place_type_t<_type>, _tParams&&... args)
			: content(leo::any_ops::use_holder,
				leo::in_place<ValueHolder<_type>>, lforward(args)...)
		{}

		template<typename _tHolder, typename... _tParams>
		ValueObject(leo::any_ops::use_holder_t,
			leo::in_place_type_t<_tHolder>, _tParams&&... args)
			: content(leo::any_ops::use_holder,
				leo::in_place<_tHolder>, lforward(args)...)
		{}
	private:
		/*!
		\brief 构造：使用持有者。
		*/
		ValueObject(const IValueHolder& holder, IValueHolder::Creation c)
			: content(holder.Create(c))
		{}
	public:
		template<typename _type>
		ValueObject(_type& obj, OwnershipTag<>)
			: content(leo::any_ops::use_holder,
				leo::in_place<RefHolder<_type>>, leo::ref(obj))
		{}

		/*!
		\brief 构造：使用对象指针。
		\note 得到包含指针指向的指定对象的实例，并获得所有权。
		\note 使用 PointerHolder 管理资源（默认使用 delete 释放资源）。
		*/
		template<typename _type>
		ValueObject(_type* p, PointerTag)
			: content(any_ops::use_holder,
				in_place<PointerHolder<_type>>, p)
		{}
		/*!
		\brief 构造：使用对象 unique_ptr 指针。
		\note 得到包含指针指向的指定对象的实例，并获得所有权。
		\note 使用 PointerHolder 管理资源（默认使用 delete 释放资源）。
		*/
		template<typename _type>
		ValueObject(unique_ptr<_type>&& p, PointerTag)
			: ValueObject(p.get(), PointerTag())
		{
			p.release();
		}
		/*!
		\brief 复制构造：默认实现。
		*/
		DefDeCopyCtor(ValueObject)
			/*!
			\brief 转移构造：默认实现。
			*/
			DefDeMoveCtor(ValueObject)
			/*!
			\brief 析构：默认实现。
			*/
			DefDeDtor(ValueObject)

			//@{
			DefDeCopyAssignment(ValueObject)
			DefDeMoveAssignment(ValueObject)
			//@}

			/*!
			\brief 判断是否为空或非空。
			*/
			DefBoolNeg(explicit, content.has_value())

			//@{
			//! \brief 比较相等：参数都为空或都非空且存储的对象相等。
			friend PDefHOp(bool, == , const ValueObject& x, const ValueObject& y)
			ImplRet(x.Equals(y))

		//@{
		//! \brief 比较相等：存储的对象值相等。
		//@}
		//@}
		

		/*!
		\brief 取储存的内容。
		*/
		DefGetter(const lnothrow, const Content&, Content, content)

		IValueHolder*
			GetHolderPtr() const;
		/*!
		\build 取持有者引用。
		\pre 持有者指针非空。
		*/
		IValueHolder&
			GetHolderRef() const;

		/*!
		\brief 取指定类型的对象。
		\pre 间接断言：存储对象类型和访问的类型一致。
		*/
		//@{
		template<typename _type>
		inline _type&
			GetObject() lnothrowv
		{
			return Deref(unchecked_any_cast<_type>(&content));
		}
		template<typename _type>
		inline const _type&
			GetObject() const lnothrowv
		{
			return Deref(unchecked_any_cast<const _type>(&content));
		}
		//@}
		//@}

		/*!
		\brief 访问指定类型对象。
		\exception std::bad_cast 空实例或类型检查失败 。
		*/
		//@{
		template<typename _type>
		inline _type&
			Access()
		{
			return any_cast<_type&>(content);
		}
		template<typename _type>
		inline const _type&
			Access() const
		{
			return any_cast<const _type&>(content);
		}
		//@}

		/*!
		\brief 访问指定类型对象指针。
		*/
		//@{
		template<typename _type>
		inline observer_ptr<_type>
			AccessPtr() lnothrow
		{
			return make_observer(any_cast<_type>(&content));
		}
		template<typename _type>
		inline observer_ptr<const _type>
			AccessPtr() const lnothrow
		{
			return make_observer(any_cast<const _type>(&content));
		}
		//@}

		/*!
		\brief 清除。
		\post <tt>*this == ValueObject()</tt> 。
		*/
		PDefH(void, Clear, ) lnothrow
			ImplExpr(content.reset())

		/*!
		\brief 取自身的复制初始化转移结果：按是否具有唯一所有权选择转移或复制值对象。
		*/
		PDefH(ValueObject, CopyMove, )
			ImplRet(leo::copy_or_move(!OwnsUnique(), *this))

			/*!
			\brief 取以指定持有者选项创建的副本。
			*/
			ValueObject
			Create(IValueHolder::Creation) const;

		/*!
		\brief 判断相等。
		\sa IValueHolder::Equals

		比较参数和持有的对象。
		*/
		//@{
		bool
			Equals(const ValueObject&) const;
		template<typename _type>
		bool
			Equals(const _type& x) const
		{
			if (const auto p_holder = content.get_holder())
				return p_holder->type() == leo::type_id<_type>()
				&& EqualsRaw(std::addressof(x));
			return {};
		}

		//! \pre 参数为空指针值或持有的对象去除 const 后具有和参数相同动态类型的对象。
		bool
			EqualsRaw(const void*) const;

		/*!
		\pre 间接断言：持有的对象非空。
		\pre 持有的对象去除 const 后具有和参数相同动态类型的对象。
		*/
		bool
			EqualsUnchecked(const void*) const;
		//@}

		/*!
		\brief 取引用的值对象的副本。
		*/
		PDefH(ValueObject, MakeCopy, ) const
			ImplRet(Create(IValueHolder::Copy))

			/*!
			\brief 取引用的值对象的转移副本。
			*/
			PDefH(ValueObject, MakeMove, ) const
			ImplRet(Create(IValueHolder::Move))

			/*!
			\brief 取引用的值对象的初始化副本：按是否具有所有权选择转移或复制对象副本。
			*/
			PDefH(ValueObject, MakeMoveCopy, ) const
			ImplRet(Create(OwnsUnique() ? IValueHolder::Move : IValueHolder::Copy))

			/*!
			\brief 取间接引用的值对象。
			*/
			PDefH(ValueObject, MakeIndirect, ) const
			ImplRet(Create(IValueHolder::Indirect))

			/*!
			\brief 取所有者持有的对象的共享所有者的总数。
			*/
			size_t
			OwnsCount() const lnothrow;

		/*!
		\brief 判断是否是持有的对象的唯一所有者。
		*/
		PDefH(bool, OwnsUnique, ) const lnothrow
			ImplRet(OwnsCount() == 1)

		//@{
		template<typename _type, typename... _tParams>
		void
			emplace(_tParams&&... args)
		{
			using Holder = ValueHolder<leo::decay_t<_type>>;

			content.emplace<Holder>(leo::any_ops::use_holder,
				Holder(lforward(args)...));
		}
		template<typename _type>
		void
			emplace(_type* p, PointerTag)
		{
			using Holder = PointerHolder<leo::decay_t<_type>>;

			content.emplace<Holder>(leo::any_ops::use_holder, Holder(p));
		}
		//@}

		/*!
		\brief 交换。
		*/
		friend PDefH(void, swap, ValueObject& x, ValueObject& y) lnothrow
			ImplExpr(x.content.swap(y.content))

		/*!
		\brief 取持有的对象的类型。
		\sa leo::any::type
		*/
		PDefH(const type_info&, type, ) const lnothrow
			ImplRet(content.type())
	};

	template<typename _type>
	inline observer_ptr<_type>
		AccessPtr(ValueObject& vo) lnothrow
	{
		return vo.AccessPtr<_type>();
	}
	template<typename _type>
	inline observer_ptr<const _type>
		AccessPtr(const ValueObject& vo) lnothrow
	{
		return vo.AccessPtr<_type>();
	}

	/*!
	\brief 以指定参数按需构造替换值。

	默认对 ValueObject 及引用值会被直接复制或转移赋值；
	其它情形调用 ValueObject::emplace 。
	使用第三和第四个参数分别指定非默认情形下不忽略值及使用赋值。
	*/
	//@{
	template<typename _type, typename... _tParams>
	void
		EmplaceCallResult(ValueObject&, _type&&, leo::false_) lnothrow
	{}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, leo::true_, leo::true_)
		lnoexcept_spec(vo = lforward(res))
	{
		vo = lforward(res);
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, leo::true_, leo::false_)
	{
		vo.emplace<leo::decay_t<_type>>(lforward(res));
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, leo::true_)
	{
		leo::EmplaceCallResult(vo, lforward(res), leo::true_(),
			std::is_same<leo::decay_t<_type>, ValueObject>());
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res)
	{
		leo::EmplaceCallResult(vo, lforward(res), leo::not_<
			std::is_same<leo::decay_t<_type>, leo::pseudo_output>>());
	}
	//@}

	template<typename _type, typename... _tParams>
	_type&
		EmplaceIfEmpty(ValueObject& vo, _tParams&&... args)
	{
		if (!vo)
		{
			vo.emplace<_type>(lforward(args)...);
			return vo.GetObject<_type>();
		}
		return vo.Access<_type>();
	}

	//! \brief 判断是否持有相同对象。
	inline PDefH(bool, HoldSame, const ValueObject& x, const ValueObject& y)
		ImplRet(leo::hold_same(x.GetContent(), y.GetContent()))
	//@}

	/*!
	\brief 依赖项模板。
	\tparam _type 被依赖的对象类型，需能被无参数构造。
	\tparam _tOwnerPointer 依赖所有者指针类型。
	\warning 依赖所有者指针需要实现所有权语义，
	否则出现无法释放资源引起内存泄漏或其它非预期行为。
	\todo 线程模型及安全性。

	基于被依赖的默认对象，可通过写时复制策略创建新对象；可能为空。
	*/
	template<typename _type, class _tOwnerPointer = shared_ptr<_type>>
	class GDependency
	{
	public:
		using DependentType = _type;
		using PointerType = _tOwnerPointer;
		using ConstReferenceType = identity_t<decltype(*(PointerType()))>;
		using ReferentType = remove_const_t<remove_reference_t<
			ConstReferenceType>>;
		using ReferenceType = ReferentType&;

	private:
		PointerType ptr;

	public:
		inline
			GDependency(PointerType p = PointerType())
			: ptr(p)
		{
			GetCopyOnWritePtr();
		}

		DefDeCopyAssignment(GDependency)
			DefDeMoveAssignment(GDependency)

			DefCvt(const lnothrow, ConstReferenceType, *ptr)
			DefCvt(lnothrow, ReferenceType, *ptr)
			DefCvt(const lnothrow, bool, bool(ptr))

			DefGetter(const lnothrow, ConstReferenceType, Ref,
				operator ConstReferenceType())
			DefGetter(lnothrow, ReferenceType, Ref, operator ReferenceType())
			DefGetter(lnothrow, ReferenceType, NewRef, *GetCopyOnWritePtr())

			//! \post 返回值非空。
			PointerType
			GetCopyOnWritePtr()
		{
			if (!ptr)
				ptr = PointerType(new DependentType());
			else if (!ptr.unique())
				ptr = PointerType(leo::clone_monomorphic(Deref(ptr)));
			return Nonnull(ptr);
		}

		inline
			void Reset()
		{
			reset(ptr);
		}
	};

	template<typename _type>
	class GMRange
	{
	public:
		using ValueType = _type;

	protected:
		ValueType max_value; //!< 最大取值。
		ValueType value; //!< 值。

		/*!
		\brief 构造：使用指定最大取值和值。
		*/
		GMRange(ValueType m, ValueType v)
			: max_value(m), value(v)
		{}

	public:
		DefGetter(const lnothrow, ValueType, MaxValue, max_value)
			DefGetter(const lnothrow, ValueType, Value, value)
	};

}

#endif
