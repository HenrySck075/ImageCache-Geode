
#include "../include/ImageCache.hpp"
#include "../include/b64.hpp"
#include <filesystem>

ImageCache* ImageCache::_instance = nullptr;

bool isUrl(std::string keyOrUrl) {
    // this also means this is the list of protocols that this supports
    return keyOrUrl.starts_with("https://") || keyOrUrl.starts_with("http://");
}

bool ImageCache::existsInCacheDir(std::string keyOrUrl) {
    std::filesystem::path path = filePath(keyOrUrl);
    bool ret = std::filesystem::exists(path);
    bool _isUrl = isUrl(keyOrUrl);
    // Cache the image in memory so we dont have to do this again
    if (ret) {
        #define durcast std::chrono::duration_cast<std::chrono::days>
        auto tsDiff = 
            // last modified is more reasonable imo
            durcast(std::chrono::system_clock::now().time_since_epoch())
            - 
            durcast(std::filesystem::last_write_time(path).time_since_epoch())
        ;
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
        return;
    };

    // check the function to see why
    if (existsInCacheDir(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
        return;
    }

    if (isUrl(keyOrUrl)) _download(keyOrUrl, headers, "", cb);
    else {
        log::warn("Image with specified key is not cached. Result image will be empty.");
        cb(new CCImage(), keyOrUrl);
    }
}

void ImageCache::addImage(std::string key, CCImage* image) {
    #ifndef GEODE_IS_MACOS 
        image->saveToFile(filePath(key).string().c_str(), false);
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
                std::string ext = "";
                if (auto delimPos = url.find_last_of('.') != std::string::npos) ext = url.substr(delimPos+1,url.size());
                CCImage::EImageFormat imgFormat = m_imageFormatFromExtension[ext]; // you dont fucking supports everything else in the EImageFormat shithead
                // if it doesnt define Content-Type then we'll just assume it's a valid image data
                if (contentTypeHeader.has_value()) {
                    std::string contentType = contentTypeHeader.value();
                    if (imgFormat == CCImage::kFmtUnKnown) {
                        if (auto delimPos = contentType.find_first_of('/') != std::string::npos) ext = contentType.substr(delimPos+1,contentType.size());
                        imgFormat = m_imageFormatFromExtension[ext];
                    }
                    if (!contentType.starts_with("image/")) {
                        log::error("this is not an image dawg (its content type is {})", contentTypeHeader.value_or("application/octet-stream"));
                    }
                }
                std::thread([res,saveStr,this,lk,l,cb,url,key,imgFormat](){
                    auto d = res->data();
                    auto img = new CCImage();
                    
                    if (!img->initWithImageData(const_cast<uint8_t*>(d.data()), d.size(),imgFormat)) {
                        log::warn("Failed to initialize image with URL of {} (key: {}). Result image will be empty.", url, key);
                        return;
                    };
                    #ifndef GEODE_IS_MACOS 
                        img->saveToFile(filePath(saveStr).string().c_str(),false);
                    #endif
                    geode::Loader::get()->queueInMainThread([this, saveStr,img,lk,l,url,cb](){
                        imageDict[saveStr] = img;
                        cb(img, saveStr);
                        listeners.erase(lk);
                        delete l;
                        log::info("Downloading image {} complete!", url);
                    });
                }).detach();
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
        return;
    };

    // check the function to see why
    if (existsInCacheDir(keyOrUrl)) {
        cb(imageDict[keyOrUrl], keyOrUrl);
        return;
    }

    log::info("Downloading {} (key: {}).", url, key.empty()?"undefined":key);
    _download(url, headers, key, cb);
}

ImageCache *ImageCache::instance() {
  if (_instance == nullptr) {
    _instance = new ImageCache();
    _instance->mod = Mod::get();
    _instance->saveDir = _instance->mod->getSaveDir();
  }
  return _instance;
}

