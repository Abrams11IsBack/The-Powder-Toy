#include "ScrollPanel.h"
#include "Engine.h"

#include "graphics/Graphics.h"

#include "common/tpt-minmax.h"

using namespace ui;

ScrollPanel::ScrollPanel(Point position, Point size):
	Panel(position, size),
	scrollBarWidth(0),
	maxOffset(0, 0),
	offsetX(0),
	offsetY(0),
	yScrollVel(0.0f),
	xScrollVel(0.0f),
	isMouseInsideScrollbar(false),
	isMouseInsideScrollbarArea(false),
	scrollbarSelected(false),
	scrollbarInitialYOffset(0),
	scrollbarInitialYClick(0),
	scrollbarClickLocation(0)
{
}

int ScrollPanel::GetScrollLimit()
{
	if (ViewportPosition.Y == 0)
		return -1;
	else if (maxOffset.Y == -ViewportPosition.Y)
		return 1;
	return 0;
}

void ScrollPanel::SetScrollPosition(int position)
{
	offsetY = float(position);
	ViewportPosition.Y = -position;
}

void ScrollPanel::XOnMouseWheelInside(int localx, int localy, int d)
{
	if (!d)
		return;
	if (ui::Engine::Ref().MomentumScroll)
		yScrollVel -= d * 2;
	else
		yScrollVel -= d * 20;
}

void ScrollPanel::Draw(const Point& screenPos)
{
	Panel::Draw(screenPos);

	Graphics * g = GetGraphics();

	//Vertical scroll bar
	if (maxOffset.Y>0 && InnerSize.Y>0)
	{
		float scrollHeight = float(Size.Y)*(float(Size.Y)/float(InnerSize.Y));
		float scrollPos = 0;
		if (-ViewportPosition.Y>0)
		{
			scrollPos = float(Size.Y-scrollHeight)*(float(offsetY)/float(maxOffset.Y));
		}

		g->BlendFilledRect(RectSized(screenPos + Vec2{ Size.X - scrollBarWidth, 0 }, { scrollBarWidth, Size.Y }), 0x7D7D7D_rgb .WithAlpha(100));
		g->DrawFilledRect(RectSized(screenPos + Vec2{ Size.X - scrollBarWidth, int(scrollPos) }, { scrollBarWidth, int(scrollHeight)+1 }), 0xFFFFFF_rgb);
	}
}

void ScrollPanel::XOnMouseClick(int x, int y, unsigned int button)
{
	if (isMouseInsideScrollbar)
	{
		scrollbarSelected = true;
		scrollbarInitialYOffset = int(offsetY);
	}
	scrollbarInitialYClick = y;
	scrollbarClickLocation = 100;
}

void ScrollPanel::XOnMouseUp(int x, int y, unsigned int button)
{
	scrollbarSelected = false;
	isMouseInsideScrollbarArea = false;
	scrollbarClickLocation = 0;
}

void ScrollPanel::XOnMouseMoved(int x, int y, int dx, int dy)
{
	if(maxOffset.Y>0 && InnerSize.Y>0)
	{
		float scrollHeight = float(Size.Y)*(float(Size.Y)/float(InnerSize.Y));
		float scrollPos = 0;
		if (-ViewportPosition.Y>0)
		{
			scrollPos = float(Size.Y-scrollHeight)*(float(offsetY)/float(maxOffset.Y));
		}

		if (scrollbarSelected)
		{
			if (x > 0)
			{
				auto scrollY = int(float(y-scrollbarInitialYClick)/float(Size.Y)*float(InnerSize.Y)+scrollbarInitialYOffset);
				ViewportPosition.Y = -scrollY;
				offsetY = float(scrollY);
			}
			else
			{
				ViewportPosition.Y = -scrollbarInitialYOffset;
				offsetY = float(scrollbarInitialYOffset);
			}
		}

		if (x > (Size.X-scrollBarWidth) && x < (Size.X-scrollBarWidth)+scrollBarWidth)
		{
			if (y > scrollPos && y < scrollPos+scrollHeight)
				isMouseInsideScrollbar = true;
			isMouseInsideScrollbarArea = true;
		}
		else
			isMouseInsideScrollbar = false;
	}
}

void ScrollPanel::XTick(float dt)
{
	if (xScrollVel > 7.0f) xScrollVel = 7.0f;
	if (xScrollVel < -7.0f) xScrollVel = -7.0f;
	if (xScrollVel > -0.5f && xScrollVel < 0.5)
		xScrollVel = 0;

	maxOffset = InnerSize-Size;
	maxOffset.Y = std::max(0, maxOffset.Y);
	maxOffset.X = std::max(0, maxOffset.X);

	auto oldOffsetY = int(offsetY);
	offsetY += yScrollVel;
	offsetX += xScrollVel;


	if (ui::Engine::Ref().MomentumScroll)
	{
		if (yScrollVel > -0.5f && yScrollVel < 0.5)
			yScrollVel = 0;
		yScrollVel *= 0.98f;
	}
	else
	{
		yScrollVel = 0.0f;
	}

	xScrollVel*=0.98f;

	if (oldOffsetY!=int(offsetY))
	{
		if (offsetY<0)
		{
			offsetY = 0;
			yScrollVel = 0;
		}
		else if (offsetY>maxOffset.Y)
		{
			offsetY = float(maxOffset.Y);
			yScrollVel = 0;
		}
		ViewportPosition.Y = -int(offsetY);
	}
	else
	{
		if (offsetY<0)
		{
			offsetY = 0;
			yScrollVel = 0;
			ViewportPosition.Y = -int(offsetY);
		}
		else if (offsetY>maxOffset.Y)
		{
			offsetY = float(maxOffset.Y);
			ViewportPosition.Y = -int(offsetY);
		}
	}

	if (mouseInside && scrollBarWidth < 6)
		scrollBarWidth++;
	else if (!mouseInside && scrollBarWidth > 0 && !scrollbarSelected)
		scrollBarWidth--;

	if (isMouseInsideScrollbarArea && scrollbarClickLocation && !scrollbarSelected)
	{
		float scrollHeight = float(Size.Y)*(float(Size.Y)/float(InnerSize.Y));
		float scrollPos = 0;
		if (-ViewportPosition.Y > 0)
			scrollPos = float(Size.Y-scrollHeight)*(float(offsetY)/float(maxOffset.Y));

		if (scrollbarInitialYClick <= scrollPos)
			scrollbarClickLocation = -1;
		else if (scrollbarInitialYClick >= scrollPos+scrollHeight)
			scrollbarClickLocation = 1;
		else
			scrollbarClickLocation = 0;

		offsetY += scrollbarClickLocation*scrollHeight/10;
		ViewportPosition.Y -= int(scrollbarClickLocation*scrollHeight/10);
	}
}
