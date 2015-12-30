////////////////////////////////////////////////////////////////////////////
//
//  Leo Engine Source File.
//  Copyright (C), FNS Studios, 2014-2015.
// -------------------------------------------------------------------------
//  File name:   HUD/Label.hpp
//  Version:     v1.00
//  Created:     11/24/2015 by leo hawke.
//  Compilers:   Visual Studio.NET 2015
//  Description: HUD标签
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef HUD_LABEL_H
#define HUD_LABEL_H

#include "Widget.h"
#include "HUDBrush.h"
#include "..\UI\Font.hpp"
#include "..\UI\TextBase.h"

LEO_BEGIN

HUD_BEGIN

using String = Drawing::String;

//文本对齐样式
enum class TextAlignment
{
	Left = 1,
	Center = 2,
	Right = 3,
	Up = 4,
	Down = 5
};

class LB_API MLabel {
public:
	/*!
	\brief 当前使用的剪切文本更新器。
	\note 第三个模板参数表示是否自动换行。
	\note 被 DrawText 调用。
	*/
	GBrushUpdater<Drawing::TextState&, const String&, bool>
		UpdateClippedText{ DefaultUpdateClippedText };

	Drawing::Color ForeColor{ Drawing::ColorSpace::Black };
	Drawing::Font Font;
	Drawing::Padding Margin{ Drawing::DefaultMargin };

	TextAlignment HorizontalAlignment,
		VerticalAlignment = TextAlignment::Center;

	/*!
	\brief 启用自动换行。
	*/
	bool AutoWrapLine = {};

	String Text{};

	

	explicit
	MLabel(const Drawing::Font& = {},Drawing::Color = Drawing::ColorSpace::Black,
			TextAlignment = TextAlignment::Left);
	DefDeMoveCtor(MLabel)

	/*!
	\brief 描画：使用发送者的大小并调用 DrawText 绘制文本。
	*/
	void
	operator()(PaintEventArgs&&) const;

	/*!
	\brief 按参数指定的边界大小和当前状态计算笔的偏移位置。
	*/
	Point
	GetAlignedPenOffset(const Size&) const;

	/*!
	\brief 绘制文本。
	\sa AlignPen
	\sa DrawClipText
	*/
	void
	DrawText(const Size&, const PaintContext&) const;

	/*!
	\brief 绘制剪切文本：使用指定的绘制上下文和文本状态。
	*/
	static void
		DefaultUpdateClippedText(const PaintContext&, Drawing::TextState&,
			const String&, bool);
	//@}
};

class LB_API Label : public Widget, protected MLabel
{
public:
	using MLabel::Font;
	using MLabel::Margin;
	using MLabel::HorizontalAlignment;
	using MLabel::VerticalAlignment;
	using MLabel::Text;

	explicit
		Label(const Rect& r = {}, const Drawing::Font& fnt = {},
			HBrush b = MakeAlphaBrush())
		: Widget(r, b), MLabel(fnt)
	{}
	DefDeMoveCtor(Label)

	/*!
	\brief 计算指定字符串、初始边界、字体的单行文本需要的标签边界。
	\note 仅当初始边界的大小为 Size::Invalid 时计算新的大小。
	*/
	static Rect
	CalculateBounds(const String&, Rect, const Drawing::Font&,const Drawing::Padding& m);


	/*!
	\brief 刷新：按指定参数绘制界面并更新状态。
	\since build 294
	*/
	void
	Refresh(PaintEventArgs&&) override;
};


LB_API std::unique_ptr<Label>
MakeLabel(const String&, const Rect& = Rect::Invalid, const Drawing::Font& = {}, const Drawing::Padding& = Drawing::DefaultMargin * 2);

HUD_END

LEO_END


#endif
