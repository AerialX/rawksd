/****************************************************************************
 * libwiigui
 *
 * Tantric 2009
 *
 * gui_text.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"

static int currentSize = 0;
static int presetSize = 0;
static int presetMaxWidth = 0;
static int presetAlignmentHor = 0;
static int presetAlignmentVert = 0;
static u16 presetStyle = 0;
static GXColor presetColor = (GXColor){255, 255, 255, 255};

#define TEXT_SCROLL_DELAY			8
#define	TEXT_SCROLL_INITIAL_DELAY	6

/**
 * Constructor for the GuiText class.
 */
GuiText::GuiText(const char * t, int s, GXColor c)
{
	origText = NULL;
	size = s;
	color = c;
	alpha = c.a;
	style = FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE;
	maxWidth = 0;
	wrap = false;
	textDyn = NULL;
	textScroll = SCROLL_NONE;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
	textScrollDelay = TEXT_SCROLL_DELAY;

	alignmentHor = ALIGN_CENTRE;
	alignmentVert = ALIGN_MIDDLE;

	if(t && t[0])
		origText = charToWideChar(t);
}

GuiText::GuiText(const s16 * t, int s, GXColor c)
{
	origText = NULL;
	size = s;
	color = c;
	alpha = c.a;
	style = FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE;
	maxWidth = 0;
	wrap = false;
	textDyn = NULL;
	textScroll = SCROLL_NONE;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
	textScrollDelay = TEXT_SCROLL_DELAY;

	alignmentHor = ALIGN_CENTRE;
	alignmentVert = ALIGN_MIDDLE;

	if(t && t[0])
		origText = shortToWideChar(t);
}

/**
 * Constructor for the GuiText class, uses presets
 */
GuiText::GuiText(const char * t)
{
	origText = NULL;
	size = presetSize;
	color = presetColor;
	alpha = presetColor.a;
	style = presetStyle;
	maxWidth = presetMaxWidth;
	wrap = false;
	textDyn = NULL;
	textScroll = SCROLL_NONE;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
	textScrollDelay = TEXT_SCROLL_DELAY;

	alignmentHor = presetAlignmentHor;
	alignmentVert = presetAlignmentVert;

	if(t && t[0])
		origText = charToWideChar(t);
}

GuiText::GuiText(const s16 * t)
{
	origText = NULL;
	size = presetSize;
	color = presetColor;
	alpha = presetColor.a;
	style = presetStyle;
	maxWidth = presetMaxWidth;
	wrap = false;
	textDyn = NULL;
	textScroll = SCROLL_NONE;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
	textScrollDelay = TEXT_SCROLL_DELAY;

	alignmentHor = presetAlignmentHor;
	alignmentVert = presetAlignmentVert;

	if(t && t[0])
		origText = shortToWideChar(t);
}

/**
 * Destructor for the GuiText class.
 */
GuiText::~GuiText()
{
	delete[] origText;
	free(textDyn);
}

void GuiText::SetText(const char * t)
{
	delete[] origText;
	free(textDyn);

	origText = NULL;
	textDyn = NULL;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;

	if(t)
		origText = charToWideChar(t);
}

void GuiText::SetText(const s16 * t)
{
	delete[] origText;
	free(textDyn);

	origText = NULL;
	textDyn = NULL;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;

	if(t)
		origText = shortToWideChar(t);
}

void GuiText::SetPresets(int sz, GXColor c, int w, u16 s, int h, int v)
{
	presetSize = sz;
	presetColor = c;
	presetStyle = s;
	presetMaxWidth = w;
	presetAlignmentHor = h;
	presetAlignmentVert = v;
}

void GuiText::SetFontSize(int s)
{
	size = s;
}

void GuiText::SetMaxWidth(int width)
{
	maxWidth = width;
}

void GuiText::SetWrap(bool w, int width)
{
	wrap = w;
	maxWidth = width;
}

void GuiText::SetScroll(int s)
{
	if(textScroll == s)
		return;

	free(textDyn);
	textDyn = NULL;

	textScroll = s;
	textScrollPos = 0;
	textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
	textScrollDelay = TEXT_SCROLL_DELAY;
}

void GuiText::SetColor(GXColor c)
{
	color = c;
	alpha = c.a;
}

void GuiText::SetStyle(u16 s)
{
	style = s;
}

