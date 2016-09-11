/*!	\file LAdaptor.h
\ingroup Core
\brief �ⲿ�������
*/

#ifndef LEO_Core_LAdaptor_h
#define LEO_Core_LAdaptor_h 1

#include<LBase/algorithm.hpp>
#include<LBase/functional.hpp>
#include<LBase/Debug.h>
#include<LBase/Mutex.h>
#include<LBase/FReference.h>
#include<LBase/FFileIO.h>

namespace leo {

	using platform::basic_string;

	using platform::string;
	using platform::wstring;
	using platform::sfmt;
	using platform::vsfmt;

	using platform::to_string;
	using platform::to_wstring;

	using platform::basic_string_view;
	using platform::string_view;
	using platform::u16string_view;
	using platform::wstring_view;

	using platform::bad_weak_ptr;
	using platform::const_pointer_cast;
	using platform::dynamic_pointer_cast;
	using platform::enable_shared_from_this;
	using platform::get_deleter;

	using platform::make_observer;
	using platform::make_shared;
	using platform::make_unique;
	using platform::make_unique_default_init;
	using platform::get_raw;
	using platform::observer_ptr;
	using platform::owner_less;
	using platform::reset;
	using platform::share_raw;
	using platform::shared_ptr;
	using platform::static_pointer_cast;
	using platform::unique_raw;
	using platform::unique_ptr;
	using platform::unique_ptr_from;
	using platform::weak_ptr;

	using platform::lref;

	//@{
	using platform::nptr;
	using platform::tidy_ptr;
	using platform::pointer_iterator;
	//@}

	/*!
	\brief ����ɾ������ʹ���߳�ģ�Ͷ�Ӧ�Ļ�����������
	*/
	using platform::Threading::unlock_delete;


	/*!
	\brief ����������
	*/
	using namespace platform::Concurrency;
	/*!
	\brief ƽ̨����������
	*/
	using namespace platform::Descriptions;

	/*!
	\brief ����ʵ�����̡�
	*/
	//@{
	using platform::usystem;
	//@}

	/*!
	\brief ��־��
	*/
	//@{
	using platform::Logger;
	using platform::FetchCommonLogger;
	//@}

	/*!
	\brief ���Բ�����
	\pre �����Բ�����ʹ�� ADL ����ͬ����������Ҫ�����յı���ʽ����͵�������������ȼۡ�
	*/
	using platform::Nonnull;

	using platform::FwdIter;
	using platform::Deref;

	/*!
	\brief �ļ��������̡�
	*/
	//@{
	using platform::upclose;
	using platform::upopen;
	//@}
}
#endif