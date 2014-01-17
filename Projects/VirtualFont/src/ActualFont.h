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
    struct Descriptor
    {
        chr::InputSourceRef source;
        int faceIndex;
        bool forceMemoryLoad;
        
        Descriptor()
        {}
        
        Descriptor(chr::InputSourceRef source, int faceIndex = 0, bool forceMemoryLoad = false)
        :
        source(source),
        faceIndex(faceIndex),
        forceMemoryLoad(forceMemoryLoad)
        {}
        
        bool empty()
        {
            return !bool(source);
        }
    };

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

    ci::Vec2f scale;
    Metrics metrics;
    
    ActualFont(std::shared_ptr<FreetypeHelper> ftHelper, const Descriptor &descriptor, float baseSize, bool useMipmap);
    ~ActualFont();

    void reload();
    void unload();
    void discardTextures();
    
    Glyph* getGlyph(uint32_t codepoint);
    
protected:
    std::shared_ptr<FreetypeHelper> ftHelper;

    Descriptor descriptor;
    float baseSize;
    bool useMipmap;
    int padding;

    ci::Buffer memoryBuffer;
    FT_Face ftFace;
    hb_font_t *hbFont;
    bool loaded;
    
    std::map<uint32_t, std::unique_ptr<Glyph>> glyphCache;
    std::vector<std::unique_ptr<ReloadableTexture>> standaloneTextures;
    
    Glyph* createGlyph(uint32_t codepoint);
    
    friend class VirtualFont;
};
