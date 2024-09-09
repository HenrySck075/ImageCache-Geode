# ImageCache
This is where she makes a mod.

btw i might implement caching images made by other mods that isnt depending on this

# How to use
[See demo here](https://youtu.be/H8r-xMHN25Y)

Simply add this mod as a [dependencies](https://docs.geode-sdk.org/mods/dependencies), `#include <henrysck075.imgcache/include/ImageCache.hpp>` and you can now start saving/getting images like this:
```cpp
auto imgcache = ImageCache::instance();
// download an image from the internet and give it a key
// It's best to prefix the key with your mod ID to prevent possible overlap!
imgcache->download(
    "https://i.pximg.net/img-original/img/2023/01/01/15/27/17/104128429_p0.png", // url
    {{"Upgrade-Insecure-Requests","1"}, {"X-User-Id", "firee"}}, // headers
    "geodesdk-artwork"_spr, // key
    [](CCImage* img, std::string keyOrUrl) {
        // do something with the image here
        // images might sometimes be empty if theres errors, check the logs if there's any
    }
);

// Next time if you want to get the image, you dont need to wait for it to download the entire thing
imgcache->getImage("geodesdk-artwork", /*similar to above*/);
```

---
Now you might be wondering "then wtf is the difference between this and those bundled the similar thing in their mod".

It is that the cache is **preserved** even after you close the game! (except macos they do not have saveToFile defined) (will add when i make a macos vm) (which is probably not in a near future)
