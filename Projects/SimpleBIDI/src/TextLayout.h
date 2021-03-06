/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

#pragma once

#include "TextGroup.h"
#include "YFont.h"

struct Shape
{
    hb_codepoint_t codepoint;
    ci::Vec2f position;
    
    Shape(hb_codepoint_t codepoint, const ci::Vec2f &position)
    :
    codepoint(codepoint),
    position(position)
    {}
};

struct Cluster
{
    YFont *font;
    ci::ColorA color;
    
    float combinedAdvance;
    std::vector<Shape> shapes;
    
    Cluster(YFont *font, const ci::ColorA &color, hb_codepoint_t codepoint, const ci::Vec2f &offset, float advance);
    void addShape(hb_codepoint_t codepoint, const ci::Vec2f &offset, float advance);
};

class TextLayout
{
public:
    hb_direction_t overallDirection;
    float advance;
    std::vector<std::pair<Cluster, float>> clusters;
    
    TextLayout() {}
    TextLayout(std::map<hb_script_t, FontList> &fontMap, const TextSpan &span);
    TextLayout(std::map<hb_script_t, FontList> &fontMap, const TextGroup &group);
    
    void draw(const ci::Vec2f &position);
    
protected:
    void addCluster(const Cluster &cluster);
    void process(std::map<hb_script_t, FontList> &fontMap, const std::vector<TextSpan> &runs);
};
