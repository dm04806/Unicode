/*
 * THE UNICODE TEST SUITE FOR CINDER: https://github.com/arielm/Unicode
 * COPYRIGHT (C) 2013, ARIEL MALKA ALL RIGHTS RESERVED.
 *
 * THE FOLLOWING SOURCE-CODE IS DISTRIBUTED UNDER THE MODIFIED BSD LICENSE:
 * https://github.com/arielm/Unicode/blob/master/LICENSE.md
 */

#include "FontManager.h"

#include "cinder/app/App.h"
#include "cinder/Xml.h"
#include "cinder/Utilities.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace ci;
using namespace app;

const string REF_FILE = "file://";
const string REF_ASSETS = "assets://";

FontManager::FontManager()
{
    ftHelper = make_shared<FreetypeHelper>();
}

ActualFont* FontManager::getCachedFont(const string &ref, float fontSize, bool useMipmap)
{
    FontKey key(ref, fontSize, useMipmap);
    auto it = fontMap.find(key);
    
    if (it != fontMap.end())
    {
        return it->second.get();
    }
    else
    {
        auto filePath = getFilePath(ref);
        
        if (!filePath.empty())
        {
            ActualFont *font = new ActualFont(ftHelper, filePath, fontSize, useMipmap);
            fontMap[key] = shared_ptr<ActualFont>(font);
            
            return font;
        }
    }
    
    return NULL;
}

VirtualFont FontManager::loadVirtualFont(DataSourceRef source, float fontSize, bool useMipmap)
{
    VirtualFont tree;
    
    XmlTree doc(source);
    auto rootElement = doc.getChild("VirtualFont");
    
    for (auto &fontElement : rootElement.getChildren())
    {
        auto langList = getLanguageList(fontElement->getAttributeValue<string>("lang", ""));
        
        for (auto lang : langList)
        {
            for (auto &variantElement : fontElement->getChildren())
            {
                if (variantElement->getTag() == "Group")
                {
                    for (auto &refElement : variantElement->getChildren())
                    {
                        string ref = refElement->getValue();
                        
                        if (tree.add(lang, getCachedFont(ref, fontSize, useMipmap)))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    string ref = variantElement->getValue();
                    tree.add(lang, getCachedFont(ref, fontSize, useMipmap));
                }
            }
        }
    }
    
    return tree;
}

fs::path FontManager::getFilePath(const std::string &ref)
{
    fs::path filePath;
    
    if (boost::starts_with(ref, REF_ASSETS))
    {
        filePath = getAssetPath(ref.substr(REF_ASSETS.size()));
    }
    else if (boost::starts_with(ref, REF_FILE))
    {
        filePath = ref.substr(REF_FILE.size());
    }
    
    if (fs::exists(filePath))
    {
        return filePath;
    }
    else
    {
        return "";
    }
}

vector<string> FontManager::getLanguageList(const string &languages)
{
	return split(languages, ":");
}
