#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

#include "b64.hpp"

#include <Geode/utils/general.hpp>

#ifdef GEODE_IS_WINDOWS
    #ifdef HENRYSCK_IMGCACHE_EXPORTING
        #define IMGC_DLL __declspec(dllexport)
    #else
        #define IMGC_DLL __declspec(dllimport)
    #endif
#else 
    #define IMGC_DLL __attribute__((visibility("default")))
#endif


class IMGC_DLL ImageCache {
private:
    static ImageCache* _instance;
    Mod* mod; // my mod
    std::filesystem::path saveDir;
    CCDictionaryExt<std::string, CCImage*> imageDict;
    std::map<std::pair<std::string, std::map<std::string, std::string>>, geode::EventListener<web::WebTask>*> listeners;

    std::filesystem::path filePath(std::string keyOrUrl) {
        return saveDir / (base64_encode(keyOrUrl)+".png");
    }
public:
    ImageCache() : mod(Mod::get()), saveDir(mod->getSaveDir()) {
        log::info("Save directory: {}", saveDir);
    };
    static ImageCache* instance() {
        if (_instance == nullptr) _instance = new ImageCache();
        return _instance;
    }

    using ImageCallback = geode::utils::MiniFunction<void(CCImage*, std::string keyOrUrl)>;
private:
    /// @brief Get the image from the url.
    /// 
    /// This **ALWAYS** overwrite the cached image.
    void _download(std::string url, std::map<std::string, std::string> headers, std::string key, ImageCallback cb);
    /// @returns true if it does and false if it isnt or the image exceeded the extended car warranty
    bool existsInCacheDir(std::string keyOrUrl);
public:
    /// @brief Get the image from the url.
    void download(std::string url, std::map<std::string, std::string> headers, std::string key, ImageCallback cb);
    void getImage(std::string keyOrUrl, std::map<std::string, std::string> headers, ImageCallback cb);
    // the image needs to be png
    void addImage(std::string key, CCImage* image);
};