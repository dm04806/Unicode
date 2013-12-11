/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

/*
 * REQUIREMENTS:
 * - THE Unicode REPOSITORY SHOULD BE PLACED UNDER $CINDER/samples
 * - DEPENDS ON FREETYPE (TO BE PLACED UNDER $CINDER/blocks): https://github.com/arielm/Freetype
 * - INCLUDES HARFBUZZ 0.9.24 (https://github.com/behdad/harfbuzz)
 *   THE "HAVE_UCDN" AND "HAVE_OT" MACROS MUST BE DEFINED
 *
 * OSX:
 * COMPILED ON OSX 10.5.8 WITH XCode 5.0.2 AND CINDER "VANILLA" 0.8.5
 *
 * WINDOWS:
 * SHOULD BE TRIVIALLY ADAPTABLE TO VISUAL STUDIO 2012
 */

/*
 * DRAG-AND-DROP SOME "XML DIRECTIVE":
 * SEE assets/directives
 *
 * THE "font" ATTRIBUTE SHOULD POINT TO A FONT-FILE ON YOUR DISK, E.G.
 * "/Library/Fonts/AdobeHebrew-Regular.otf"
 *
 * OR TO ONE OF THE FILES IN assets/fonts, E.G.
 * "fonts/DroidSansFallback.ttf"
 *
 * THE "script" ATTRIBUTE SHOULD BE ONE OF:
 * https://github.com/behdad/harfbuzz/blob/0.9.24/src/hb-common.h#L151-274
 * http://unicode.org/iso15924/iso15924-codes.html
 */

/*
 * OR  DRAG-AND-DROP SOME FONT-FILE (.TTF OR .OTF)
 *
 * IF YOU CAN'T SEE ANYTHING DISPLAYED, IT'S LIKELY BECAUSE THE FONT LACKS THE
 * PROPER UNICODE CHARACTERS (WE'RE NOT YET HANDLING FONT-SUBSTITUTION)
 */
// RUNNING THE FOLLOWING IN OSX TERMINAL WILL PLACE SYMLINKS OF YOUR SYSTEM FONTS IN ~/Documents/Fonts
// ln -s /Library/Fonts/* ~/Documents/Fonts/
// ln -s /System/Library/Fonts/* ~/Documents/Fonts/

#include "cinder/app/AppNative.h"

#include "YFont.h"
#include "Directive.h"

using namespace std;
using namespace ci;
using namespace app;

const float FONT_SIZE = 48;
const float Y0 = 80;
const float LINE_H = 96;

class Application : public AppNative
{
    shared_ptr<FreetypeHelper> ftHelper; // THE UNDERLYING FT_Library WILL BE DESTROYED AFTER ALL THE YFont INSTANCES
    
    shared_ptr<Directive> currentDirective;
    shared_ptr<YFont> currentFont;
    vector<LineLayout> lineLayouts;
   
public:
    void prepareSettings(Settings *settings);
    void setup();
    
    void draw();
    void drawLineLayout(YFont &font, const LineLayout &layout, float y);
    void drawLine(float y);
    
    void fileDrop(FileDropEvent event);
    void applyDirective(shared_ptr<Directive> directive);
};

void Application::prepareSettings(Settings *settings)
{
    settings->setWindowSize(1280, 720);
}

void Application::setup()
{
    ftHelper = make_shared<FreetypeHelper>();
    
    applyDirective(make_shared<Directive>("drop xml directive"));
//  applyDirective(make_shared<Directive>(loadAsset("directives/Hebrew1_osx.xml")));
    
    // ---
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
}

void Application::draw()
{
    gl::clear(Color::gray(0.5f), false);
    gl::setMatricesWindow(toPixels(getWindowSize()), true);
    
    // ---
    
    float y = Y0;
    
    for (auto layout : lineLayouts)
    {
        drawLineLayout(*currentFont, layout, y);
        y += LINE_H;
    }
}

void Application::drawLineLayout(YFont &font, const LineLayout &layout, float y)
{
    glColor4f(1, 1, 1, 1);
    font.drawLayout(layout, Vec2f(24, y));
    
    glColor4f(1, 0.75f, 0, 0.5f);
    drawLine(y);
    
    glColor4f(1, 1, 0, 0.33f);
    drawLine(y - font.ascent);
    drawLine(y + font.descent);
}

void Application::drawLine(float y)
{
    gl::drawLine(Vec2f(-9999, y), Vec2f(+9999, y));
}

void Application::fileDrop(FileDropEvent event)
{
    if (event.getNumFiles() == 1)
    {
        auto file = event.getFile(0);
        auto extension = file.extension();
        
        if (!is_directory(file))
        {
            if (extension == ".xml")
            {
                try
                {
                    applyDirective(make_shared<Directive>(loadFile(file)));
                }
                catch (exception &e)
                {
                    applyDirective(make_shared<Directive>(e));
                }
            }
            else if ((extension == ".ttf") || (extension == ".otf"))
            {
                if (currentDirective)
                {
                    applyDirective(make_shared<Directive>(file, *currentDirective));
                }
            }
        }
    }
}

void Application::applyDirective(shared_ptr<Directive> directive)
{
    currentDirective = directive;
    currentFont = make_shared<YFont>(ftHelper, directive->fontPath, FONT_SIZE);
    
    lineLayouts.clear();

    for (auto line : directive->lines)
    {
        lineLayouts.push_back(currentFont->createLayout(TextSpan(line, directive->script, directive->direction)));
    }
    
    getWindow()->setTitle(currentFont->getName());
}

CINDER_APP_NATIVE(Application, RendererGl(RendererGl::AA_NONE))