void GuiText::SetAlignment(int hor, int vert)
{
	style = 0;

	switch(hor)
	{
		case ALIGN_LEFT:
			style |= FTGX_JUSTIFY_LEFT;
			break;
		case ALIGN_RIGHT:
			style |= FTGX_JUSTIFY_RIGHT;
			break;
		default:
			style |= FTGX_JUSTIFY_CENTER;
			break;
	}
	switch(vert)
	{
		case ALIGN_TOP:
			style |= FTGX_ALIGN_TOP;
			break;
		case ALIGN_BOTTOM:
			style |= FTGX_ALIGN_BOTTOM;
			break;
		default:
			style |= FTGX_ALIGN_MIDDLE;
			break;
	}

	alignmentHor = hor;
	alignmentVert = vert;
}

/**
 * Draw the text on screen
 */
void GuiText::Draw()
{
	if(!origText)
		return;

	if(!IsVisible())
		return;

	GXColor c = color;
	c.a = this->GetAlpha();

	int newSize = size*GetScale();

	if(newSize > MAX_FONT_SIZE)
		newSize = MAX_FONT_SIZE;

	if(newSize != currentSize)
	{
		ChangeFontSize(newSize);
		if(!fontSystem[newSize])
			fontSystem[newSize] = new FreeTypeGX(newSize);
		currentSize = newSize;
	}

	if(maxWidth > 0)
	{
		wchar_t * tmpText = wcsdup(origText);
		u8 maxChar = (maxWidth*2.0) / newSize;

		if(!textDyn)
		{
			if(wcslen(tmpText) > maxChar)
				tmpText[maxChar] = L'\0';
			textDyn = wcsdup(tmpText);
		}

		if(textScroll == SCROLL_HORIZONTAL)
		{
			int textlen = wcslen(origText);

			if(textlen > maxChar && (FrameTimer % textScrollDelay == 0))
			{
				if(textScrollInitialDelay)
				{
					textScrollInitialDelay--;
				}
				else
				{
					textScrollPos++;
					if(textScrollPos > textlen-1)
					{
						textScrollPos = 0;
						textScrollInitialDelay = TEXT_SCROLL_INITIAL_DELAY;
					}

					wcsncpy(tmpText, &origText[textScrollPos], maxChar-1);
					tmpText[maxChar-1] = L'\0';

					int dynlen = wcslen(tmpText);

					if(dynlen+2 < maxChar)
					{
						tmpText[dynlen] = L' ';
						tmpText[dynlen+1] = L' ';
						wcsncat(&tmpText[dynlen+2], origText, maxChar - dynlen - 2);
					}
					free(textDyn);
					textDyn = wcsdup(tmpText);
				}
			}
			if(textDyn)
				fontSystem[currentSize]->drawText(this->GetLeft(), this->GetTop(), textDyn, c, style);
		}
		else if(wrap)
		{
			int lineheight = newSize + 6;
			int txtlen = wcslen(origText);
			int i = 0;
			int ch = 0;
			int linenum = 0;
			int lastSpace = -1;
			int lastSpaceIndex = -1;
			wchar_t * textrow[20];

			while(ch < txtlen)
			{
				if(i == 0)
					textrow[linenum] = new wchar_t[txtlen + 1];

				if (origText[ch] == L'\n') {
					textrow[linenum][i] = L'\0';
					i = 0;
					linenum++;
					ch++;
					continue;
				}

				textrow[linenum][i] = origText[ch];
				textrow[linenum][i+1] = L'\0';

				if(origText[ch] == L' ' || ch == txtlen-1)
				{
					if(wcslen(textrow[linenum]) >= maxChar)
					{
						if(lastSpace >= 0)
						{
							textrow[linenum][lastSpaceIndex] = 0; // discard space, and everything after
							ch = lastSpace; // go backwards to the last space
							lastSpace = -1; // we have used this space
							lastSpaceIndex = -1;
						}
						linenum++;
						i = -1;
					}
					else if(ch == txtlen-1)
					{
						linenum++;
					}
				}
				if(origText[ch] == L' ' && i >= 0)
				{
					lastSpace = ch;
					lastSpaceIndex = i;
				}
				ch++;
				i++;
			}

			int voffset = 0;

			if(alignmentVert == ALIGN_MIDDLE)
				voffset = -(lineheight*linenum)/2 + lineheight/2;

			for(i=0; i < linenum; i++)
			{
				fontSystem[currentSize]->drawText(GetLeft(), GetTop()+voffset+i*lineheight, textrow[i], c, style);
				delete[] textrow[i];
			}
		}
		else
		{
			fontSystem[currentSize]->drawText(GetLeft(), GetTop(), textDyn, c, style);
		}
		free(tmpText);
	}
	else
	{
		fontSystem[currentSize]->drawText(GetLeft(), GetTop(), origText, c, style);
	}
	UpdateEffects();
}
