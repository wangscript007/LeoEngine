#ifndef UI_UI_h
#define UI_UI_h

#include <ldef.h>
#include <BaseMacro.h>
#include <leoint.hpp>
#include <LAssert.h>
#include <base.h>
#include <utility.hpp>
#include <functional.hpp>
#include <leoint.hpp>
#include <iterator.hpp>
#include <exception.hpp>
#include <bitseg.hpp>
#include <iterator_op.hpp>
#include <string.hpp>
#include <cctype.h>
#include <cwctype.h>

#include <algorithm>


#ifndef UI_BEGIN
//UI包括2D UI和3D UI，主要用于游戏特定UI
#define UI_BEGIN namespace UI{
#define UI_END }
#define LEO_UI_BEGIN LEO_BEGIN UI_BEGIN
#define LEO_UI_END LEO_END UI_END
//Drawing 为HUD和UI公用部分
#define DRAW_BEGIN namespace Drawing{
#define DRAW_END }
#define LEO_DRAW_BEGIN LEO_BEGIN DRAW_BEGIN
#define LEO_DRAW_END LEO_END DRAW_END
#endif

LEO_DRAW_BEGIN

class Font;
struct Padding;
struct Size;
struct Rect;
class Color;

LEO_DRAW_END

#endif
