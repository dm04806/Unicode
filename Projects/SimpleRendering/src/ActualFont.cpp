/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

#include "ActualFont.h"

#include "hb-ft.h"

#include "cinder/Utilities.h"

using namespace std;
using namespace ci;

/*
 See http://www.microsoft.com/typography/otspec/name.htm for a list of some
 possible platform-encoding pairs.  We're interested in 0-3 aka 3-1 - UCS-2.
 Otherwise, fail. If a font has some unicode map, but lacks UCS-2 - it is a
 broken or irrelevant font. What exactly Freetype will select on face load
 (it promises most wide unicode, and if that will be slower that UCS-2 -
 left as an excercise to check.)
 */
static FT_Error force_ucs2_charmap(FT_Face face)
{
    for (int i = 0; i < face->num_charmaps; i++)
    {
        auto platform_id = face->charmaps[i]->platform_id;
        auto encoding_id = face->charmaps[i]->encoding_id;
        
        if (((platform_id == 0) && (encoding_id == 3)) || ((platform_id == 3) && (encoding_id == 1)))
        {
            return FT_Set_Charmap(face, face->charmaps[i]);
        }
    }
    
    return -1;
}

static int nextPowerOfTwo(int x)
{
    int result = 1;
    
    while (result < x)
    {
        result <<= 1;
    }
    
    return result;
}

ActualFont::ActualFont(shared_ptr<FreetypeHelper> ftHelper, const fs::path &filePath, float baseSize, bool useMipmap)
:
ftHelper(ftHelper),
baseSize(baseSize),
useMipmap(useMipmap)
{
    FT_Error error = FT_New_Face(ftHelper->getLib(), filePath.c_str(), 0, &ftFace);
    
    if (error)
    {
        throw runtime_error("FREETYPE: ERROR " + toString(error));
    }
    
    if (force_ucs2_charmap(ftFace))
    {
        throw runtime_error("HARFBUZZ: FONT IS BROKEN OR IRRELEVANT");
    }
    
    // ---
    
    /*
     * USING A MATRIX WITH A MULTIPLIER ALLOWS FOR FRACTIONAL VALUES
     * TRICK FROM http://code.google.com/p/freetype-gl/
     *
     * - WITHOUT A FRACTIONAL ADVANCE: CHARACTER SPACING LOOKS DUMB
     * - WITHOUT A FRACTIONAL HEIGHT: SOME CHARACTERS WON'T BE PERFECTLY ALIGNED ON THE BASELINE
     */
    int res = 64;
    int dpi = 72;
    
    scale = Vec2f::one() / Vec2f(res, res) / 64;
    FT_Set_Char_Size(ftFace, baseSize * 64, 0, dpi * res, dpi * res);
    
    FT_Matrix matrix =
    {
        int((1.0 / res) * 0x10000L),
        int((0.0) * 0x10000L),
        int((0.0) * 0x10000L),
        int((1.0 / res) * 0x10000L)
    };
    
    FT_Set_Transform(ftFace, &matrix, NULL);
    
    // ---
    
    metrics.height = ftFace->size->metrics.height * scale.y;
    metrics.ascent = ftFace->size->metrics.ascender * scale.y;
    metrics.descent = -ftFace->size->metrics.descender * scale.y;

    metrics.lineThickness = ftFace->underline_thickness / 64.0f;
    metrics.underlineOffset = -ftFace->underline_position / 64.0f;
    
    //
    
    auto os2 = (TT_OS2*)FT_Get_Sfnt_Table(ftFace, ft_sfnt_os2);
    
    if (os2 && (os2->version != 0xFFFF))
    {
        metrics.strikethroughOffset = FT_MulFix(os2->yStrikeoutPosition, ftFace->size->metrics.y_scale) * scale.y;
    }
    else
    {
        metrics.strikethroughOffset = 0.5f * (metrics.ascent - metrics.descent);
    }
    
    // ---
    
    hbFont = hb_ft_font_create(ftFace, NULL);
}

ActualFont::~ActualFont()
{
    clearGlyphCache();
    
    hb_font_destroy(hbFont);
    FT_Done_Face(ftFace);
}

ActualFont::Glyph* ActualFont::getGlyph(uint32_t codepoint)
{
    auto entry = glyphCache.find(codepoint);
    
    if (entry == glyphCache.end())
    {
        Glyph *glyph = createGlyph(codepoint);
        
        if (glyph)
        {
            glyphCache[codepoint] = glyph;
        }
        
        return glyph;
    }
    else
    {
        return entry->second;
    }
}

void ActualFont::clearGlyphCache()
{
    for (auto entry : glyphCache)
    {
        delete entry.second;
    }
    
    glyphCache.clear();
}

ActualFont::Glyph* ActualFont::createGlyph(uint32_t codepoint)
{
    if (codepoint > 0)
    {
        auto error = FT_Load_Glyph(ftFace, codepoint, FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT);
        
        if (!error)
        {
            auto slot = ftFace->glyph;
            
            FT_Glyph glyph;
            error = FT_Get_Glyph(slot, &glyph);
            
            if (!error)
            {
                ActualFont::Glyph *g = NULL;
                FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
                
                auto width = slot->bitmap.width;
                auto height = slot->bitmap.rows;
                
                if (width * height > 0)
                {
                    auto texture = createTexture(slot->bitmap.buffer, width, height);
                    textureList.push_back(shared_ptr<gl::Texture>(texture));
                    
                    Vec2f offset(slot->bitmap_left, -slot->bitmap_top);
                    Vec2f size(width, height);
                    g = new Glyph(texture, offset, size);
                }
                
                FT_Done_Glyph(glyph);
                return g;
            }
        }
    }
    
    return NULL;
}

gl::Texture* ActualFont::createTexture(unsigned char *data, int width, int height)
{
    if (width * height > 0)
    {
        int textureWidth = nextPowerOfTwo(width);
        int textureHeight = nextPowerOfTwo(height);
        unique_ptr<unsigned char[]> textureData(new unsigned char[textureWidth * textureHeight]()); // ZERO-FILLED + AUTOMATICALLY FREED
        
        for (int iy = 0; iy < height; iy++)
        {
            for (int ix = 0; ix < width; ix++)
            {
                textureData[iy * textureWidth + ix] = data[iy * width + ix];
            }
        }
        
        gl::Texture::Format format;
        format.setInternalFormat(GL_ALPHA);
        
        if (useMipmap)
        {
            format.enableMipmapping(true);
            format.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
        }
        
        return new gl::Texture(textureData.get(), GL_ALPHA, textureWidth, textureHeight, format);
    }
    
    return NULL;
}
