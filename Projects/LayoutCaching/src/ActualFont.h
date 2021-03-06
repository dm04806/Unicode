/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

#pragma once

#include "GlyphData.h"
#include "ReloadableTexture.h"

#include "chronotext/InputSource.h"

#include "hb.h"

#include <map>

class ActualFont
{
public:
    struct Glyph
    {
        ReloadableTexture *texture;
        ci::Vec2f offset;
        ci::Vec2f size;
        
        float tx1;
        float ty1;
        float tx2;
        float ty2;
        
        Glyph()
        :
        texture(NULL)
        {}
        
        Glyph(ReloadableTexture *texture, ci::Vec2f offset, ci::Vec2f size)
        :
        texture(texture),
        offset(offset),
        size(size)
        {
            tx1 = 0;
            ty1 = 0;
            tx2 = size.x / texture->getWidth();
            ty2 = size.y / texture->getHeight();
        }
    };
    
    struct Metrics
    {
        float height;
        float ascent;
        float descent;
        float strikethroughOffset;
        float underlineOffset;
        float lineThickness;
    };

    float baseSize;
    bool useMipmap;
    int padding;
    
    ci::Vec2f scale;
    Metrics metrics;
    hb_font_t *hbFont;
    
    ActualFont(std::shared_ptr<FreetypeHelper> ftHelper, chr::InputSourceRef source, float baseSize, bool useMipmap, int padding);
    ~ActualFont();
    
    Glyph* getGlyph(uint32_t codepoint);
    void clearGlyphCache();
    void unloadTextures();
    
protected:
    std::shared_ptr<FreetypeHelper> ftHelper;
    FT_Face ftFace;
    
    std::map<uint32_t, std::unique_ptr<Glyph>> glyphCache;
    std::vector<std::unique_ptr<ReloadableTexture>> standaloneTextures;
    
    Glyph* createGlyph(uint32_t codepoint);
};
