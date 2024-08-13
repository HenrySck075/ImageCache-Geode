#include <Geode/Geode.hpp>
#include "../include/ImageCache.hpp"
using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto imgcache = ImageCache::instance();
        std::map<std::string, std::string> headers = {
            {"Upgrade-Insecure-Requests","1"}, 
            {"Referer", "https://www.pixiv.net/"},
            {"X-User-Id", "76179633"}
        };
        // download an image from the internet and give it a key
        // It's best to prefix the key with your mod id. If not, it will get the wrong image!
        imgcache->download(
            "https://i.pximg.net/img-original/img/2024/05/29/00/04/16/119141822_p0.png", // url
            headers, // headers
            "geodesdk-artwork"_spr, // key
            [](CCImage* img, std::string keyOrUrl) {
                // do something with the image here
                // images might sometimes be empty if theres errors, check the logs if there's any
            }
        );
        return true;
    }
};