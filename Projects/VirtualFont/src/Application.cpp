/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

/*
 * FEATURES:
 *
 * 1) DISPLAYING TEXT IN ANY LANGUAGE, AS LONG AS THE PROPER LIST OF FONTS
 *    IS DEFINED FOR THE LANGUAGE IN QUESTION IN THE VIRTUAL-FONT XML
 *
 * 2) USING TEXT FROM pango-language-sample-table.h
 *
 *
 * TODO:
 *
 * 1) CREATE VIRTUAL-FONT FOR OSX AND iOS
 *    AND CREATE iOS AND ANDROID PROJECTS
 *
 * 2) START TO MEASURE PERFORMANCE ON iOS AND ANDROID:
 *    - SHAPING
 *    - RENDERING
 *
 * 3) PROVIDE "METRICS" PER FONT-SET:
 *    - ASCENT, DESCENT, HEIGHT AND STRIKETHROUGH OFFSET:
 *      BASED ON FIRST FONT IN SET
 */

#include "cinder/app/AppNative.h"
#include "cinder/Xml.h"
#include "cinder/Utilities.h"

#include "TextLayout.h"
#include "FontManager.h"
#include "LanguageHelper.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace ci;
using namespace app;

const float FONT_SIZE = 32;
const float LINE_TOP = 66;
const float LINE_SPACING = 66;

class Application : public AppNative
{
    LanguageHelper languageHelper;
    FontManager fontManager;
    
    vector<TextLayout> lineLayouts;
    
public:
    void prepareSettings(Settings *settings);
    void setup();
    
    void draw();
    void drawLineLayout(TextLayout &layout, float y, float left, float right);
    void drawHLine(float y);
    
    TextLayout createLayout(VirtualFont &font, const string &text, const string &lang) const;
    TextSpan createRun(const string &text, const string &lang) const;
    string trimText(const string &text) const;
};

void Application::prepareSettings(Settings *settings)
{
    settings->setWindowSize(1280, 736);
}

void Application::setup()
{
    auto sansSerifFont = fontManager.loadVirtualFont(loadResource("SansSerif.xml"), FONT_SIZE);
    
    XmlTree doc(loadResource("Text.xml"));
    auto rootElement = doc.getChild("Text");
    
    for (auto &lineElement : rootElement.getChildren())
    {
        auto lang =lineElement->getAttributeValue<string>("lang");
        auto text = trimText(lineElement->getValue());
        
        lineLayouts.emplace_back(createLayout(sansSerifFont, text, lang));
    }

    // ---
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
}

void Application::draw()
{
    gl::clear(Color::gray(0.5f), false);
    
    Vec2i windowSize = toPixels(getWindowSize());
    gl::setMatricesWindow(windowSize, true);

    // ---

    float y = LINE_TOP;
    float left = 24;
    float right = windowSize.x - 24;
    
    for (auto layout : lineLayouts)
    {
        drawLineLayout(layout, y, left, right);
        y += LINE_SPACING;
    }
}

void Application::drawLineLayout(TextLayout &layout, float y, float left, float right)
{
    float x = (layout.direction == HB_DIRECTION_LTR) ? left : (right - layout.advance);
    
    glColor4f(1, 1, 1, 1);
    layout.draw(Vec2f(x, y));
    
    glColor4f(1, 0.75f, 0, 0.5f);
    drawHLine(y);
}

void Application::drawHLine(float y)
{
    gl::drawLine(Vec2f(-9999, y), Vec2f(+9999, y));
}

TextSpan Application::createRun(const string &text, const string &lang) const
{
    auto script = languageHelper.getScript(lang);
    auto direction = hb_script_get_horizontal_direction(script);
    
    return TextSpan(text, script, lang, direction);
}

TextLayout Application::createLayout(VirtualFont &font, const string &text, const string &lang) const
{
    return TextLayout(font.getFontSet(lang), createRun(text, lang));
}

string Application::trimText(const string &text) const
{
    auto rawLines = split(text, '\n');
    
    for (auto line : rawLines)
    {
        auto trimmed = boost::algorithm::trim_copy(line);
        
        if (!trimmed.empty())
        {
            return trimmed;
        }
    }
    
    return "";
}

CINDER_APP_NATIVE(Application, RendererGl(RendererGl::AA_NONE))