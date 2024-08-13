
#include "../include/ImageCache.hpp"
#include "../include/b64.hpp"
#include <filesystem>

ImageCache* ImageCache::_instance = nullptr;

bool isUrl(std::string keyOrUrl) {
    // this also means this is the list of protocols that this supports
    return keyOrUrl.starts_with("https://") || keyOrUrl.starts_with("http://");
}

bool ImageCache::existsInCacheDir(std::string keyOrUrl) {
    std::filesystem::path j = saveDir;
    std::filesystem::path path = j.concat(base64_encode(keyOrUrl, true)+".png");
    bool ret = std::filesystem::exists(path);
    bool _isUrl = isUrl(keyOrUrl);
    // Cache the image in memory so we dont have to do this again
    if (ret) {
        auto tsDiff = std::chrono::duration_cast<std::chrono::days>(
            // last modified is more reasonable imo
            std::chrono::system_clock::now().time_since_epoch()
            - 
            std::filesystem::last_write_time(path).time_since_epoch() 
        );
        // cached image expires
        if (tsDiff.count()>=mod->getSettingValue<int64_t>("expires")) return false;

        CCImage* i = new CCImage();
        // idk
        if (i && i->initWithImageFile(path.string().c_str(),CCImage::EImageFormat::kFmtPng)) {}
        else {log::warn("Failed to initialize image with {} of {}. Result image will be empty.", _isUrl ? "URL" : "key", keyOrUrl);}
        imageDict[keyOrUrl] = i;
    }
    return ret;
}

void ImageCache::getImage(std::string keyOrUrl, std::map<std::string, std::string> headers, ImageCallback cb) {
    if (imageDict.contains(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
    };

    // check the function to see why
    if (existsInCacheDir(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
    }

    if (isUrl(keyOrUrl)) _download(keyOrUrl, headers, "", cb);
    else {
        log::warn("Image with specified key is not cached. Result image will be empty.");
        cb(new CCImage(), keyOrUrl);
    }
}

void ImageCache::addImage(std::string key, CCImage* image) {
    #ifndef GEODE_IS_MACOS image->saveToFile(filePath(key).string().c_str(), false);
    #endif
    imageDict[key] = image;
};


void ImageCache::_download(std::string url, std::map<std::string, std::string> headers, std::string key, ImageCallback cb) {
    auto r = web::WebRequest();
    auto saveStr = key.empty()?url:key;
    auto lk = std::make_pair(url, headers);
    for (auto& header : headers) {
        r.header(header.first, header.second);
    }
    // making it actually exists in the map
    if (!listeners.contains(lk)) listeners.insert({lk,new EventListener<web::WebTask>()});
    auto l = listeners[lk];
    l->bind([url,cb,key,saveStr,l,lk,this](web::WebTask::Event* j){
        if (j->isCancelled()) {
            log::info("request canceled");
            listeners.erase(lk);
            delete l;
            return;
        }
        if (auto res = j->getValue()) {
            if (res->ok()) {
                auto contentTypeHeader = res->header("Content-Type");
                //if (contentTypeHeader.value_or("application/octet-stream").starts_with("image/")) {
                    std::thread([res,saveStr,this,lk,l,cb,url,key](){
                        auto d = res->data();
                        auto img = new CCImage();
                        if (!img->initWithImageData(const_cast<uint8_t*>(d.data()), d.size())) {
                            log::warn("Failed to initialize image with URL of {} (key: {}). Result image will be empty.", url, key);
                            return;
                        };
                        #ifndef GEODE_IS_MACOS img->saveToFile(filePath(saveStr).string().c_str(),false);
                        #endif
                        imageDict[key] = img;
                        cb(img, saveStr);
                        listeners.erase(lk);
                        delete l;
                    }).detach();
                /*
                } else {
                    log::error("this is not an image dawg (its content type is {})", contentTypeHeader.value_or("application/octet-stream"));
                }
                */
            }
            else {
                log::error("Downloading image {} failed with error code {}", url, res->code());
            }
        }
    });
    l->setFilter(r.get(url));
}

void ImageCache::download(std::string url, std::map<std::string, std::string> headers, std::string key, ImageCallback cb) {
    if (url.empty()) {
        getImage(key,headers,cb);
        return;
    }
    auto keyOrUrl = key.empty()?url:key;
    if (imageDict.contains(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
    };

    // check the function to see why
    if (existsInCacheDir(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
    }

    log::info("Downloading {}. If theres nothing happens then your code sucks lmoa", url);
    _download(url, headers, key, cb);
}